#include "SerialListener/SerialListener.h"

#include <iostream>
#include <sstream>
#include <stdexcept>

SerialListener::SerialListener(std::atomic<bool> &isRunning,
                               const SerialConfig &config,
                               MixerSharedState &sharedState)
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
    if (m_serialPort.is_open()) {
        m_serialPort.close();
    }
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
            m_sharedState.version.fetch_add(1, std::memory_order_release);
            m_sharedState.version.notify_all();
            return;
        }
        std::cout << "[SERIAL] Attempting to reconnect..." << std::endl;
        if (openPort()) {
            m_reconnectAttempts = 0;
            onRead();
        } else {
            ++m_reconnectAttempts;
            std::cerr << "[SERIAL] Reconnect attempt " << m_reconnectAttempts
                      << " failed." << std::endl;
            scheduleReconnect();
        }
    });
}

void SerialListener::run() {

    m_sharedState.comReady.wait(false, std::memory_order_acquire);

    if (m_serialPort.is_open()) {
        onRead();
    }
    while (m_isRunning.load(std::memory_order_acquire)) {
        try {
            m_ioContext.run();

            // Fallback in case broken callback chain stop without an error
            if (m_isRunning.load(std::memory_order_acquire)) {
                std::cerr
                    << "[SERIAL] io_context stopped unexpectedly. Attempting "
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
            scheduleReconnect();
        }
    }
}

void SerialListener::onRead() {
    boost::asio::async_read_until(m_serialPort, m_readBuffer, '\n',
                                  [this](const boost::system::error_code &error,
                                         size_t bytesTransferred) {
                                      onMessageRecieved(error,
                                                        bytesTransferred);
                                  });
}

void SerialListener::onMessageRecieved(const boost::system::error_code &error,
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

    std::cout << "[SERIAL] Received line: " << line << std::endl;

    // Parse the line as csv
    std::istringstream ss(line);

    bool result =
        m_sharedState.muteButtons ? handleDataWithMute(ss) : handleData(ss);

    if (!result) {
        std::cerr << "[SERIAL] Failed to handle data." << std::endl;
    }

    // Continue reading for the next line
    onRead();
}

bool SerialListener::handleData(std::istringstream &ss) {
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
            float volume = 0.0f;
            if (m_serialConfig.invert_sliders) {
                volume = 1.0f - static_cast<float>(values[i]) / 1023.0f;
            } else {
                volume = static_cast<float>(values[i]) / 1023.0f;
            }
            m_sharedState.channelVolumes[i].store(volume,
                                                  std::memory_order_relaxed);
        }

        // Wake up audio thread
        m_sharedState.version.fetch_add(1, std::memory_order_release);
        m_sharedState.version.notify_all();
        return true;
    }

    return false;
}

bool SerialListener::handleDataWithMute(std::istringstream &ss) {
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
    if (values.size() == m_sharedState.numChannels * 2) {
        for (size_t i = 0; i < m_sharedState.numChannels; ++i) {
            float volume = 0.0f;
            if (m_serialConfig.invert_sliders) {
                volume = 1.0f - static_cast<float>(values[i]) / 1023.0f;
            } else {
                volume = static_cast<float>(values[i]) / 1023.0f;
            }
            bool mute = values[i + m_sharedState.numChannels] != 0;

            m_sharedState.channelVolumes[i].store(volume,
                                                  std::memory_order_relaxed);
            m_sharedState.channelMuteStates[i].store(mute,
                                                     std::memory_order_relaxed);
        }

        // Wake up audio thread
        m_sharedState.version.fetch_add(1, std::memory_order_release);
        m_sharedState.version.notify_all();
        return true;
    }

    return false;
}