#include "AppHost/AppHost.h"

#include <chrono>
#include <thread>

AppHost::AppHost(const AppConfig &config, std::atomic<bool> &isRunning)
    : m_appConfig(config),
      m_mixerSharedState(m_appConfig.audioConfig.num_channels, m_appConfig.audioConfig.mute_buttons),
      m_isRunning(isRunning),
      m_serialListener(m_isRunning, m_appConfig.serialConfig, m_mixerSharedState),
      m_audioDriver(m_isRunning, m_appConfig.audioConfig, m_mixerSharedState) {}

void AppHost::run() {
    m_serialWorker = std::jthread([this] { m_serialListener.run(); });
    m_audioWorker = std::jthread([this] { m_audioDriver.run(); });
}