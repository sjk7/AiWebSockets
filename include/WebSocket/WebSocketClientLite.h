#pragma once

#include "Socket.h"
#include <memory>
#include <functional>
#include <string>

namespace nob {

// Type aliases for cleaner code
using MessageReceiveResult = std::pair<Result, std::string>;

class WebSocketClientLite {
private:
    std::unique_ptr<Socket> m_socket;
    std::string m_serverHost;
    uint16_t m_serverPort;
    bool m_connected;
    
    // Callbacks
    std::function<void(const std::string&)> m_onMessage;
    std::function<void()> m_onConnect;
    std::function<void()> m_onDisconnect;
    std::function<void(const Result&)> m_onError;

public:
    // Constructor
    WebSocketClientLite(const std::string& host = "127.0.0.1", uint16_t port = 8080);
    
    // Destructor
    ~WebSocketClientLite();
    
    // Configuration
    WebSocketClientLite& setServer(const std::string& host, uint16_t port);
    
    // Callback registration
    WebSocketClientLite& onMessage(const std::function<void(const std::string&)>& callback);
    WebSocketClientLite& onConnect(const std::function<void()>& callback);
    WebSocketClientLite& onDisconnect(const std::function<void()>& callback);
    WebSocketClientLite& onError(const std::function<void(const Result&)>& callback);
    
    // Connection control
    Result connect();
    Result disconnect();
    bool isConnected() const { return m_connected; }
    
    // Message sending
    Result sendMessage(const std::string& message);
    Result sendBinary(const std::vector<uint8_t>& data);
    
    // Message receiving (blocking)
    std::pair<Result, std::string> receiveMessage();
    
    // Message receiving (non-blocking)
    void processMessages(); // Call this regularly to receive messages
    
    // Get connection info
    std::string getServerHost() const { return m_serverHost; }
    uint16_t getServerPort() const { return m_serverPort; }

private:
    Result performWebSocketHandshake();
    Result sendWebSocketFrame(const std::vector<uint8_t>& data, int opcode);
};

} // namespace nob
