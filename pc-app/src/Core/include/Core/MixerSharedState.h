#pragma once

#include <atomic>
#include <memory>
#include <mutex>

struct MixerSharedState {
    size_t numChannels;
    std::unique_ptr<std::atomic<float>[]> channelVolumes;
    std::mutex sleepMutex;
    std::condition_variable sleepCv;
    std::atomic<bool> shouldSleep{true};

    MixerSharedState(size_t numChannels)
        : numChannels(numChannels),
          channelVolumes(std::make_unique<std::atomic<float>[]>(numChannels)) {
        for (size_t i = 0; i < numChannels; ++i)
            channelVolumes[i].store(0.0f); // Default volume
    }
    MixerSharedState() = delete;
    MixerSharedState(const MixerSharedState &) = delete;
    MixerSharedState &operator=(const MixerSharedState &) = delete;
};
