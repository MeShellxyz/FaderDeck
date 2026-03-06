#pragma once

#include <memory>
#include <string>
#include <vector>

#include "Config/AppConfig.h"
#include "Core/MixerSharedState.h"
#include "Core/Platform.h"

#ifdef FD_PLATFORM_WINDOWS

#include <wrl/client.h>
using Microsoft::WRL::ComPtr;

#include <audiopolicy.h>
#include <endpointvolume.h>
#include <mmdeviceapi.h>
#include <windows.h>

#include <chrono>
#include <shared_mutex>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>
#endif

#if defined(FD_PLATFORM_WINDOWS)
struct ComInitGuard {
    HRESULT hr;
    ComInitGuard() { hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED); }
    ~ComInitGuard() { if (hr == S_OK) CoUninitialize(); }
};
class DeviceNotificationClient;
class ManagerNotificationClient;
#endif

/**
 * VolumeController - Controls system and application audio volumes
 *
 * This class provides a platform-independent interface to control
 * audio volumes for both the master system volume and per-application
 * volumes. Implementation details are hidden behind the PIMPL idiom.
 */
class AudioDriver {
public:
    AudioDriver(const std::atomic<bool> &isRunning, const AudioConfig &config,
                MixerSharedState &sharedState);
    ~AudioDriver() = default;

    // Prevent copying and moving
    AudioDriver(const AudioDriver &) = delete;
    AudioDriver &operator=(const AudioDriver &) = delete;
    AudioDriver(AudioDriver &&) = delete;
    AudioDriver &operator=(AudioDriver &&) = delete;

    void run();

private:
    // Audio interface wrapper methods
    bool setVolume(int channelIndex, float volumeLevel);
    bool setMute(int channelIndex, int mute);
    bool setMasterVolume(float volumeLevel);
    bool setMasterMute(int mute);

#if defined(FD_PLATFORM_WINDOWS)
    inline void handleDeviceChange();
    inline void handleCacheReset();

    inline void applyMutes();
    inline void applyVolumes();

    bool initGlobalInterfaces();
    void releaseGlobalInterfaces();

    bool initDeviceInterfaces();
    void releaseDeviceInterfaces();

    std::wstring getProcessNameFromPID(const DWORD &processId);
    void refreshAudioSessionsCache();

    void normalizeWString(std::wstring &input);

#endif

    int m_masterChannelIndex{-1};

    const std::atomic<bool> &m_isRunning;
    const AudioConfig &m_audioConfig;
    MixerSharedState &m_sharedState;

/// NEW NEW NEW
#if defined(FD_PLATFORM_WINDOWS)
    std::atomic<bool> m_needsDeviceReset{false};
    std::atomic<bool> m_needsCacheRefresh{true};

    // Cache
    std::vector<std::vector<ComPtr<ISimpleAudioVolume>>> m_sessionsCache;
    std::shared_mutex m_cacheMutex;

    // Pointers to Windows audio interfaces
    ComPtr<IMMDeviceEnumerator> m_deviceEnumerator;
    ComPtr<IMMDevice> m_defaultDevice;
    ComPtr<IAudioEndpointVolume> m_endpointVolume;
    ComPtr<IAudioSessionManager2> m_sessionManager2;

    // Pointers to notification clients
    ComPtr<IMMNotificationClient> m_deviceNotificationClient;
    ComPtr<IAudioSessionNotification> m_managerNotificationClient;

#endif
};