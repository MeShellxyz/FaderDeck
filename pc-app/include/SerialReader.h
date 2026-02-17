#pragma once

#include <atomic>
#include <boost/asio.hpp>
#include <functional>
#include <memory>
#include <string>
#include <thread>
#include <vector>

// Define callback type for serial input processing
using SerialInputCallback = std::function<void(const std::vector<int> &)>;

class SerialReader {
public:
    SerialReader(const std::string &port, unsigned int baudRate);
    ~SerialReader();

    bool start();
    void stop();
    bool isConnected() const { return m_connected; }

    void setCallback(SerialInputCallback callback) {
        m_callback = std::move(callback);
    }
    void setSyncMessage(const std::string &syncMsg) { m_syncMessage = syncMsg; }

private:
    // Constants
    static constexpr unsigned int RECONNECT_INTERVAL_MS = 1000;
    static constexpr unsigned int SYNC_INTERVAL_MS = 1000;

    // Port operations
    bool openPort();
    bool closePort();

    // Message handling
    void sendMessage(const std::string &message,
                     std::function<void(bool success)> callback = nullptr);
    void sendSyncMessage();
    void scheduleSyncTimer();

    // Reading operations
    void readStart();
    void readComplete(const boost::system::error_code &error,
                      size_t bytesTransferred);

    // Thread management
    void workerThread();

    // Configuration
    std::string m_portName;
    unsigned int m_baudRate;
    std::string m_syncMessage;
    SerialInputCallback m_callback;

    // Boost ASIO objects
    boost::asio::io_context m_ioService;
    boost::asio::serial_port m_serialPort;
    boost::asio::streambuf m_readBuffer;
    std::unique_ptr<boost::asio::steady_timer> m_syncTimer;

    // Thread and state
    std::jthread m_workerThread;
    std::atomic<bool> m_running{false};
    std::atomic<bool> m_connected{false};
};