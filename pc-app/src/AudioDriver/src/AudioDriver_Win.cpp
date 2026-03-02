#include "AudioDriver/AudioDriver.h"
#include "AudioNotificationClients_Win.h"

#include <TlHelp32.h>
#include <algorithm>
#include <codecvt>
#include <set>
#include <stdexcept>

#include <array>
#include <filesystem>
#include <numeric>
#include <vector>

AudioDriver::AudioDriver(const std::atomic<bool> &isRunning,
                         const AudioConfig &config,
                         MixerSharedState &sharedState)
    : m_isRunning(isRunning),
      m_audioConfig(config),
      m_sharedState(sharedState) {
    // Initialize master channel index if mapped
    auto it = m_audioConfig.processChannelMapping.find(L"master");
    if (it != m_audioConfig.processChannelMapping.end()) {
        m_masterChannelIndex = it->second;
    }
}

void AudioDriver::run() {
    ComInitGuard comGuard;
    if (comGuard.hr != S_OK)
        throw std::runtime_error("Failed to initialize COM.");

    if (!initGlobalInterfaces())
        throw std::runtime_error("Failed to initialize audio interfaces.");

    if (!initDeviceInterfaces()) {
        releaseGlobalInterfaces();
        throw std::runtime_error(
            "Failed to initialize audio device interfaces.");
    }

    uint64_t lastVersion = 0;

    while (m_isRunning.load(std::memory_order_acquire)) {
        handleDeviceChange();
        handleCacheReset();

        applyVolumes();

        m_sharedState.version.wait(lastVersion, std::memory_order_acquire);
        lastVersion = m_sharedState.version.load(std::memory_order_acquire);
    }

    releaseDeviceInterfaces();
    releaseGlobalInterfaces();
}

bool AudioDriver::setVolume(int channelIndex, float volumeLevel) {
    std::shared_lock lock(m_cacheMutex);
    if (channelIndex < 0 || channelIndex >= m_audioConfig.num_channels ||
        channelIndex == m_masterChannelIndex)
        return false;

    for (const auto &sessionVolume : m_sessionsCache[channelIndex]) {
        if (FAILED(sessionVolume->SetMasterVolume(volumeLevel, nullptr))) {
            return false;
        }
    }

    return true;
}

bool AudioDriver::setMute(int channelIndex, int mute) {
    std::shared_lock lock(m_cacheMutex);
    if (channelIndex < 0 || channelIndex >= m_audioConfig.num_channels ||
        channelIndex == m_masterChannelIndex)
        return false;

    for (const auto &sessionVolume : m_sessionsCache[channelIndex]) {
        if (FAILED(sessionVolume->SetMute(mute, nullptr))) {
            return false;
        }
    }

    return true;
}

bool AudioDriver::setMasterVolume(float volumeLevel) {
    if (!m_endpointVolume) return false;

    return SUCCEEDED(
        m_endpointVolume->SetMasterVolumeLevelScalar(volumeLevel, nullptr));
}

bool AudioDriver::setMasterMute(int mute) {
    if (!m_endpointVolume) return false;

    return SUCCEEDED(m_endpointVolume->SetMute(mute, nullptr));
}

void AudioDriver::handleDeviceChange() {
    if (m_needsDeviceReset.load(std::memory_order_acquire)) {
        releaseDeviceInterfaces();
        if (!initDeviceInterfaces()) {
            throw std::runtime_error(
                "Failed to reinitialize audio device interfaces.");
        }
        m_needsDeviceReset.store(false, std::memory_order_release);
        m_needsCacheRefresh.store(true, std::memory_order_release);
    }
}

void AudioDriver::handleCacheReset() {
    if (m_needsCacheRefresh.load(std::memory_order_acquire)) {
        std::unique_lock lock(m_cacheMutex);
        refreshAudioSessionsCache();
        m_needsCacheRefresh.store(false, std::memory_order_release);
    }
}

void AudioDriver::applyVolumes() {
    bool isChanging = true;
    size_t timestamp = 0;
    constexpr float stabilizationThreshold = 0.01f;
    constexpr size_t rollbackWindow = 20;
    std::vector<std::array<float, rollbackWindow>> volumesHistory(
        m_audioConfig.num_channels);

    while (isChanging && m_isRunning.load(std::memory_order_acquire) &&
           !m_needsDeviceReset.load(std::memory_order_acquire) &&
           !m_needsCacheRefresh.load(std::memory_order_acquire)) {

        // Apply volumes and store history

        for (size_t i = 0; i < m_audioConfig.num_channels; i++) {
            float currentVolume =
                m_sharedState.channelVolumes[i].load(std::memory_order_relaxed);

            if (i == m_masterChannelIndex) {
                if (!setMasterVolume(currentVolume)) {
                    m_needsDeviceReset.store(true, std::memory_order_release);
                    break;
                }
            } else if (!setVolume(i, currentVolume)) {
                m_needsCacheRefresh.store(true, std::memory_order_release);
                break;
            }

            volumesHistory[i][timestamp % rollbackWindow] = currentVolume;
        }

        // Check if volmes stabilized
        if (timestamp >= rollbackWindow) {
            float sum = 0.0f;
            for (size_t i = 0; i < m_audioConfig.num_channels; i++) {
                for (size_t j = 0; j < rollbackWindow; j++) {
                    sum += volumesHistory[i][j];
                    if (sum > stabilizationThreshold) break;
                }
                if (sum > stabilizationThreshold) break;
            }

            if (sum < stabilizationThreshold) isChanging = false;
        }

        // Sleep for set framerate or until version changes
        timestamp++;
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

bool AudioDriver::initGlobalInterfaces() {
    HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr,
                                  CLSCTX_ALL, __uuidof(IMMDeviceEnumerator),
                                  (void **)m_deviceEnumerator.GetAddressOf());
    if (FAILED(hr)) return false;

    m_deviceNotificationClient = Microsoft::WRL::Make<DeviceNotificationClient>(
        m_needsDeviceReset, m_sharedState.version);
    hr = m_deviceEnumerator->RegisterEndpointNotificationCallback(
        m_deviceNotificationClient.Get());

    if (FAILED(hr)) return false;
}

void AudioDriver::releaseGlobalInterfaces() {
    if (m_deviceEnumerator && m_deviceNotificationClient) {
        m_deviceEnumerator->UnregisterEndpointNotificationCallback(
            m_deviceNotificationClient.Get());
    }
    m_deviceNotificationClient.Reset();
    m_deviceEnumerator.Reset();
}

bool AudioDriver::initDeviceInterfaces() {
    HRESULT hr = m_deviceEnumerator->GetDefaultAudioEndpoint(eRender, eConsole,
                                                             &m_defaultDevice);
    if (FAILED(hr)) return false;

    hr = m_defaultDevice->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_ALL,
                                   nullptr,
                                   (void **)m_endpointVolume.GetAddressOf());
    if (FAILED(hr)) return false;

    hr = m_defaultDevice->Activate(__uuidof(IAudioSessionManager2), CLSCTX_ALL,
                                   nullptr,
                                   (void **)m_sessionManager2.GetAddressOf());
    if (FAILED(hr)) return false;

    m_managerNotificationClient =
        Microsoft::WRL::Make<ManagerNotificationClient>(m_needsCacheRefresh,
                                                        m_sharedState.version);
    hr = m_sessionManager2->RegisterSessionNotification(
        m_managerNotificationClient.Get());

    if (FAILED(hr)) return false;

    return true;
}

void AudioDriver::releaseDeviceInterfaces() {
    if (m_sessionManager2 && m_managerNotificationClient) {
        m_sessionManager2->UnregisterSessionNotification(
            m_managerNotificationClient.Get());
    }
    m_managerNotificationClient.Reset();
    m_sessionManager2.Reset();
    m_endpointVolume.Reset();
    m_defaultDevice.Reset();
}

std::wstring AudioDriver::getProcessNameFromPID(const DWORD &processId) {
    std::wstring processName = L"<unknown>";
    HANDLE hProcess =
        OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, 0, processId);
    if (!hProcess) return processName;

    WCHAR szProcessPath[MAX_PATH] = {0};
    DWORD dwSize = MAX_PATH;
    if (QueryFullProcessImageNameW(hProcess, 0, szProcessPath, &dwSize)) {
        std::filesystem::path p(szProcessPath);
        processName = p.filename().wstring();
        return processName;
    }

    return processName;
}

void AudioDriver::refreshAudioSessionsCache() {
    if (!m_sessionsCache.empty()) m_sessionsCache.clear();

    ComPtr<IAudioSessionEnumerator> pSessionEnumerator = nullptr;
    HRESULT hr = m_sessionManager2->GetSessionEnumerator(&pSessionEnumerator);

    if (FAILED(hr)) return;

    int sessionCount = 0;
    hr = pSessionEnumerator->GetCount(&sessionCount);
    if (FAILED(hr)) return;

    ComPtr<IAudioSessionControl> pSessionControl = nullptr;
    ComPtr<IAudioSessionControl2> pSessionControl2 = nullptr;
    ComPtr<ISimpleAudioVolume> pSimpleAudioVolume = nullptr;

    for (int i = 0; i < sessionCount; ++i) {
        hr = pSessionEnumerator->GetSession(i, &pSessionControl);
        if (FAILED(hr)) continue;

        hr = pSessionControl->QueryInterface(
            __uuidof(IAudioSessionControl2),
            (void **)pSessionControl2.GetAddressOf());
        if (FAILED(hr)) continue;

        DWORD pid = 0;
        hr = pSessionControl2->GetProcessId(&pid);
        if (FAILED(hr)) continue;

        std::wstring processName = getProcessNameFromPID(pid);
        normalizeWString(processName);

        auto it = m_audioConfig.processChannelMapping.find(processName);
        if (it != m_audioConfig.processChannelMapping.end()) {
            int channelIndex = it->second;
            if (channelIndex >= 0 &&
                channelIndex < m_audioConfig.num_channels) {
                hr = pSessionControl2->QueryInterface(
                    __uuidof(ISimpleAudioVolume),
                    (void **)pSimpleAudioVolume.GetAddressOf());
                if (FAILED(hr)) continue;

                m_sessionsCache[channelIndex].push_back(pSimpleAudioVolume);
            }
        }

        pSimpleAudioVolume.Reset();
        pSessionControl2.Reset();
        pSessionControl.Reset();
    }
}

void AudioDriver::normalizeWString(std::wstring &input) {
    CharLowerBuffW(input.data(), static_cast<DWORD>(input.size()));
}