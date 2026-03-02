#include "AppHost/AppHost.h"

#include <chrono>
#include <thread>

AppHost::AppHost(const AppConfig &config)
    : m_appConfig(config),
      m_mixerSharedState(m_appConfig.audioConfig.num_channels, m_appConfig.audioConfig.mute_buttons),
      m_serialListener(m_isRunning, m_appConfig.serialConfig, m_mixerSharedState),
      m_audioDriver(m_isRunning, m_appConfig.audioConfig, m_mixerSharedState) {}

void AppHost::run() {
    m_isRunning.store(true, std::memory_order_release);
    m_serialWorker = std::jthread([this] { m_serialListener.run(); });
    m_audioWorker = std::jthread([this] { m_audioDriver.run(); });
}