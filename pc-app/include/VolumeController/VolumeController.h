#pragma once

#include <memory>
#include <string>
#include <vector>

/**
 * VolumeController - Controls system and application audio volumes
 *
 * This class provides a platform-independent interface to control
 * audio volumes for both the master system volume and per-application
 * volumes. Implementation details are hidden behind the PIMPL idiom.
 */
class AudioDriver {
public:
    AudioDriver();
    ~AudioDriver();

    class Impl;

    // Prevent copying, allow moving
    AudioDriver(const AudioDriver &) = delete;
    AudioDriver &operator=(const AudioDriver &) = delete;
    AudioDriver(AudioDriver &&) noexcept;
    AudioDriver &operator=(AudioDriver &&) noexcept;

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
    // Forward declaration of implementation class
    std::unique_ptr<Impl> pImpl;
};
