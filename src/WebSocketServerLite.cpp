#include "WebSocket/WebSocketServerLite.h"
#include "WebSocket/WebSocketProtocol.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <algorithm>
#include <sstream>
#include <winerror.h>

namespace nob {

WebSocketServerLite::WebSocketServerLite(uint16_t port, const std::string& bindAddress)
    : m_bindAddress(bindAddress), m_port(port), m_running(false), m_securityEnabled(true),
      m_maxConnections(50), m_maxConnectionsPerIP(5), m_maxConnectionsPerMinute(10) {
}

WebSocketServerLite::~WebSocketServerLite() {
    stop();
}

WebSocketServerLite& WebSocketServerLite::setPort(uint16_t port) {
    if (m_running) {
        throw std::runtime_error("Cannot change port while server is running");
    }
    m_port = port;
    return *this;
}

WebSocketServerLite& WebSocketServerLite::setBindAddress(const std::string& address) {
    if (m_running) {
        throw std::runtime_error("Cannot change bind address while server is running");
    }
    m_bindAddress = address;
    return *this;
}

WebSocketServerLite& WebSocketServerLite::enableSecurity(bool enabled) {
    m_securityEnabled = enabled;
    return *this;
}

WebSocketServerLite& WebSocketServerLite::setMaxConnections(int maxConnections) {
    m_maxConnections = maxConnections;
    return *this;
}

WebSocketServerLite& WebSocketServerLite::setMaxConnectionsPerIP(int maxPerIP) {
    m_maxConnectionsPerIP = maxPerIP;
    return *this;
}

WebSocketServerLite& WebSocketServerLite::setMaxConnectionsPerMinute(int maxPerMinute) {
    m_maxConnectionsPerMinute = maxPerMinute;
    return *this;
}

WebSocketServerLite& WebSocketServerLite::onMessage(const std::function<void(const std::string&)>& callback) {
    m_onMessage = callback;
    return *this;
}

WebSocketServerLite& WebSocketServerLite::onConnect(const std::function<void(const std::string&)>& callback) {
    m_onConnect = callback;
    return *this;
}

WebSocketServerLite& WebSocketServerLite::onDisconnect(const std::function<void(const std::string&)>& callback) {
    m_onDisconnect = callback;
    return *this;
}

WebSocketServerLite& WebSocketServerLite::onError(const std::function<void(const Result&)>& callback) {
    m_onError = callback;
    return *this;
}

Result WebSocketServerLite::start() {
    return startNonBlocking();
}

Result WebSocketServerLite::stop() {
    if (!m_running) {
        return Result();
    }
    
    m_running = false;
    
    if (m_serverSocket) {
        m_serverSocket->close();
        m_serverSocket.reset();
    }
    
    std::cout << "WebSocket Server stopped" << std::endl;
    return Result();
}

Result WebSocketServerLite::startNonBlocking() {
    if (m_running) {
        return Result(ErrorCode::invalidParameter, "Server is already running");
    }
    
    auto initResult = initializeServer();
    if (!initResult.isSuccess()) {
        if (m_onError) {
            m_onError(initResult);
        }
        return initResult;
    }
    
    m_running = true;
    std::cout << "WebSocket Server started on " << m_bindAddress << ":" << m_port << " (non-blocking)" << std::endl;
    std::cout << "Security: " << (m_securityEnabled ? "ENABLED" : "DISABLED") << std::endl;
    std::cout << "Max connections: " << m_maxConnections << " (per IP: " << m_maxConnectionsPerIP << ")" << std::endl;
    
    return Result();
}

void WebSocketServerLite::processEvents() {
    if (!m_running || !m_serverSocket) {
        return;
    }
    
    // Accept new connections
    auto acceptResult = m_serverSocket->accept();
    if (acceptResult.first.isSuccess() && acceptResult.second) {
        std::string clientIP = getClientIP(*acceptResult.second); // No HTTP request yet for initial connection
        
        if (m_securityEnabled && !isConnectionAllowed(clientIP)) {
            std::cout << "Connection rejected: " << clientIP << " (security limits exceeded)" << std::endl;
            acceptResult.second->close();
            return;
        }
        
        // Handle connection in a separate thread
        std::thread clientThread([this, client = acceptResult.second.release(), clientIP]() {
            std::unique_ptr<Socket> clientSocket(client);
            handleClientConnection(std::move(clientSocket));
        });
        clientThread.detach();
    }
}

int WebSocketServerLite::getCurrentConnectionCount() const {
    return currentConnections.load();
}

Result WebSocketServerLite::initializeServer() {
    // Check if port is available
    if (!Socket::isPortAvailable(m_port, m_bindAddress)) {
        return Result(ErrorCode::socketBindFailed, "Port " + std::to_string(m_port) + " is already in use");
    }
    
    // Create server socket
    m_serverSocket = std::make_unique<Socket>();
    
    // Determine socket family based on bind address
    socketFamily family = socketFamily::IPV4; // default
    if (Socket::isIPv6Address(m_bindAddress) || m_bindAddress == "::") {
        family = socketFamily::IPV6;
    }
    
    auto createResult = m_serverSocket->create(family, socketType::TCP);
    if (!createResult.isSuccess()) {
        m_serverSocket.reset();
        return createResult;
    }
    
    // Set socket to non-blocking mode FIRST
    auto blockingResult = m_serverSocket->blocking(false);
    if (!blockingResult.isSuccess()) {
        std::cout << "Warning: Failed to set non-blocking mode: " << blockingResult.getErrorMessage() << std::endl;
        // Continue anyway, but this is a problem
    }
    
    // Set socket options
    auto reuseResult = m_serverSocket->reuseAddress(true);
    if (!reuseResult.isSuccess()) {
        std::cout << "Warning: Failed to set reuse address: " << reuseResult.getErrorMessage() << std::endl;
    }
    
    // Bind to address and port
    auto bindResult = m_serverSocket->bind(m_bindAddress, m_port);
    if (!bindResult.isSuccess()) {
        m_serverSocket.reset();
        return bindResult;
    }
    
    // Start listening
    auto listenResult = m_serverSocket->listen(128);
    if (!listenResult.isSuccess()) {
        m_serverSocket.reset();
        return listenResult;
    }
    
    std::cout << "Server initialized in non-blocking mode" << std::endl;
    return Result();
}

void WebSocketServerLite::handleClientConnection(std::unique_ptr<Socket> clientSocket) {
    if (!clientSocket) {
        return;
    }
    
    std::string clientIP = getClientIP(*clientSocket);
    
    // Set client socket to non-blocking mode
    auto blockingResult = clientSocket->blocking(false);
    if (!blockingResult.isSuccess()) {
        std::cout << "Warning: Failed to set client socket non-blocking: " << blockingResult.getErrorMessage() << std::endl;
    }
    
    std::cout << "Client connected from " << clientIP << " (non-blocking)" << std::endl;
    
    if (m_onConnect) {
        m_onConnect(clientIP);
    }
    
    try {
        // Non-blocking receive loop
        std::string accumulatedRequest;
        const size_t MAX_REQUEST_SIZE = 65536;
        
        while (m_running) {
            auto receiveResult = clientSocket->receive(4096);
            
            if (receiveResult.first.isSuccess()) {
                if (receiveResult.second.empty()) {
                    // Connection closed gracefully
                    break;
                }
                
                accumulatedRequest.append(receiveResult.second.begin(), receiveResult.second.end());
                
                // Check if we have complete headers
                if (accumulatedRequest.find("\r\n\r\n") != std::string::npos) {
                    // Update client IP with HTTP header information (proxy detection)
                    std::string realClientIP = getClientIP(*clientSocket, accumulatedRequest);
                    if (realClientIP != clientIP) {
                        std::cout << "Updated client IP: " << clientIP << " -> " << realClientIP << " (proxy detected)" << std::endl;
                        clientIP = realClientIP;
                    }
                    break;
                }
                
                // Check size limit
                if (accumulatedRequest.size() > MAX_REQUEST_SIZE) {
                    std::cout << "Request too large from " << clientIP << std::endl;
                    break;
                }
            } else {
                Result error = receiveResult.first;
                if (error.getErrorCode() == ErrorCode::socketReceiveFailed) {
                    // Non-blocking receive would normally return "would block" - check if it's a real error
                    int systemError = error.getSystemErrorCode();
#ifdef _WIN32
                    if (systemError != SocketErrors::WOULD_BLOCK) {
#else
                    if (systemError != EAGAIN && systemError != EWOULDBLOCK) {
#endif
                        // Real error occurred
                        std::cout << "Receive error from " << clientIP << ": " << error.getErrorMessage() << std::endl;
                        break;
                    }
                    // Would block - continue loop
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                    continue;
                } else {
                    // Other error
                    std::cout << "Receive error from " << clientIP << ": " << error.getErrorMessage() << std::endl;
                    break;
                }
            }
        }
        
        // Validate HTTP request
        if (m_securityEnabled && !isHTTPRequestValid(accumulatedRequest)) {
            std::cout << "Invalid HTTP request from " << clientIP << std::endl;
            sendHTTPResponse(*clientSocket, "400 Bad Request", "text/plain", "Bad Request");
            removeConnection(clientIP);
            return;
        }
        
        // Perform WebSocket handshake
        auto handshakeResult = performWebSocketHandshake(*clientSocket, accumulatedRequest);
        if (!handshakeResult.isSuccess()) {
            std::cout << "WebSocket handshake failed: " << handshakeResult.getErrorMessage() << std::endl;
            sendHTTPResponse(*clientSocket, "400 Bad Request", "text/plain", "WebSocket handshake failed");
            removeConnection(clientIP);
            return;
        }
        
        std::cout << "WebSocket handshake successful for " << clientIP << std::endl;
        
        // Handle WebSocket messages (non-blocking)
        while (m_running) {
            // Simple frame receive implementation
            auto receiveResult = clientSocket->receive(4096);
            if (!receiveResult.first.isSuccess()) {
                Result error = receiveResult.first;
                if (error.getErrorCode() == ErrorCode::websocketConnectionClosed) {
                    break; // Normal closure
                } else if (error.getErrorCode() == ErrorCode::socketReceiveFailed) {
                    // Check if it's just a non-blocking "would block" situation
                    int systemError = error.getSystemErrorCode();
#ifdef _WIN32
                    if (systemError != SocketErrors::WOULD_BLOCK) {
#else
                    if (systemError != EAGAIN && systemError != EWOULDBLOCK) {
#endif
                        // Real error
                        std::cout << "WebSocket receive error: " << error.getErrorMessage() << std::endl;
                        break;
                    }
                    // Would block - continue
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                    continue;
                } else {
                    // Other error
                    std::cout << "WebSocket error: " << error.getErrorMessage() << std::endl;
                    break;
                }
            }
            
            if (m_onMessage && !receiveResult.second.empty()) {
                std::string message(receiveResult.second.begin(), receiveResult.second.end());
                m_onMessage(message);
            }
        }
        
    } catch (const std::exception& e) {
        std::cout << "Exception in client handler: " << e.what() << std::endl;
    }
    
    std::cout << "Client disconnected: " << clientIP << std::endl;
    
    if (m_onDisconnect) {
        m_onDisconnect(clientIP);
    }
    
    removeConnection(clientIP);
}

bool WebSocketServerLite::isHTTPRequestValid(const std::string& request) {
    // Check request size
    const size_t MAX_REQUEST_SIZE = 65536;
    if (request.length() > MAX_REQUEST_SIZE) {
        return false;
    }
    
    // Check for complete headers
    if (request.find("\r\n\r\n") == std::string::npos) {
        return false;
    }
    
    // Check for required HTTP request line
    if (request.find("GET ") != 0) {
        return false;
    }
    
    // Check for required headers
    if (request.find("\r\nHost:") == std::string::npos) {
        return false;
    }
    
    // User-Agent filtering (security feature)
    if (request.find("\r\nUser-Agent:") != std::string::npos) {
        size_t userAgentPos = request.find("\r\nUser-Agent:");
        size_t userAgentStart = userAgentPos + 13;
        size_t userAgentEnd = request.find("\r\n", userAgentStart);
        
        if (userAgentEnd == std::string::npos) {
            userAgentEnd = request.length();
        }
        
        if (userAgentEnd > userAgentStart) {
            std::string userAgent = request.substr(userAgentStart, userAgentEnd - userAgentStart);
            // Trim whitespace
            userAgent.erase(0, userAgent.find_first_not_of(" \t"));
            userAgent.erase(userAgent.find_last_not_of(" \t\r\n") + 1);
            
            // Check for common attack patterns (case-insensitive)
            std::string userAgentLower = userAgent;
            std::transform(userAgentLower.begin(), userAgentLower.end(), userAgentLower.begin(), ::tolower);
            
            if (userAgentLower.find("sqlmap") != std::string::npos ||
                userAgentLower.find("nikto") != std::string::npos ||
                userAgentLower.find("nmap") != std::string::npos ||
                userAgentLower.find("masscan") != std::string::npos) {
                std::cout << "Suspicious User-Agent blocked: " << userAgent << std::endl;
                return false;
            }
        }
    }
    
    return true;
}

Result WebSocketServerLite::performWebSocketHandshake(Socket& clientSocket, const std::string& request) {
    (void)clientSocket; // Suppress unused parameter warning
    HandshakeInfo info;
    return WebSocketProtocol::validateHandshakeRequest(request, info);
}

void WebSocketServerLite::sendHTTPResponse(Socket& clientSocket, const std::string& status, const std::string& contentType, const std::string& body) {
    std::stringstream response;
    response << "HTTP/1.1 " << status << "\r\n";
    response << "Content-Type: " << contentType << "\r\n";
    response << "Content-Length: " << body.length() << "\r\n";
    response << "Connection: close\r\n";
    response << "\r\n";
    response << body;
    
    std::string responseStr = response.str();
    clientSocket.send(std::vector<uint8_t>(responseStr.begin(), responseStr.end()));
}

std::string WebSocketServerLite::getClientIP(const Socket& socket, const std::string& httpRequest) {
    // First check for X-Forwarded-For header (proxy/load balancer scenario)
    if (!httpRequest.empty()) {
        size_t xffPos = httpRequest.find("\r\nX-Forwarded-For:");
        if (xffPos != std::string::npos) {
            size_t valueStart = httpRequest.find(":", xffPos) + 1;
            size_t valueEnd = httpRequest.find("\r\n", valueStart);
            
            if (valueEnd != std::string::npos && valueStart > xffPos) {
                std::string xffValue = httpRequest.substr(valueStart, valueEnd - valueStart);
                // Trim whitespace
                xffValue.erase(0, xffValue.find_first_not_of(" \t"));
                xffValue.erase(xffValue.find_last_not_of(" \t\r\n") + 1);
                
                // X-Forwarded-For can contain multiple IPs, take the first (original client)
                size_t commaPos = xffValue.find(',');
                if (commaPos != std::string::npos) {
                    xffValue = xffValue.substr(0, commaPos);
                }
                
                // Basic IP validation
                if (!xffValue.empty() && xffValue != "unknown") {
                    return xffValue;
                }
            }
        }
        
        // Also check X-Real-IP header (nginx/common proxy)
        size_t realIPPos = httpRequest.find("\r\nX-Real-IP:");
        if (realIPPos != std::string::npos) {
            size_t valueStart = httpRequest.find(":", realIPPos) + 1;
            size_t valueEnd = httpRequest.find("\r\n", valueStart);
            
            if (valueEnd != std::string::npos && valueStart > realIPPos) {
                std::string realIPValue = httpRequest.substr(valueStart, valueEnd - valueStart);
                // Trim whitespace
                realIPValue.erase(0, realIPValue.find_first_not_of(" \t"));
                realIPValue.erase(realIPValue.find_last_not_of(" \t\r\n") + 1);
                
                if (!realIPValue.empty() && realIPValue != "unknown") {
                    return realIPValue;
                }
            }
        }
    }
    
    // Fallback to direct socket peer IP
    return socket.remoteAddress();
}

bool WebSocketServerLite::isConnectionAllowed(const std::string& clientIP) {
    if (!m_securityEnabled) {
        return true;
    }
    
    std::lock_guard<std::mutex> lock(ipConnectionMutex);
    auto now = std::chrono::steady_clock::now();
    
    // Check global connection limit
    if (currentConnections.load() >= m_maxConnections) {
        return false;
    }
    
    // Get or create IP info
    ConnectionInfo& ipInfo = ipConnectionMap[clientIP];
    
    // Reset per-minute counter if needed
    if (now - ipInfo.minuteStart > std::chrono::minutes(1)) {
        ipInfo.connectionsPerMinute = 0;
        ipInfo.minuteStart = now;
    }
    
    // Check per-IP connection limit
    if (ipInfo.currentConnections >= m_maxConnectionsPerIP) {
        return false;
    }
    
    // Check rate limiting
    if (ipInfo.connectionsPerMinute >= m_maxConnectionsPerMinute) {
        return false;
    }
    
    // Update counters
    ipInfo.currentConnections++;
    ipInfo.connectionsPerMinute++;
    ipInfo.lastConnectionTime = now;
    currentConnections++;
    
    return true;
}

void WebSocketServerLite::removeConnection(const std::string& clientIP) {
    if (!m_securityEnabled) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(ipConnectionMutex);
    currentConnections--;

    auto it = ipConnectionMap.find(clientIP);
    if (it != ipConnectionMap.end()) {
        it->second.currentConnections--;
        if (it->second.currentConnections <= 0) {
            ipConnectionMap.erase(it);
        }
    }
}

} // namespace nob
