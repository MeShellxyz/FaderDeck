#pragma once

#include "Core/MixerSharedState.h"
#include "Config/AppConfig.h"
#include "Config/ConfigStore.h"
#include "SerialListener/SerialListener.h"
#include "AudioDriver/AudioDriver.h"

#include <atomic>
#include <thread>

class AppHost
{
public:
    AppHost(const AppConfig &config);
    ~AppHost() = default;

    AppHost(const AppHost &) = delete;
    AppHost &operator=(const AppHost &) = delete;
    AppHost(AppHost &&) noexcept;
    AppHost &operator=(AppHost &&) noexcept;

    void run();

private:

    const AppConfig &m_appConfig;

    MixerSharedState m_mixerSharedState;
    SerialListener m_serialListener;
    AudioDriver m_audioDriver;

    std::jthread m_serialWorker;
    std::jthread m_audioWorker;

    std::atomic<bool> m_isRunning{false};

};