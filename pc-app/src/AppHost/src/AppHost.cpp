#include "AppHost/AppHost.h"

#include <chrono>
#include <thread>

AppHost::AppHost(const AppConfig &config)
    : m_appConfig(config),
      m_mixerSharedState(m_appConfig.audioConfig.num_channels),
      m_serialListener(m_appConfig.serialConfig, m_mixerSharedState),
      m_audioDriver(m_appConfig.audioConfig) {}

void AppHost::run() {
    m_serialWorker = std::jthread([this] { m_serialListener.run(m_isRunning); });
    m_audioWorker = std::jthread([this] { m_audioDriver.run(m_isRunning); });
}