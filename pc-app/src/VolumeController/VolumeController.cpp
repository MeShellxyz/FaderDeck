#include "VolumeController.h"

#if defined(_WIN32) || defined(_WIN64)
#include "VolumeControllerImpl_Windows.h"
#elif defined(__linux__)
#error "Linux implementation not available yet"
#elif defined(__APPLE__)
#error "macOS implementation not available yet"
#else
#error "Unsupported platform"
#endif

// Constructor
VolumeController::VolumeController() : pImpl(std::make_unique<Impl>()) {}

// Destructor
VolumeController::~VolumeController() = default;

// Move constructor/assignment
VolumeController::VolumeController(VolumeController &&) noexcept = default;
VolumeController &
VolumeController::operator=(VolumeController &&) noexcept = default;

// Volume control methods
bool VolumeController::setMasterVolume(float volumeLevel) {
    return pImpl->setMasterVolume(volumeLevel);
}

bool VolumeController::setVolume(const std::string &processName,
                                 float volumeLevel) {
    return pImpl->setVolume(processName, volumeLevel);
}

bool VolumeController::setVolume(const std::vector<std::string> &processNames,
                                 float volumeLevel) {
    return pImpl->setVolume(processNames, volumeLevel);
}

// Mute control methods
bool VolumeController::setMasterMute(int mute) {
    return pImpl->setMasterMute(mute);
}

bool VolumeController::setMute(const std::string &processName, int mute) {
    return pImpl->setMute(processName, mute);
}

bool VolumeController::setMute(const std::vector<std::string> &processNames,
                               int mute) {
    return pImpl->setMute(processNames, mute);
}

// Notification for default device changes
void VolumeController::onDefaultDeviceChanged() {
    return pImpl->onDefaultDeviceChanged();
}
