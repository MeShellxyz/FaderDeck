#include "SerialListener/SerialListener.h"

#include <iostream>
#include <sstream>
#include <stdexcept>

SerialListener::SerialListener(const SerialConfig &config,
                               MixerSharedState &sharedState,
                               std::atomic<bool> &isRunning)
    : m_serialConfig(config),
      m_sharedState(sharedState),
      m_isRunning(isRunning),
      m_ioContext(),
      m_serialPort(m_ioContext),
      m_reconnectTimer(m_ioContext) {
    if (!openPort()) {
        std::cerr << "[SERIAL] Failed to open port on initialization. Will "
                     "attempt to reconnect."
                  << std::endl;
        scheduleReconnect();
    }
}

SerialListener::~SerialListener() {
    m_serialPort.close();
    m_ioContext.stop();
}

bool SerialListener::openPort() {
    try {
        m_serialPort.open(m_serialConfig.com_port);
        m_serialPort.set_option(
            boost::asio::serial_port_base::baud_rate(m_serialConfig.baud_rate));
        m_serialPort.set_option(
            boost::asio::serial_port_base::character_size(8));
        m_serialPort.set_option(boost::asio::serial_port_base::parity(
            boost::asio::serial_port_base::parity::none));
        m_serialPort.set_option(boost::asio::serial_port_base::stop_bits(
            boost::asio::serial_port_base::stop_bits::one));

        std::cout << "[SERIAL] Port opened: " << m_serialConfig.com_port
                  << " at " << m_serialConfig.baud_rate << " baud."
                  << std::endl;
        return true;
    } catch (const std::exception &e) {
        std::cerr << "[SERIAL] Error opening port: " << e.what() << std::endl;
        return false;
    }
}

void SerialListener::scheduleReconnect() {
    m_reconnectTimer.expires_after(std::chrono::seconds(5));
    m_reconnectTimer.async_wait([this](const boost::system::error_code &error) {
        if (error) return;

        if (m_reconnectAttempts >= m_maxReconnectAttempts) {
            std::cerr << "[SERIAL] Max reconnect attempts reached. Will not "
                         "attempt further reconnects."
                      << std::endl;

            m_isRunning.store(false, std::memory_order_release);
            m_sharedState.sleepCv.notify_all();
            this->stop();
            return;
        }
        std::cout << "[SERIAL] Attempting to reconnect..." << std::endl;
        if (openPort()) {
            m_reconnectAttempts = 0;
            doRead();
        } else {
            ++m_reconnectAttempts;
            std::cerr << "[SERIAL] Reconnect attempt " << m_reconnectAttempts
                      << " failed." << std::endl;
            scheduleReconnect(isRunning);
        }
    });
}

void SerialListener::run() {
    if (m_serialPort.is_open()) {
        doRead();
    }
    while (m_isRunning.load(std::memory_order_acquire)) {
        try {
            m_ioContext.run();
            if (m_isRunning.load(std::memory_order_acquire)) {
                std::cerr << "[SERIAL] io_context stopped unexpectedly. Attempting "
                             "to reconnect."
                          << std::endl;
                m_serialPort.close();
                m_ioContext.restart();
                scheduleReconnect();
            }
        } catch (const std::exception &e) {
            std::cerr << "[SERIAL] Exception in io_context: " << e.what()
                      << std::endl;
            m_serialPort.close();
            m_ioContext.restart();
            scheduleReconnect(isRunning);
        }
    }
}

void SerialListener::stop() {
    if (m_serialPort.is_open()) {
        m_serialPort.close();
    }
    m_ioContext.stop();
}

void SerialListener::doRead() {
    boost::asio::async_read_until(m_serialPort, m_readBuffer, '\n',
                                  [this](const boost::system::error_code &error,
                                         size_t bytesTransferred) {
                                      handleRead(error, bytesTransferred);
                                  });
}

void SerialListener::handleRead(const boost::system::error_code &error,
                                size_t bytesTransferred) {
    if (error) {
        std::cerr << "[SERIAL] Read error: " << error.message() << std::endl;
        m_serialPort.close();
        m_readBuffer.consume(bytesTransferred);
        m_ioContext.restart();
        scheduleReconnect();
        return;
    }

    // Extract the line from the buffer
    std::istream is(&m_readBuffer);
    std::string line;
    std::getline(is, line);

    // Parse the line as csv
    std::stringstream ss(line);
    std::string token;

    std::vector<int> values;
    while (std::getline(ss, token, ',')) {
        try {
            values.push_back(std::stoi(token));
        } catch (const std::exception &e) {
            std::cerr << "[SERIAL] Failed to parse value: " << token
                      << " Error: " << e.what() << std::endl;
            break;
        }
    }

    // Update shared state if we got the expected number of values
    if (values.size() == m_sharedState.numChannels) {
        for (size_t i = 0; i < values.size(); ++i) {
            float volume = static_cast<float>(values[i]) / 1023.0f;
            m_sharedState.channelVolumes[i].store(volume,
                                                  std::memory_order_relaxed);
        }

        // Wake up audio thread
        m_sharedState.shouldSleep.store(false, std::memory_order_release);
        m_sharedState.sleepCv.notify_all();
    } else {
        std::cerr << "[SERIAL] Received " << values.size()
                  << " values, expected " << m_sharedState.numChannels
                  << std::endl;
    }

    // Continue reading for the next line
    doRead();
}