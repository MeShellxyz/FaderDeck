#pragma once

#include <audiopolicy.h>
#include <mmdeviceapi.h>
#include <wrl/implements.h>

class DeviceNotificationClient
    : public Microsoft::WRL::RuntimeClass<
          Microsoft::WRL::RuntimeClassFlags<Microsoft::WRL::ClassicCom>,
          IMMNotificationClient> {
    std::atomic<bool> &m_resetFlag;
    std::atomic<uint64_t> &m_wakeupSignal;

public:
    DeviceNotificationClient(std::atomic<bool> &m_resetFlag,
                             std::atomic<uint64_t> &m_wakeupSignal)
        : m_resetFlag(m_resetFlag), m_wakeupSignal(m_wakeupSignal) {}

    // ~DeviceNotificationClient() = default;

    // IMMNotificationClient methods
    IFACEMETHODIMP OnDeviceStateChanged(LPCWSTR pwstrDeviceId,
                                        DWORD dwNewState) override {
        m_resetFlag.store(true, std::memory_order_release);
        m_wakeupSignal.fetch_add(1, std::memory_order_release);
        m_wakeupSignal.notify_all();
        return S_OK;
    }
    IFACEMETHODIMP OnDeviceAdded(LPCWSTR pwstrDeviceId) override {
        return S_OK;
    }
    IFACEMETHODIMP OnDeviceRemoved(LPCWSTR pwstrDeviceId) override {
        return S_OK;
    }
    IFACEMETHODIMP
    OnDefaultDeviceChanged(EDataFlow flow, ERole role,
                           LPCWSTR pwstrDefaultDeviceId) override {
        m_resetFlag.store(true, std::memory_order_release);
        m_wakeupSignal.fetch_add(1, std::memory_order_release);
        m_wakeupSignal.notify_all();
        return S_OK;
    }
    IFACEMETHODIMP OnPropertyValueChanged(LPCWSTR pwstrDeviceId,
                                          const PROPERTYKEY key) override {
        return S_OK;
    }
};

class ManagerNotificationClient
    : public Microsoft::WRL::RuntimeClass<
          Microsoft::WRL::RuntimeClassFlags<Microsoft::WRL::ClassicCom>,
          IAudioSessionNotification> {
    std::atomic<bool> &m_refreshFlag;
    std::atomic<uint64_t> &m_wakeupSignal;

public:
    ManagerNotificationClient(std::atomic<bool> &refreshFlag,
                              std::atomic<uint64_t> &wakeupSignal)
        : m_refreshFlag(refreshFlag), m_wakeupSignal(wakeupSignal) {}

    // ~ManagerNotificationClient() = default;

    // IAudioSessionNotification method
    IFACEMETHODIMP OnSessionCreated(IAudioSessionControl *NewSession) override {

        m_refreshFlag.store(true, std::memory_order_release);
        m_wakeupSignal.fetch_add(1, std::memory_order_release);
        m_wakeupSignal.notify_all();
        return S_OK;
    }
};
