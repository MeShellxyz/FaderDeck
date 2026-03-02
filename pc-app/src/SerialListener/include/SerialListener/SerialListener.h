#pragma once

#include "Config/AppConfig.h"
#include "Core/MixerSharedState.h"

#include <boost/asio.hpp>
#include <sstream>
#include <memory>

class SerialListener {
public:
    SerialListener(std::atomic<bool> &isRunning,
                   const SerialConfig &config, MixerSharedState &sharedState);
    ~SerialListener();

    // Prevent copying, allow moving
    SerialListener(const SerialListener &) = delete;
    SerialListener &operator=(const SerialListener &) = delete;
    SerialListener(SerialListener &&) = delete;
    SerialListener &operator=(SerialListener &&) = delete;

    void run();

private:
    void onRead();
    void onMessageRecieved(const boost::system::error_code &error,
                    size_t bytesTransferred);

    bool handleData(std::istringstream &ss);
    bool handleDataWithMute(std::istringstream &ss);

    bool openPort();
    void scheduleReconnect();

    std::atomic<bool> &m_isRunning;
    const SerialConfig &m_serialConfig;
    MixerSharedState &m_sharedState;

    boost::asio::io_context m_ioContext;
    boost::asio::serial_port m_serialPort;
    boost::asio::streambuf m_readBuffer;
    boost::asio::steady_timer m_reconnectTimer;

    int m_reconnectAttempts{0};
    const int m_maxReconnectAttempts{20};
};