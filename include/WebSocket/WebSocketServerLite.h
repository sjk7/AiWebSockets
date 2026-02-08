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
    WebSocketServerLite& SetPort(uint16_t port);
    WebSocketServerLite& SetBindAddress(const std::string& address);
    WebSocketServerLite& EnableSecurity(bool enabled = true);
    WebSocketServerLite& SetMaxConnections(int maxConnections);
    WebSocketServerLite& SetMaxConnectionsPerIP(int maxPerIP);
    WebSocketServerLite& SetMaxConnectionsPerMinute(int maxPerMinute);
    
    // Callback registration
    WebSocketServerLite& OnMessage(const std::function<void(const std::string&)>& callback);
    WebSocketServerLite& OnConnect(const std::function<void(const std::string&)>& callback);
    WebSocketServerLite& OnDisconnect(const std::function<void(const std::string&)>& callback);
    WebSocketServerLite& OnError(const std::function<void(const Result&)>& callback);
    
    // Server control
    Result Start();
    Result Stop();
    bool IsRunning() const { return m_running; }
    
    // Non-blocking event processing
    Result StartNonBlocking();
    void ProcessEvents(); // Call this regularly in your main loop
    
    // Get server info
    uint16_t GetPort() const { return m_port; }
    std::string GetBindAddress() const { return m_bindAddress; }
    int GetCurrentConnectionCount() const;

private:
    // Internal methods
    Result InitializeServer();
    void HandleClientConnection(std::unique_ptr<Socket> clientSocket);
    bool ValidateHTTPRequest(const std::string& request);
    Result PerformWebSocketHandshake(Socket& clientSocket, const std::string& request);
    void SendHTTPResponse(Socket& clientSocket, const std::string& status, const std::string& contentType, const std::string& body);
    std::string GetClientIP(const Socket& socket, const std::string& httpRequest = "");
    
    // Security methods
    bool IsConnectionAllowed(const std::string& clientIP);
    void RemoveConnection(const std::string& clientIP);
};

} // namespace WebSocket
