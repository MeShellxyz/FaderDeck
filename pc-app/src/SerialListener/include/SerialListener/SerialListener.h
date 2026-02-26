#pragma once

#include "Config/AppConfig.h"
#include "Core/MixerSharedState.h"

#include <boost/asio.hpp>
#include <memory>

class SerialListener {
public:
    SerialListener(const SerialConfig &config, MixerSharedState &sharedState);
    ~SerialListener();

    // Prevent copying, allow moving
    SerialListener(const SerialListener &) = delete;
    SerialListener &operator=(const SerialListener &) = delete;
    SerialListener(SerialListener &&) = delete;
    SerialListener &operator=(SerialListener &&) = delete;

    void run();
    void stop();

private:
    void doRead();
    void handleRead(const boost::system::error_code &error,
                    size_t bytesTransferred);

    bool openPort();
    void scheduleReconnect();

    const SerialConfig &m_serialConfig;
    MixerSharedState &m_sharedState;
    std::atomic<bool> &m_isRunning;

    boost::asio::io_context m_ioContext;
    boost::asio::serial_port m_serialPort;
    boost::asio::streambuf m_readBuffer;
    boost::asio::steady_timer m_reconnectTimer;

    int m_reconnectAttempts{0};
    const int m_maxReconnectAttempts{20};
};