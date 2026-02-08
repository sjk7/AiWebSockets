#pragma once

#include "Socket.h"
#include "Types.h"
#include "WebSocketProtocol.h"
#include <string>
#include <functional>
#include <memory>
#include <vector>
#include <thread>
#include <mutex>
#include <atomic>
#include <map>

namespace WebSocket {

// Forward declarations
struct HTTPRequest;
struct ClientConnection;

/**
 * @brief Security configuration for the HTTP server
 */
struct SecurityConfig {
    // Connection limits
    int maxConnectionsPerIP = 10;           // Max connections from single IP
    int maxConnectionsTotal = 100;          // Max total connections
    int maxRequestsPerIP = 1000;            // Max requests per IP per time window
    
    // Rate limiting
    int requestResetPeriodSeconds = 60;      // Time window for request counting
    int connectionTimeoutSeconds = 300;     // Connection timeout (5 minutes)
    
    // Request/Message size limits
    size_t maxRequestSize = 1024 * 1024;     // 1MB max request size
    size_t maxMessageSize = 1024 * 1024;     // 1MB max message size
    
    // Security features
    bool enableRequestSizeLimit = true;      // Enable request size validation
    bool enableMessageSizeLimit = true;      // Enable message size validation
    bool enableConnectionTimeout = true;     // Enable connection timeout
    bool enableRateLimiting = true;          // Enable rate limiting
    bool enableIPBlocking = true;            // Enable IP blocking
    
    // Blocked IPs (can be managed dynamically)
    std::vector<std::string> blockedIPs;
};

/**
 * @brief HTTP request structure
 */
struct HTTPRequest {
    std::string method;
    std::string path;
    std::string version;
    std::map<std::string, std::string> headers;
    std::string body;
    std::string clientIP;
};

/**
 * @brief Client connection information
 */
struct ClientConnection {
    std::unique_ptr<Socket> socket;
    std::string clientIP;
    std::chrono::steady_clock::time_point connectTime;
    int requestCount = 0;
    bool isWebSocket = false;
};

/**
 * @brief Connection tracking information for security
 */
struct ConnectionInfo {
    std::chrono::steady_clock::time_point firstConnection;
    std::chrono::steady_clock::time_point lastConnection;
    std::chrono::steady_clock::time_point lastActivity;
    std::chrono::steady_clock::time_point requestPeriodStart;
    int currentConnections = 0;
    int requestsThisPeriod = 0;
    int totalRequests = 0;
    bool isWebSocket = false;
};

/**
 * @brief WebSocket message with client IP information
 */
struct WebSocketMessageWithIP {
    WebSocketMessage message;
    std::string clientIP;
    WEBSOCKET_OPCODE opcode;
};

/**
 * @brief HTTP + WebSocket Server with advanced security features
 * 
 * This server provides both HTTP and WebSocket functionality with comprehensive
 * security measures including rate limiting, IP blocking, connection limits,
 * and request size validation.
 */
class HttpWsServer {
private:
    std::unique_ptr<Socket> m_serverSocket;
    std::string m_bindAddress;
    uint16_t m_port;
    bool m_running;
    SecurityConfig m_securityConfig;
    
    // Connection tracking
    std::map<std::string, ConnectionInfo> m_connectionMap;
    std::atomic<int> m_currentConnections{0};
    mutable std::mutex m_connectionMutex;
    
    // Client connections
    std::vector<std::unique_ptr<ClientConnection>> m_clients;
    mutable std::mutex m_clientsMutex;
    
    // Callbacks
    std::function<std::string(const HTTPRequest&)> m_onHttpRequest;
    std::function<std::string(const WebSocketMessageWithIP&)> m_onWebSocketMessage;
    std::function<void(const std::string&)> m_onConnect;
    std::function<void(const std::string&)> m_onDisconnect;
    std::function<void(const std::string&, const std::string&)> m_onSecurityViolation;
    std::function<void(const std::string&)> m_onError;
    
    // Server thread
    std::unique_ptr<std::thread> m_serverThread;
    std::atomic<bool> m_shouldStop{false};

public:
    // Constructor
    explicit HttpWsServer(uint16_t port = 8080, 
                         const std::string& bindAddress = "127.0.0.1",
                         const SecurityConfig& config = SecurityConfig{});
    
    // Destructor
    ~HttpWsServer();
    
    // Configuration methods
    HttpWsServer& SetPort(uint16_t port);
    HttpWsServer& SetBindAddress(const std::string& address);
    HttpWsServer& SetSecurityConfig(const SecurityConfig& config);
    
    // Callback registration
    HttpWsServer& OnHttpRequest(const std::function<std::string(const HTTPRequest&)>& callback);
    HttpWsServer& OnWebSocketMessage(const std::function<std::string(const WebSocketMessageWithIP&)>& callback);
    HttpWsServer& OnConnect(const std::function<void(const std::string&)>& callback);
    HttpWsServer& OnDisconnect(const std::function<void(const std::string&)>& callback);
    HttpWsServer& OnSecurityViolation(const std::function<void(const std::string&, const std::string&)>& callback);
    HttpWsServer& OnError(const std::function<void(const std::string&)>& callback);
    
    // Server control
    Result Start();
    Result Stop();
    bool IsRunning() const { return m_running; }
    
    // Server info
    uint16_t GetPort() const { return m_port; }
    std::string GetBindAddress() const { return m_bindAddress; }
    int GetCurrentConnectionCount() const;
    std::vector<std::string> GetConnectedIPs() const;
    
    // Security management
    void BlockIP(const std::string& ip);
    void UnblockIP(const std::string& ip);
    std::vector<std::string> GetBlockedIPs() const;
    SecurityConfig GetSecurityConfig() const { return m_securityConfig; }

private:
    // Internal methods
    void ServerLoop();
    void HandleClient(std::unique_ptr<ClientConnection> client);
    void HandleHTTPRequest(ClientConnection* client, const std::string& request);
    void HandleWebSocketConnection(ClientConnection* client, const std::string& request);
    void SendHTTPResponse(ClientConnection* client, const std::string& status, 
                         const std::string& contentType, const std::string& body);
    void SendHTTPResponseSync(ClientConnection* client, const std::string& status, 
                             const std::string& contentType, const std::string& body);
    
    // Security methods
    bool IsIPBlocked(const std::string& ip) const;
    bool IsConnectionAllowed(const std::string& ip);
    bool IsRequestSizeValid(const std::string& request, const std::string& clientIP) const;
    bool IsMessageSizeValid(const std::string& message, const std::string& clientIP) const;
    void UpdateConnectionInfo(const std::string& ip, bool isWebSocket = false);
    void RemoveConnection(const std::string& ip);
    void CleanupStaleConnections();
    
    // Utility methods
    std::string GetClientIP(const Socket& socket);
    HTTPRequest ParseHTTPRequest(const std::string& request, const std::string& clientIP);
    bool IsWebSocketUpgrade(const std::string& request) const;
    std::string GenerateHTTPResponse(const std::string& status, const std::string& contentType, const std::string& body);
};

} // namespace WebSocket
