#pragma once

#include <atomic>
#include <memory>
#include <mutex>

struct MixerSharedState {
    size_t numChannels;
    bool muteButtons;
    std::unique_ptr<std::atomic<float>[]> channelVolumes;
    std::unique_ptr<std::atomic<bool>[]> channelMuteStates;
    std::atomic<uint64_t> version{0};
    std::atomic<bool> comReady{false};

    MixerSharedState(size_t numChannels, bool muteButtons)
        : numChannels(numChannels),
          muteButtons(muteButtons),
          channelVolumes(std::make_unique<std::atomic<float>[]>(numChannels)),
          channelMuteStates(
              std::make_unique<std::atomic<bool>[]>(numChannels)) {
        for (size_t i = 0; i < numChannels; ++i) {
            channelVolumes[i].store(0.0f);     // Default volume - muted
            channelMuteStates[i].store(false); // Default mute state - unmuted
        }
    }
    MixerSharedState() = delete;
    MixerSharedState(const MixerSharedState &) = delete;
    MixerSharedState &operator=(const MixerSharedState &) = delete;
};
