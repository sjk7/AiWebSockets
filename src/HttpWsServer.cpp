#include "WebSocket/HttpWsServer.h"
#include <sstream>
#include <algorithm>

namespace WebSocket {

HttpWsServer::HttpWsServer(uint16_t port, 
                           const std::string& bindAddress,
                           const protectionConfig& config)
    : m_bindAddress(bindAddress), m_port(port), m_running(false), m_protectionConfig(config) {
}

HttpWsServer::~HttpWsServer() {
    stop();
}

HttpWsServer& HttpWsServer::setPort(uint16_t port) {
    m_port = port;
    return *this;
}

HttpWsServer& HttpWsServer::setBindAddress(const std::string& address) {
    m_bindAddress = address;
    return *this;
}

HttpWsServer& HttpWsServer::setprotectionConfig(const protectionConfig& config) {
    m_protectionConfig = config;
    return *this;
}

HttpWsServer& HttpWsServer::onHttpRequest(const std::function<std::string(const HTTPRequest&)>& callback) {
    m_onHttpRequest = callback;
    return *this;
}

HttpWsServer& HttpWsServer::onWebSocketMessage(const std::function<std::string(const WebSocketMessageWithIP&)>& callback) {
    m_onWebSocketMessage = callback;
    return *this;
}

HttpWsServer& HttpWsServer::onConnect(const std::function<void(const std::string&)>& callback) {
    m_onConnect = callback;
    return *this;
}

HttpWsServer& HttpWsServer::onDisconnect(const std::function<void(const std::string&)>& callback) {
    m_onDisconnect = callback;
    return *this;
}

HttpWsServer& HttpWsServer::onProtectionViolation(const std::function<void(const std::string&, const std::string&)>& callback) {
    m_onProtectionViolation = callback;
    return *this;
}

HttpWsServer& HttpWsServer::onError(const std::function<void(const std::string&)>& callback) {
    m_onError = callback;
    return *this;
}

Result HttpWsServer::start() {
    if (m_running) {
        return Result(ErrorCode::unknownError, "Server is already running");
    }
    
    // Create server socket
    m_serverSocket = std::make_unique<Socket>();
    auto createResult = m_serverSocket->create(socketFamily::IPV4, socketType::TCP);
    if (!createResult.isSuccess()) {
        if (m_onError) m_onError("Failed to create server socket: " + createResult.getErrorMessage());
        return createResult;
    }
    
    // Set socket options
    m_serverSocket->reuseAddress(true);
    
    // Bind to address
    auto bindResult = m_serverSocket->bind(m_bindAddress, m_port);
    if (!bindResult.isSuccess()) {
        if (m_onError) m_onError("Failed to bind server socket: " + bindResult.getErrorMessage());
        return bindResult;
    }
    
    // Start listening
    auto listenResult = m_serverSocket->listen(128);
    if (!listenResult.isSuccess()) {
        if (m_onError) m_onError("Failed to listen on server socket: " + listenResult.getErrorMessage());
        return listenResult;
    }
    
    m_running = true;
    m_shouldStop = false;
    
    // Start server thread
    m_serverThread = std::make_unique<std::thread>(&HttpWsServer::serverLoop, this);
    
    return Result();
}

Result HttpWsServer::stop() {
    if (!m_running) {
        return Result();
    }
    
    m_running = false;
    m_shouldStop = true;
    
    // Close server socket to unblock accept
    if (m_serverSocket) {
        m_serverSocket->close();
    }
    
    // Wait for server thread to finish
    if (m_serverThread && m_serverThread->joinable()) {
        m_serverThread->join();
    }
    
    // Close all client connections
    {
        std::lock_guard<std::mutex> lock(m_clientsMutex);
        for (auto& client : m_clients) {
            if (client && client->socket) {
                client->socket->close();
            }
        }
        m_clients.clear();
    }
    
    return Result();
}

int HttpWsServer::getCurrentConnectionCount() const {
    return m_currentConnections.load();
}

std::vector<std::string> HttpWsServer::getConnectedIPs() const {
    std::lock_guard<std::mutex> lock(m_clientsMutex);
    std::vector<std::string> ips;
    for (const auto& client : m_clients) {
        if (client) {
            ips.push_back(client->clientIP);
        }
    }
    return ips;
}

void HttpWsServer::blockIP(const std::string& ip) {
    std::lock_guard<std::mutex> lock(m_connectionMutex);
    if (std::find(m_protectionConfig.blockedIPs.begin(), m_protectionConfig.blockedIPs.end(), ip) == m_protectionConfig.blockedIPs.end()) {
        m_protectionConfig.blockedIPs.push_back(ip);
        
        // Disconnect existing connections from this IP
        std::lock_guard<std::mutex> clientsLock(m_clientsMutex);
        for (auto& client : m_clients) {
            if (client && client->clientIP == ip) {
                client->socket->close();
            }
        }
    }
}

void HttpWsServer::unblockIP(const std::string& ip) {
    std::lock_guard<std::mutex> lock(m_connectionMutex);
    auto it = std::remove(m_protectionConfig.blockedIPs.begin(), m_protectionConfig.blockedIPs.end(), ip);
    m_protectionConfig.blockedIPs.erase(it, m_protectionConfig.blockedIPs.end());
}

std::vector<std::string> HttpWsServer::getBlockedIPs() const {
    std::lock_guard<std::mutex> lock(m_connectionMutex);
    return m_protectionConfig.blockedIPs;
}

void HttpWsServer::serverLoop() {
    while (!m_shouldStop) {
        // Accept new connection
        auto [acceptResult, clientSocket] = m_serverSocket->accept();
        if (!acceptResult.isSuccess() || !clientSocket) {
            if (m_shouldStop) break;
            continue;
        }
        
        // Enable async I/O for better performance
        auto asyncResult = clientSocket->enableAsyncIO();
        if (!asyncResult.isSuccess()) {
            // Log warning but continue with sync mode
            if (m_onError) m_onError("Failed to enable async I/O: " + asyncResult.getErrorMessage());
        }
        
        std::string clientIP = getClientIP(*clientSocket);
        
        // Security checks
        if (!isConnectionAllowed(clientIP)) {
            if (m_onProtectionViolation) {
                m_onProtectionViolation(clientIP, "Connection rejected: Security limits exceeded");
            }
            clientSocket->close();
            continue;
        }
        
        // Create client connection
        auto client = std::make_unique<ClientConnection>();
        client->socket = std::move(clientSocket);
        client->clientIP = clientIP;
        client->connectTime = std::chrono::steady_clock::now();
        
        // Update connection tracking
        updateConnectionInfo(clientIP);
        
        // Handle client in separate thread
        std::thread clientThread(&HttpWsServer::handleClient, this, std::move(client));
        clientThread.detach();
    }
}

void HttpWsServer::handleClient(std::unique_ptr<ClientConnection> client) {
    if (!client || !client->socket) return;
    
    std::string clientIP = client->clientIP;
    
    // Add to clients list
    {
        std::lock_guard<std::mutex> lock(m_clientsMutex);
        m_clients.push_back(std::move(client));
    }
    
    // Notify connection
    if (m_onConnect) {
        m_onConnect(clientIP);
    }
    
    // Receive request with short timeout to prevent hanging
    auto [receiveResult, requestData] = m_clients.back()->socket->receive(m_protectionConfig.maxRequestSize, 1000); // 1 second timeout
    if (!receiveResult.isSuccess() || requestData.empty()) {
        removeConnection(clientIP);
        return;
    }
    
    std::string request(requestData.begin(), requestData.end());
    
    // Validate request size
    if (m_protectionConfig.enableRequestSizeLimit && !isRequestSizeValid(request, clientIP)) {
        if (m_onProtectionViolation) {
            m_onProtectionViolation(clientIP, "Request too large");
        }
        removeConnection(clientIP);
        return;
    }
    
    // Handle based on request type
    if (isWebSocketUpgrade(request)) {
        handleWebSocketConnection(m_clients.back().get(), request);
    } else {
        handleHTTPRequest(m_clients.back().get(), request);
    }
    
    // Remove connection
    removeConnection(clientIP);
}

void HttpWsServer::handleHTTPRequest(ClientConnection* client, const std::string& request) {
    if (!client || !client->socket) return;
    
    HTTPRequest httpRequest = parseHTTPRequest(request, client->clientIP);
    
    std::string response;
    if (m_onHttpRequest) {
        try {
            response = m_onHttpRequest(httpRequest);
        } catch (const std::exception& e) {
            response = generateHTTPResponse("500 Internal Server Error", "text/plain", "Server Error");
            if (m_onError) m_onError("HTTP request handler error: " + std::string(e.what()));
        }
    } else {
        // Default response
        if (httpRequest.path == "/") {
            response = generateHTTPResponse("200 OK", "text/html", 
                "<!DOCTYPE html><html><head><title>Secure HTTP + WebSocket Server</title></head>"
                "<body><h1>Secure HTTP + WebSocket Server</h1>"
                "<p>This server handles both HTTP and WebSocket with security features!</p>"
                "<p>Connected clients: " + std::to_string(getCurrentConnectionCount()) + "</p>"
                "</body></html>");
        } else {
            response = generateHTTPResponse("404 Not Found", "text/plain", "Not Found");
        }
    }
    
    sendHTTPResponse(client, "200 OK", "text/html", response);
}

void HttpWsServer::handleWebSocketConnection(ClientConnection* client, const std::string& request) {
    if (!client || !client->socket) return;
    
    client->isWebSocket = true;
    
    // Perform WebSocket handshake
    HandshakeInfo info;
    auto handshakeResult = WebSocketProtocol::validateHandshakeRequest(request, info);
    
    if (!handshakeResult.isSuccess()) {
        sendHTTPResponse(client, "400 Bad Request", "text/plain", "Invalid WebSocket handshake");
        if (m_onProtectionViolation) {
            m_onProtectionViolation(client->clientIP, "Invalid WebSocket handshake");
        }
        return;
    }
    
    // Send handshake response
    std::string handshakeResponse = WebSocketProtocol::generateHandshakeResponse(info);
    auto sendResult = client->socket->send(std::vector<uint8_t>(handshakeResponse.begin(), handshakeResponse.end()));
    if (!sendResult.isSuccess()) {
        if (m_onError) m_onError("Failed to send WebSocket handshake: " + sendResult.getErrorMessage());
        return;
    }
    
    // Handle WebSocket messages
    while (!m_shouldStop && client->socket && client->socket->isValid()) {
        auto [msgResult, msgData] = client->socket->receive(m_protectionConfig.maxMessageSize);
        if (!msgResult.isSuccess() || msgData.empty()) {
            break;
        }
        
        // Update activity time
        client->connectTime = std::chrono::steady_clock::now();
        
        // Parse WebSocket frame
        WebSocketFrame frame;
        size_t bytesConsumed = 0;
        auto parseResult = WebSocketProtocol::parseFrame(msgData, frame, bytesConsumed);
        
        if (!parseResult.isSuccess()) {
            continue;
        }
        
        // Handle text messages
        if (frame.Opcode == websocketOpcode::TEXT) {
            std::string message(frame.PayloadData.begin(), frame.PayloadData.end());
            
            // Validate message size
            if (m_protectionConfig.enableMessageSizeLimit && !isMessageSizeValid(message, client->clientIP)) {
                if (m_onProtectionViolation) {
                    m_onProtectionViolation(client->clientIP, "WebSocket message too large");
                }
                break;
            }
            
            // Call message handler
            if (m_onWebSocketMessage) {
                try {
                    WebSocketMessage wsMessage{frame.Opcode, std::vector<uint8_t>(message.begin(), message.end())};
                    WebSocketMessageWithIP wsMessageWithIP{wsMessage, client->clientIP, frame.Opcode};
                    std::string response = m_onWebSocketMessage(wsMessageWithIP);
                    
                    if (!response.empty()) {
                        // Send response
                        WebSocketFrame responseFrame = WebSocketProtocol::createTextFrame(response);
                        std::vector<uint8_t> responseData = WebSocketProtocol::generateFrame(responseFrame);
                        client->socket->send(responseData);
                    }
                } catch (const std::exception& e) {
                    if (m_onError) m_onError("WebSocket message handler error: " + std::string(e.what()));
                }
            }
        } else if (frame.Opcode == websocketOpcode::CLOSE) {
            break;
        }
    }
}

void HttpWsServer::sendHTTPResponseSync(ClientConnection* client, const std::string& status, 
                                                    const std::string& contentType, const std::string& body) {
    if (!client || !client->socket) return;
    
    // Generate optimized HTTP response directly as vector
    std::vector<uint8_t> response;
    response.reserve(256 + body.size()); // Pre-allocate
    
    // Add status line
    const char* statusLine = "HTTP/1.1 ";
    response.insert(response.end(), statusLine, statusLine + 9);
    response.insert(response.end(), status.begin(), status.end());
    response.push_back('\r'); response.push_back('\n');
    
    // Add content type
    const char* contentTypeHeader = "Content-Type: ";
    response.insert(response.end(), contentTypeHeader, contentTypeHeader + 14);
    response.insert(response.end(), contentType.begin(), contentType.end());
    const char* charset = "; charset=UTF-8\r\n";
    response.insert(response.end(), charset, charset + 17);
    
    // Add content length
    const char* contentLengthHeader = "Content-Length: ";
    response.insert(response.end(), contentLengthHeader, contentLengthHeader + 16);
    std::string lengthStr = std::to_string(body.length());
    response.insert(response.end(), lengthStr.begin(), lengthStr.end());
    response.push_back('\r'); response.push_back('\n');
    
    // Add connection close
    const char* connectionHeader = "Connection: close\r\n";
    response.insert(response.end(), connectionHeader, connectionHeader + 19);
    
    // Add blank line
    response.push_back('\r'); response.push_back('\n');
    
    // Add body
    response.insert(response.end(), body.begin(), body.end());
    
    // Send synchronously - blocking fallback
    client->socket->send(response);
    
    // For HTTP connections, close after sending response
    // Socket::Close() handles proper shutdown internally
    client->socket->close();
}

void HttpWsServer::sendHTTPResponse(ClientConnection* client, const std::string& status, 
                                               const std::string& contentType, const std::string& body) {
    if (!client || !client->socket) return;
    
    // Enable async I/O for better performance
    if (!client->socket->isAsyncEnabled()) {
        auto asyncResult = client->socket->enableAsyncIO();
        if (!asyncResult.isSuccess()) {
            // Fallback to sync if async fails
            sendHTTPResponseSync(client, status, contentType, body);
            return;
        }
    }
    
    // Generate optimized HTTP response directly as vector
    std::vector<uint8_t> response;
    response.reserve(256 + body.size()); // Pre-allocate
    
    // Add status line
    const char* statusLine = "HTTP/1.1 ";
    response.insert(response.end(), statusLine, statusLine + 9);
    response.insert(response.end(), status.begin(), status.end());
    response.push_back('\r'); response.push_back('\n');
    
    // Add content type
    const char* contentTypeHeader = "Content-Type: ";
    response.insert(response.end(), contentTypeHeader, contentTypeHeader + 14);
    response.insert(response.end(), contentType.begin(), contentType.end());
    const char* charset = "; charset=UTF-8\r\n";
    response.insert(response.end(), charset, charset + 17);
    
    // Add content length
    const char* contentLengthHeader = "Content-Length: ";
    response.insert(response.end(), contentLengthHeader, contentLengthHeader + 16);
    std::string lengthStr = std::to_string(body.length());
    response.insert(response.end(), lengthStr.begin(), lengthStr.end());
    response.push_back('\r'); response.push_back('\n');
    
    // Add connection close for reliability
    const char* connectionHeader = "Connection: close\r\n";
    response.insert(response.end(), connectionHeader, connectionHeader + 19);
    
    // Add blank line
    response.push_back('\r'); response.push_back('\n');
    
    // Add body
    response.insert(response.end(), body.begin(), body.end());
    
    // Send asynchronously - non-blocking
    auto sendResult = client->socket->sendAsync(response);
    if (!sendResult.isSuccess()) {
        // Fallback to sync if async send fails
        sendHTTPResponseSync(client, status, contentType, body);
        return;
    }
    
    // For HTTP connections, close after sending response
    // Socket::Close() handles proper shutdown internally
    client->socket->close();
}

bool HttpWsServer::isIPBlocked(const std::string& ip) const {
    return std::find(m_protectionConfig.blockedIPs.begin(), m_protectionConfig.blockedIPs.end(), ip) != m_protectionConfig.blockedIPs.end();
}

bool HttpWsServer::isConnectionAllowed(const std::string& ip) {
    std::lock_guard<std::mutex> lock(m_connectionMutex);
    
    // Skip security limits for local addresses
    if (ip == "127.0.0.1" || ip == "::1" || ip == "localhost") {
        return true;
    }
    
    // Check if IP is blocked
    if (isIPBlocked(ip)) {
        return false;
    }
    
    // Check total connection limit
    if (m_currentConnections.load() >= m_protectionConfig.maxConnectionsTotal) {
        return false;
    }
    
    // Check per-IP connection limit
    auto it = m_connectionMap.find(ip);
    if (it != m_connectionMap.end()) {
        auto& info = it->second;
        
        // Check current connections from this IP
        if (info.currentConnections >= m_protectionConfig.maxConnectionsPerIP) {
            return false;
        }
        
        // Check request rate limiting
        auto now = std::chrono::steady_clock::now();
        auto timeSincePeriodStart = std::chrono::duration_cast<std::chrono::seconds>(now - info.requestPeriodStart);
        
        // Reset request counter if period has elapsed
        if (timeSincePeriodStart.count() >= m_protectionConfig.requestResetPeriodSeconds) {
            info.requestsThisPeriod = 0;
            info.requestPeriodStart = now;
        }
        
        // Check if this IP has exceeded requests per period
        if (info.requestsThisPeriod >= m_protectionConfig.maxRequestsPerIP) {
            return false;
        }
    }
    
    return true;
}

bool HttpWsServer::isRequestSizeValid(const std::string& request, const std::string& clientIP) const {
    // Skip size validation for local addresses
    if (clientIP == "127.0.0.1" || clientIP == "::1" || clientIP == "localhost") {
        return true;
    }
    return request.size() <= static_cast<size_t>(m_protectionConfig.maxRequestSize);
}

bool HttpWsServer::isMessageSizeValid(const std::string& message, const std::string& clientIP) const {
    // Skip size validation for local addresses
    if (clientIP == "127.0.0.1" || clientIP == "::1" || clientIP == "localhost") {
        return true;
    }
    return message.size() <= static_cast<size_t>(m_protectionConfig.maxMessageSize);
}

void HttpWsServer::updateConnectionInfo(const std::string& ip, bool isWebSocket) {
    // Skip tracking for local addresses (they're trusted)
    if (ip == "127.0.0.1" || ip == "::1" || ip == "localhost") {
        m_currentConnections++;
        return;
    }
    
    std::lock_guard<std::mutex> lock(m_connectionMutex);
    
    auto& info = m_connectionMap[ip];
    auto now = std::chrono::steady_clock::now();
    
    if (info.currentConnections == 0) {
        // First connection from this IP
        info.firstConnection = now;
        info.requestPeriodStart = now;
    }
    
    info.lastConnection = now;
    info.lastActivity = now;
    info.currentConnections++;
    info.requestsThisPeriod++;
    info.totalRequests++;
    info.isWebSocket = isWebSocket;
    
    m_currentConnections++;
}

void HttpWsServer::removeConnection(const std::string& ip) {
    // Skip tracking cleanup for local addresses
    if (ip == "127.0.0.1" || ip == "::1" || ip == "localhost") {
        m_currentConnections--;
        return;
    }
    
    // NOTE: We use find+erase instead of remove_if because:
// 1. std::map keys are const (std::string), cannot be moved
// 2. remove_if tries to move elements during algorithm to fill gaps
// 3. Moving const std::string causes compilation errors
// 4. find+erase is more efficient for single-element operations
    {
        std::lock_guard<std::mutex> lock(m_connectionMutex);
        auto it = m_connectionMap.find(ip);
        if (it != m_connectionMap.end()) {
            it->second.currentConnections--;
            if (it->second.currentConnections <= 0) {
                m_connectionMap.erase(it);
            }
        }
        m_currentConnections--;
    }
    
    // Remove from clients list
    {
        std::lock_guard<std::mutex> lock(m_clientsMutex);
        m_clients.erase(
            std::remove_if(m_clients.begin(), m_clients.end(),
                [&ip](const std::unique_ptr<ClientConnection>& client) {
                    return client && client->clientIP == ip;
                }),
            m_clients.end()
        );
    }
    
    // Notify disconnection
    if (m_onDisconnect) {
        m_onDisconnect(ip);
    }
}

std::string HttpWsServer::getClientIP(const Socket& socket) {
    return socket.remoteAddress();
}

HTTPRequest HttpWsServer::parseHTTPRequest(const std::string& request, const std::string& clientIP) {
    HTTPRequest httpRequest;
    httpRequest.clientIP = clientIP;
    
    // Parse first line (method and path)
    size_t lineEnd = request.find("\r\n");
    if (lineEnd != std::string::npos) {
        std::string firstLine = request.substr(0, lineEnd);
        std::stringstream ss(firstLine);
        ss >> httpRequest.method >> httpRequest.path;
    }
    
    // Parse headers
    size_t pos = lineEnd + 2;
    while (pos < request.size()) {
        size_t nextLineEnd = request.find("\r\n", pos);
        if (nextLineEnd == std::string::npos) break;
        
        std::string line = request.substr(pos, nextLineEnd - pos);
        if (line.empty()) break;
        
        size_t colonPos = line.find(':');
        if (colonPos != std::string::npos) {
            std::string key = line.substr(0, colonPos);
            std::string value = line.substr(colonPos + 1);
            
            // Trim whitespace
            key.erase(0, key.find_first_not_of(" \t"));
            key.erase(key.find_last_not_of(" \t") + 1);
            value.erase(0, value.find_first_not_of(" \t"));
            value.erase(value.find_last_not_of(" \t") + 1);
            
            httpRequest.headers[key] = value;
        }
        
        pos = nextLineEnd + 2;
    }
    
    return httpRequest;
}

bool HttpWsServer::isWebSocketUpgrade(const std::string& request) const {
    std::string lowerRequest = request;
    std::transform(lowerRequest.begin(), lowerRequest.end(), lowerRequest.begin(), ::tolower);
    
    return lowerRequest.find("upgrade: websocket") != std::string::npos &&
           lowerRequest.find("connection: upgrade") != std::string::npos &&
           lowerRequest.find("sec-websocket-key:") != std::string::npos;
}

std::string HttpWsServer::generateHTTPResponse(const std::string& status, const std::string& contentType, const std::string& body) {
    std::vector<uint8_t> response;
    response.reserve(256 + body.size()); // Pre-allocate to avoid reallocations
    
    // Add status line
    const char* statusLine = "HTTP/1.1 ";
    response.insert(response.end(), statusLine, statusLine + 9);
    response.insert(response.end(), status.begin(), status.end());
    response.push_back('\r'); response.push_back('\n');
    
    // Add content type
    const char* contentTypeHeader = "Content-Type: ";
    response.insert(response.end(), contentTypeHeader, contentTypeHeader + 14);
    response.insert(response.end(), contentType.begin(), contentType.end());
    const char* charset = "; charset=UTF-8\r\n";
    response.insert(response.end(), charset, charset + 17);
    
    // Add content length
    const char* contentLengthHeader = "Content-Length: ";
    response.insert(response.end(), contentLengthHeader, contentLengthHeader + 16);
    std::string lengthStr = std::to_string(body.length());
    response.insert(response.end(), lengthStr.begin(), lengthStr.end());
    response.push_back('\r'); response.push_back('\n');
    
    // Add connection close
    const char* connectionHeader = "Connection: close\r\n";
    response.insert(response.end(), connectionHeader, connectionHeader + 19);
    
    // Add blank line
    response.push_back('\r'); response.push_back('\n');
    
    // Add body
    response.insert(response.end(), body.begin(), body.end());
    
    // Convert to string (this is the only copy needed)
    return std::string(response.begin(), response.end());
}

} // namespace WebSocket
