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
class VolumeController {
public:
    VolumeController();
    ~VolumeController();

    class Impl;

    // Prevent copying, allow moving
    VolumeController(const VolumeController &) = delete;
    VolumeController &operator=(const VolumeController &) = delete;
    VolumeController(VolumeController &&) noexcept;
    VolumeController &operator=(VolumeController &&) noexcept;

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