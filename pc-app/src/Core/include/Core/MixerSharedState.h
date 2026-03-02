#pragma once

#include <atomic>
#include <memory>
#include <mutex>

struct MixerSharedState {
    size_t numChannels;
    std::unique_ptr<std::atomic<float>[]> channelVolumes;
    std::atomic<uint64_t> version{0};

    MixerSharedState(size_t numChannels)
        : numChannels(numChannels),
          channelVolumes(std::make_unique<std::atomic<float>[]>(numChannels)) {
        for (size_t i = 0; i < numChannels; ++i)
            channelVolumes[i].store(0.0f); // Default volume - muted
    }
    MixerSharedState() = delete;
    MixerSharedState(const MixerSharedState &) = delete;
    MixerSharedState &operator=(const MixerSharedState &) = delete;
};
