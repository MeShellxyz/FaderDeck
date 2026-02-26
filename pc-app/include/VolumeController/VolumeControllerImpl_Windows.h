#pragma once
#include "VolumeController.h"

#include <atlbase.h>
#include <atlcom.h>
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
class AudioDriver::Impl {
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
};
