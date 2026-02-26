#include "AudioDriver/AudioDriver.h"

#include <TlHelp32.h>
#include <algorithm>
#include <codecvt>
#include <set>
#include <stdexcept>

// Initialize static process name cache
std::unordered_map<DWORD, AudioDriver::CacheProcessEntry>
    AudioDriver::processNameCache;

AudioDriver::AudioDriver() {
    if (!initializeCOM()) {
        throw std::runtime_error("Failed to initialize COM.");
    }
}

AudioDriver::~AudioDriver() {
    // Unregister notification callback to prevent callbacks to freed Impl
    if (pEnumerator && pNotificationClient) {
        // Unregister; use raw pointer from CComPtr
        pEnumerator->UnregisterEndpointNotificationCallback(pNotificationClient.p);
    }

    CoUninitialize();
}

bool AudioDriver::initializeCOM() {
    // Initialize COM library
    HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (FAILED(hr)) {
        return false;
    }

    // Create device enumerator
    hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL,
                          __uuidof(IMMDeviceEnumerator),
                          reinterpret_cast<void **>(&pEnumerator));

    if (FAILED(hr)) {
        CoUninitialize();
        return false;
    }

    // Get default audio endpoint
    hr = pEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &pDevice);
    if (FAILED(hr)) {
        CoUninitialize();
        return false;
    }

    // Register for device notifications (default endpoint changes)
    CComObject<NotificationClient>* pNotifRaw = nullptr;
    hr = CComObject<NotificationClient>::CreateInstance(&pNotifRaw);
    if (FAILED(hr) || !pNotifRaw) {
        CoUninitialize();
        return false;
    }

    pNotifRaw->Init(this);

    CComPtr<IMMNotificationClient> spNotify;
    hr = pNotifRaw->QueryInterface(__uuidof(IMMNotificationClient),
                                   reinterpret_cast<void**>(&spNotify));
    if (FAILED(hr)) {
        delete pNotifRaw;
        CoUninitialize();
        return false;
    }

    hr = pEnumerator->RegisterEndpointNotificationCallback(spNotify);
    if (FAILED(hr)) {
        spNotify.Release();
        CoUninitialize();
        return false;
    }

    pNotificationClient = spNotify;


    // Activate audio endpoint volume interface
    hr = pDevice->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_ALL, nullptr,
                           reinterpret_cast<void **>(&pEndpointVolume));

    if (FAILED(hr)) {
        CoUninitialize();
        return false;
    }

    // Activate audio session manager interface
    hr = pDevice->Activate(__uuidof(IAudioSessionManager2), CLSCTX_ALL, nullptr,
                           reinterpret_cast<void **>(&pSessionManager));

    if (FAILED(hr)) {
        CoUninitialize();
        return false;
    }

    return true;
}

std::string AudioDriver::WideToUtf8(const wchar_t *wstr) {
    if (!wstr)
        return "";

    std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converter;
    try {
        return converter.to_bytes(wstr);
    } catch (const std::exception &) {
        return "<conversion_error>";
    }
}

std::string
AudioDriver::getProcessNameFromId(const DWORD &processId) {
    // Check cache first (valid for 30 seconds)
    auto now = std::chrono::system_clock::now();
    auto it = processNameCache.find(processId);
    if (it != processNameCache.end() &&
        (now - it->second.timestamp) < std::chrono::seconds(30)) {
        return it->second.name;
    }

    // Not in cache, need to query process name
    std::string processName = "<unknown>";
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ,
                                  FALSE, processId);

    if (hProcess) {
        WCHAR szProcessPath[MAX_PATH] = {0};
        DWORD dwSize = MAX_PATH;

        // Get the full process path
        if (QueryFullProcessImageNameW(hProcess, 0, szProcessPath, &dwSize)) {
            WCHAR *fileName = wcsrchr(szProcessPath, L'\\');
            if (fileName) {
                // Skip the backslash and convert to UTF-8
                fileName++;
                processName = WideToUtf8(fileName);

                // Update cache
                processNameCache[processId] = {processName, now};
            }
        }
        CloseHandle(hProcess);
    }

    return processName;
}

std::vector<CComPtr<ISimpleAudioVolume>>
AudioDriver::getAudioSessionsForProcess(
    const std::string &processName) {
    std::vector<CComPtr<ISimpleAudioVolume>> sessions;

    // Get session enumerator
    CComPtr<IAudioSessionEnumerator> pSessionEnumerator = nullptr;
    HRESULT hr = pSessionManager->GetSessionEnumerator(&pSessionEnumerator);
    if (FAILED(hr)) {
        return sessions;
    }

    // Get session count
    int sessionCount = 0;
    hr = pSessionEnumerator->GetCount(&sessionCount);
    if (FAILED(hr)) {
        return sessions;
    }

    // Convert process name to lowercase for case-insensitive comparison
    std::string processNameLower = processName;
    std::transform(processNameLower.begin(), processNameLower.end(),
                   processNameLower.begin(), ::towlower);

    // Iterate through all audio sessions
    for (int i = 0; i < sessionCount; i++) {
        // Get session control
        CComPtr<IAudioSessionControl> pSessionControl = nullptr;
        hr = pSessionEnumerator->GetSession(i, &pSessionControl);
        if (FAILED(hr)) {
            continue;
        }

        // Get session control 2
        CComPtr<IAudioSessionControl2> pSessionControl2 = nullptr;
        hr = pSessionControl->QueryInterface(
            __uuidof(IAudioSessionControl2),
            reinterpret_cast<void **>(&pSessionControl2));

        if (FAILED(hr)) {
            continue;
        }

        // Get process ID
        DWORD processId = 0;
        hr = pSessionControl2->GetProcessId(&processId);
        if (FAILED(hr)) {
            continue;
        }

        // Get process name and convert to lowercase
        std::string currentProcessName = getProcessNameFromId(processId);
        std::transform(currentProcessName.begin(), currentProcessName.end(),
                       currentProcessName.begin(), ::towlower);

        // If process name matches
        if (currentProcessName == processNameLower) {
            // Get simple audio volume interface
            CComPtr<ISimpleAudioVolume> pSimpleVolume = nullptr;
            hr = pSessionControl->QueryInterface(
                __uuidof(ISimpleAudioVolume),
                reinterpret_cast<void **>(&pSimpleVolume));

            if (SUCCEEDED(hr)) {
                sessions.push_back(pSimpleVolume);
            }
        }
    }

    return sessions;
}

bool AudioDriver::setMasterVolume(float volumeLevel) {
    // Clip volume level to valid range [0.0, 1.0]
    volumeLevel = std::clamp(volumeLevel, 0.0f, 1.0f);

    // Set master volume
    HRESULT hr =
        pEndpointVolume->SetMasterVolumeLevelScalar(volumeLevel, nullptr);

    return SUCCEEDED(hr);
}

bool AudioDriver::setVolumeInternal(const std::string &processName,
                                               float volumeLevel) {
    // Clip volume level to valid range [0.0, 1.0]
    volumeLevel = std::clamp(volumeLevel, 0.0f, 1.0f);

    // Convert process name to lowercase for case-insensitive comparison
    std::string processNameLower = processName;
    std::transform(processNameLower.begin(), processNameLower.end(),
                   processNameLower.begin(), ::towlower);

    // Special case for master volume
    if (processNameLower == "master") {
        return setMasterVolume(volumeLevel);
    }

    // Set volume for all sessions of the specified process
    for (auto &session : getAudioSessionsForProcess(processName)) {
        if (session) {
            HRESULT hr = session->SetMasterVolume(volumeLevel, nullptr);
            if (FAILED(hr)) {
                return false;
            }
        }
    }

    return true;
}

bool AudioDriver::setVolume(const std::string &processName,
                                       float volumeLevel) {
    // Thread-safe access to volume control
    std::lock_guard<std::mutex> lock(mtx);
    return setVolumeInternal(processName, volumeLevel);
}

bool AudioDriver::setVolume(
    const std::vector<std::string> &processNames, float volumeLevel) {
    // Thread-safe access to volume control
    std::lock_guard<std::mutex> lock(mtx);

    // Clip volume level to valid range [0.0, 1.0]
    volumeLevel = std::clamp(volumeLevel, 0.0f, 1.0f);

    std::set<CComPtr<ISimpleAudioVolume>> uniqueSessions;

    // Set volume for all specified processes
    for (const auto &processName : processNames) {
        std::string processNameLower = processName;
        std::transform(processNameLower.begin(), processNameLower.end(),
                       processNameLower.begin(), ::towlower);

        if (processNameLower == "master") {
            return setMasterVolume(volumeLevel);
        }

        for (auto &session : getAudioSessionsForProcess(processNameLower)) {
            if (session) {
                uniqueSessions.insert(session);
            }
        }
    }

    for (auto &session : uniqueSessions) {
        if (session) {
            HRESULT hr = session->SetMasterVolume(volumeLevel, nullptr);
            if (FAILED(hr)) {
                return false;
            }
        }
    }

    return true;
}

bool AudioDriver::setMasterMute(int mute) {
    // Set master mute state
    HRESULT hr = pEndpointVolume->SetMute(mute, nullptr);
    return SUCCEEDED(hr);
}

bool AudioDriver::setMuteInternal(const std::string &processName,
                                             int mute) {
    // Convert process name to lowercase for case-insensitive comparison
    std::string processNameLower = processName;
    std::transform(processNameLower.begin(), processNameLower.end(),
                   processNameLower.begin(), ::towlower);

    // Special case for master mute
    if (processNameLower == "master") {
        return setMasterMute(mute);
    }

    // Set mute for all sessions of the specified process
    for (auto &session : getAudioSessionsForProcess(processName)) {
        if (session) {
            HRESULT hr = session->SetMute(mute, nullptr);
            if (FAILED(hr)) {
                return false;
            }
        }
    }

    return true;
}

bool AudioDriver::setMute(const std::string &processName, int mute) {
    // Thread-safe access to mute control
    std::lock_guard<std::mutex> lock(mtx);
    return setMuteInternal(processName, mute);
}

bool AudioDriver::setMute(
    const std::vector<std::string> &processNames, int mute) {
    // Thread-safe access to mute control
    std::lock_guard<std::mutex> lock(mtx);

    std::set<CComPtr<ISimpleAudioVolume>> uniqueSessions;

    for (const auto &processName : processNames) {
        std::string processNameLower = processName;
        std::transform(processNameLower.begin(), processNameLower.end(),
                       processNameLower.begin(), ::towlower);

        if (processNameLower == "master") {
            return setMasterMute(mute);
        }

        for (auto &session : getAudioSessionsForProcess(processNameLower)) {
            if (session) {
                uniqueSessions.insert(session);
            }
        }
    }

    // Set mute for all specified processes
    for (auto &session : uniqueSessions) {
        if (session) {
            HRESULT hr = session->SetMute(mute, nullptr);
            if (FAILED(hr)) {
                return false;
            }
        }
    }

    return true;
}

void AudioDriver::onDefaultDeviceChanged() {
    std::lock_guard<std::mutex> lock(mtx);

    pDevice.Release();
    pEndpointVolume.Release();
    pSessionManager.Release();

    HRESULT hr =
        pEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &pDevice);
    if (SUCCEEDED(hr)) {
        pDevice->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_ALL, nullptr,
                          reinterpret_cast<void **>(&pEndpointVolume));
        pDevice->Activate(__uuidof(IAudioSessionManager2), CLSCTX_ALL, nullptr,
                          reinterpret_cast<void **>(&pSessionManager));
    }
}
