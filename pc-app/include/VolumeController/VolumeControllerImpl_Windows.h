#pragma once
#include "VolumeController.h"

#include <atlbase.h>
#include <audiopolicy.h>
#include <endpointvolume.h>
#include <mmdeviceapi.h>
#include <windows.h>

#include <chrono>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

/**
 * Windows-specific implementation of VolumeController
 */
class VolumeController::Impl {
public:
    Impl();
    ~Impl();

    // Volume control methods
    bool setMasterVolume(float volumeLevel);
    bool setVolume(const std::string &processName, float volumeLevel);
    bool setVolume(const std::vector<std::string> &processNames,
                   float volumeLevel);

    // Mute control methods
    bool setMasterMute(int mute);
    bool setMute(const std::string &processName, int mute);
    bool setMute(const std::vector<std::string> &processNames, int mute);

    // Notification for default device changes
    void onDefaultDeviceChanged();

private:
    // Windows COM initialization
    bool initializeCOM();

    // Internal implementation methods (thread-unsafe)
    bool setVolumeInternal(const std::string &processName, float volumeLevel);
    bool setMuteInternal(const std::string &processName, int mute);

    // Helper methods
    CComPtr<IMMNotificationClient> pNotificationClient;

    // Process utilities
    struct CacheProcessEntry {
        std::string name;
        std::chrono::system_clock::time_point timestamp;
    };

    static std::unordered_map<DWORD, CacheProcessEntry> processNameCache;
    static std::string WideToUtf8(const wchar_t *wstr);
    static std::string getProcessNameFromId(const DWORD &processId);

    // Audio session management
    std::vector<CComPtr<ISimpleAudioVolume>>
    getAudioSessionsForProcess(const std::string &processName);

    // Windows COM interfaces
    CComPtr<IMMDeviceEnumerator> pEnumerator;
    CComPtr<IMMDevice> pDevice;
    CComPtr<IAudioEndpointVolume> pEndpointVolume;
    CComPtr<IAudioSessionManager2> pSessionManager;

    // Thread safety
    std::mutex mtx;
};

class NotificationClient : public IMMNotificationClient {
    LONG m_refCount = 1;
    VolumeController::Impl *pImpl;

public:
    NotificationClient(VolumeController::Impl *impl) : pImpl(impl) {}

    // IUnknown methods
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid,
                                             void **ppvObject) override {
        if (riid == IID_IUnknown || riid == __uuidof(IMMNotificationClient)) {
            *ppvObject = static_cast<IMMNotificationClient *>(this);
            AddRef();
            return S_OK;
        }
        *ppvObject = nullptr;
        return E_NOINTERFACE;
    };
    ULONG STDMETHODCALLTYPE AddRef() override {
        return InterlockedIncrement(&m_refCount);
    };
    ULONG STDMETHODCALLTYPE Release() override {
        ULONG refCount = InterlockedDecrement(&m_refCount);
        if (refCount == 0) {
            delete this;
        }
        return refCount;
    };

    // IMMNotificationClient methods
    HRESULT STDMETHODCALLTYPE OnDefaultDeviceChanged(
        EDataFlow flow, ERole role, LPCWSTR pwstrDefaultDeviceId) override {
        if (flow == eRender && role == eConsole) {
            pImpl->onDefaultDeviceChanged();
        }
        return S_OK;
    };

    // These methods can be implemented as needed
    HRESULT STDMETHODCALLTYPE OnDeviceStateChanged(LPCWSTR pwstrDeviceId,
                                                   DWORD dwNewState) override {
        return S_OK;
    };
    HRESULT STDMETHODCALLTYPE OnDeviceAdded(LPCWSTR pwstrDeviceId) override {
        return S_OK;
    };
    HRESULT STDMETHODCALLTYPE OnDeviceRemoved(LPCWSTR pwstrDeviceId) override {
        return S_OK;
    };
    HRESULT STDMETHODCALLTYPE OnPropertyValueChanged(
        LPCWSTR pwstrDeviceId, const PROPERTYKEY key) override {
        return S_OK;
    };
};