#include "SerialReader.h"

#include <chrono>
#include <iostream>
#include <sstream>

SerialReader::SerialReader(const std::string &port, unsigned int baudRate)
    : m_portName(port), m_baudRate(baudRate), m_serialPort(m_ioService) {
    m_syncTimer = std::make_unique<boost::asio::steady_timer>(m_ioService);
}

SerialReader::~SerialReader() { stop(); }

bool SerialReader::start() {
    if (m_running) {
        return true;
    }

    m_running = true;
    m_workerThread = std::jthread(&SerialReader::workerThread, this);
    return true;
}

void SerialReader::stop() {
    if (!m_running) {
        return;
    }

    m_running = false;
    if (m_connected) {
        closePort();
    }
    m_ioService.stop();
}

bool SerialReader::openPort() {
    try {
        m_serialPort.open(m_portName);

        // Configure serial port
        m_serialPort.set_option(
            boost::asio::serial_port_base::baud_rate(m_baudRate));
        m_serialPort.set_option(
            boost::asio::serial_port_base::character_size(8));
        m_serialPort.set_option(boost::asio::serial_port_base::parity(
            boost::asio::serial_port_base::parity::none));
        m_serialPort.set_option(boost::asio::serial_port_base::stop_bits(
            boost::asio::serial_port_base::stop_bits::one));

        std::cout << "Serial port opened: " << m_portName << std::endl;
        m_connected = true;

        scheduleSyncTimer();
    } catch (const std::exception &e) {
        std::cerr << "Error opening port: " << e.what() << std::endl;
        return false;
    }
    return true;
}

bool SerialReader::closePort() {
    if (m_serialPort.is_open()) {
        try {
            m_serialPort.cancel();
            m_serialPort.close();
        } catch (const std::exception &e) {
            std::cerr << "Error closing port: " << e.what() << std::endl;
            return false;
        }
    }
    m_connected = false;
    return true;
}

void SerialReader::sendMessage(const std::string &message,
                               std::function<void(bool success)> callback) {
    if (!m_connected || !m_serialPort.is_open()) {
        if (callback) {
            callback(false);
        }
        return;
    }

    // Create a shared_ptr to keep the message alive during async operation
    auto messageBuffer = std::make_shared<std::string>(message);

    boost::asio::async_write(
        m_serialPort, boost::asio::buffer(*messageBuffer),
        [this, messageBuffer, callback](const boost::system::error_code &error,
                                        std::size_t bytes_transferred) {
            if (error) {
                std::cerr << "Error sending message: " << error.message()
                          << std::endl;
                if (callback) {
                    callback(false);
                }
                return;
            }
            if (callback) {
                callback(true);
            }
        });
}

void SerialReader::sendSyncMessage() {
    if (m_connected && m_running) {
        sendMessage(m_syncMessage, [this](bool success) {
            if (!success) {
                std::cerr << "Failed to send sync message." << std::endl;
                m_connected = false;
                closePort();
                m_ioService.restart();
                return;
            }
            scheduleSyncTimer();
        });
    }
}

void SerialReader::scheduleSyncTimer() {
    if (!m_running || !m_connected) {
        return;
    }

    m_syncTimer->expires_after(std::chrono::milliseconds(SYNC_INTERVAL_MS));
    m_syncTimer->async_wait([this](const boost::system::error_code &error) {
        if (!error && m_running && m_connected) {
            sendSyncMessage();
        }
    });
}

void SerialReader::readStart() {
    if (!m_connected || !m_running) {
        return;
    }

    boost::asio::async_read_until(m_serialPort, m_readBuffer, '\n',
                                  std::bind(&SerialReader::readComplete, this,
                                            std::placeholders::_1,
                                            std::placeholders::_2));
}

void SerialReader::readComplete(const boost::system::error_code &error,
                                size_t bytesTransferred) {
    if (!m_running) {
        return;
    }

    if (!error) {
        // Read the line from the buffer
        std::istream is(&m_readBuffer);
        std::string line;
        std::getline(is, line);

        // Parse comma-separated values
        std::vector<int> values;
        std::string item;
        std::istringstream iss(line);

        while (std::getline(iss, item, ',')) {
            try {
                int value = std::stoi(item);
                values.push_back(value);
            } catch (const std::exception &) {
                // Skip invalid values
            }
        }

        // Invoke callback with parsed values
        if (m_callback) {
            m_callback(values);
        }

        // Continue reading
        readStart();
    } else {
        std::cerr << "Error reading from serial port: " << error.message()
                  << std::endl;
        closePort();
        m_ioService.restart();
    }
}

void SerialReader::workerThread() {
    while (m_running) {
        if (!m_connected) {
            if (openPort()) {
                readStart();
                m_ioService.run();
                m_ioService.restart();
            } else {
                // Wait before reconnecting
                std::this_thread::sleep_for(
                    std::chrono::milliseconds(RECONNECT_INTERVAL_MS));
            }
        } else {
            // Small sleep to prevent CPU hogging
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
}