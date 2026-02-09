#pragma once

#include "Socket.h"
#include <memory>
#include <functional>
#include <string>
#include <map>
#include <mutex>
#include <atomic>

namespace WebSocket {

// Connection tracking for security
struct ConnectionInfo {
    std::chrono::steady_clock::time_point lastConnectionTime;
    int currentConnections = 0;
    int connectionsPerMinute = 0;
    std::chrono::steady_clock::time_point minuteStart;
};

class WebSocketServerLite {
private:
    std::unique_ptr<Socket> m_serverSocket;
    std::string m_bindAddress;
    uint16_t m_port;
    bool m_running;
    bool m_securityEnabled;
    
    // Security settings
    int m_maxConnections;
    int m_maxConnectionsPerIP;
    int m_maxConnectionsPerMinute;
    
    // Connection tracking
    std::map<std::string, ConnectionInfo> ipConnectionMap;
    std::atomic<int> currentConnections{0};
    mutable std::mutex ipConnectionMutex;
    
    // Callbacks
    std::function<void(const std::string&)> m_onMessage;
    std::function<void(const std::string&)> m_onConnect;
    std::function<void(const std::string&)> m_onDisconnect;
    std::function<void(const Result&)> m_onError;

public:
    // Constructor with sensible defaults
    WebSocketServerLite(uint16_t port = 8080, const std::string& bindAddress = "127.0.0.1");
    
    // Destructor
    ~WebSocketServerLite();
    
    // Configuration methods
    WebSocketServerLite& setPort(uint16_t port);
    WebSocketServerLite& setBindAddress(const std::string& address);
    WebSocketServerLite& enableSecurity(bool enabled = true);
    WebSocketServerLite& setMaxConnections(int maxConnections);
    WebSocketServerLite& setMaxConnectionsPerIP(int maxPerIP);
    WebSocketServerLite& setMaxConnectionsPerMinute(int maxPerMinute);
    
    // Callback registration
    WebSocketServerLite& onMessage(const std::function<void(const std::string&)>& callback);
    WebSocketServerLite& onConnect(const std::function<void(const std::string&)>& callback);
    WebSocketServerLite& onDisconnect(const std::function<void(const std::string&)>& callback);
    WebSocketServerLite& onError(const std::function<void(const Result&)>& callback);
    
    // Server control
    Result start();
    Result stop();
    bool isRunning() const { return m_running; }
    
    // Non-blocking event processing
    Result startNonBlocking();
    void processEvents(); // Call this regularly in your main loop
    
    // Get server info
    uint16_t getPort() const { return m_port; }
    std::string getBindAddress() const { return m_bindAddress; }
    int getCurrentConnectionCount() const;

private:
    // Internal methods
    Result initializeServer();
    void handleClientConnection(std::unique_ptr<Socket> clientSocket);
    bool isHTTPRequestValid(const std::string& request);
    Result performWebSocketHandshake(Socket& clientSocket, const std::string& request);
    void sendHTTPResponse(Socket& clientSocket, const std::string& status, const std::string& contentType, const std::string& body);
    std::string getClientIP(const Socket& socket, const std::string& httpRequest = "");
    
    // Security methods
    bool isConnectionAllowed(const std::string& clientIP);
    void removeConnection(const std::string& clientIP);
};

} // namespace WebSocket
