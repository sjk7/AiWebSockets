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
 * @brief Protection configuration for the HTTP/WebSocket server
 */
struct ProtectionConfig {
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
    ProtectionConfig m_protectionConfig;
    
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
    std::function<void(const std::string&, const std::string&)> m_onProtectionViolation;
    std::function<void(const std::string&)> m_onError;
    
    // Server thread
    std::unique_ptr<std::thread> m_serverThread;
    std::atomic<bool> m_shouldStop{false};

public:
    // Constructor
    explicit HttpWsServer(uint16_t port = 8080, 
                         const std::string& bindAddress = "127.0.0.1",
                         const ProtectionConfig& config = ProtectionConfig{});
    
    // Destructor
    ~HttpWsServer();
    
    // Configuration methods
    HttpWsServer& setPort(uint16_t port);
    HttpWsServer& setBindAddress(const std::string& address);
    HttpWsServer& setProtectionConfig(const ProtectionConfig& config);
    
    // Callback registration
    HttpWsServer& onHttpRequest(const std::function<std::string(const HTTPRequest&)>& callback);
    HttpWsServer& onWebSocketMessage(const std::function<std::string(const WebSocketMessageWithIP&)>& callback);
    HttpWsServer& onConnect(const std::function<void(const std::string&)>& callback);
    HttpWsServer& onDisconnect(const std::function<void(const std::string&)>& callback);
    HttpWsServer& onProtectionViolation(const std::function<void(const std::string&, const std::string&)>& callback);
    HttpWsServer& onError(const std::function<void(const std::string&)>& callback);
    
    // Server control
    Result start();
    Result stop();
    bool isRunning() const { return m_running; }
    
    // Server info
    uint16_t getPort() const { return m_port; }
    std::string getBindAddress() const { return m_bindAddress; }
    int getCurrentConnectionCount() const;
    std::vector<std::string> getConnectedIPs() const;
    
    // Protection management
    void blockIP(const std::string& ip);
    void unblockIP(const std::string& ip);
    std::vector<std::string> getBlockedIPs() const;
    ProtectionConfig getProtectionConfig() const { return m_protectionConfig; }

private:
    // Internal methods
    void serverLoop();
    void handleClient(std::unique_ptr<ClientConnection> client);
    void handleHTTPRequest(ClientConnection* client, const std::string& request);
    void handleWebSocketConnection(ClientConnection* client, const std::string& request);
    void sendHTTPResponse(ClientConnection* client, const std::string& status, 
                         const std::string& contentType, const std::string& body);
    void sendHTTPResponseSync(ClientConnection* client, const std::string& status, 
                             const std::string& contentType, const std::string& body);
    
    // Protection methods
    bool isIPBlocked(const std::string& ip) const;
    bool isConnectionAllowed(const std::string& ip);
    bool isRequestSizeValid(const std::string& request, const std::string& clientIP) const;
    bool isMessageSizeValid(const std::string& message, const std::string& clientIP) const;
    void updateConnectionInfo(const std::string& ip, bool isWebSocket = false);
    void removeConnection(const std::string& ip);
    void cleanupStaleConnections();
    
    // Utility methods
    std::string getClientIP(const Socket& socket);
    HTTPRequest parseHTTPRequest(const std::string& request, const std::string& clientIP);
    bool isWebSocketUpgrade(const std::string& request) const;
    std::string generateHTTPResponse(const std::string& status, const std::string& contentType, const std::string& body);
};

} // namespace WebSocket
