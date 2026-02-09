#include "WebSocket/WebSocketServerLite.h"
#include "WebSocket/WebSocketProtocol.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <map>
#include <algorithm>
#include <sstream>

namespace WebSocket {

WebSocketServerLite::WebSocketServerLite(uint16_t port, const std::string& bindAddress)
    : m_bindAddress(bindAddress), m_port(port), m_running(false), m_securityEnabled(true),
      m_maxConnections(50), m_maxConnectionsPerIP(5), m_maxConnectionsPerMinute(10) {
}

WebSocketServerLite::~WebSocketServerLite() {
    Stop();
}

WebSocketServerLite& WebSocketServerLite::SetPort(uint16_t port) {
    if (m_running) {
        throw std::runtime_error("Cannot change port while server is running");
    }
    m_port = port;
    return *this;
}

WebSocketServerLite& WebSocketServerLite::SetBindAddress(const std::string& address) {
    if (m_running) {
        throw std::runtime_error("Cannot change bind address while server is running");
    }
    m_bindAddress = address;
    return *this;
}

WebSocketServerLite& WebSocketServerLite::EnableSecurity(bool enabled) {
    m_securityEnabled = enabled;
    return *this;
}

WebSocketServerLite& WebSocketServerLite::SetMaxConnections(int maxConnections) {
    m_maxConnections = maxConnections;
    return *this;
}

WebSocketServerLite& WebSocketServerLite::SetMaxConnectionsPerIP(int maxPerIP) {
    m_maxConnectionsPerIP = maxPerIP;
    return *this;
}

WebSocketServerLite& WebSocketServerLite::SetMaxConnectionsPerMinute(int maxPerMinute) {
    m_maxConnectionsPerMinute = maxPerMinute;
    return *this;
}

WebSocketServerLite& WebSocketServerLite::OnMessage(const std::function<void(const std::string&)>& callback) {
    m_onMessage = callback;
    return *this;
}

WebSocketServerLite& WebSocketServerLite::OnConnect(const std::function<void(const std::string&)>& callback) {
    m_onConnect = callback;
    return *this;
}

WebSocketServerLite& WebSocketServerLite::OnDisconnect(const std::function<void(const std::string&)>& callback) {
    m_onDisconnect = callback;
    return *this;
}

WebSocketServerLite& WebSocketServerLite::OnError(const std::function<void(const Result&)>& callback) {
    m_onError = callback;
    return *this;
}

Result WebSocketServerLite::Start() {
    return StartNonBlocking();
}

Result WebSocketServerLite::Stop() {
    if (!m_running) {
        return Result();
    }
    
    m_running = false;
    
    if (m_serverSocket) {
        m_serverSocket->close();
        m_serverSocket.reset();
    }
    
    std::cout << "🛑 WebSocket Server stopped" << std::endl;
    return Result();
}

Result WebSocketServerLite::StartNonBlocking() {
    if (m_running) {
        return Result(ERROR_CODE::INVALID_PARAMETER, "Server is already running");
    }
    
    auto initResult = InitializeServer();
    if (!initResult.IsSuccess()) {
        if (m_onError) {
            m_onError(initResult);
        }
        return initResult;
    }
    
    m_running = true;
    std::cout << "🚀 WebSocket Server started on " << m_bindAddress << ":" << m_port << " (non-blocking)" << std::endl;
    std::cout << "🔒 Security: " << (m_securityEnabled ? "ENABLED" : "DISABLED") << std::endl;
    std::cout << "📊 Max connections: " << m_maxConnections << " (per IP: " << m_maxConnectionsPerIP << ")" << std::endl;
    
    return Result();
}

void WebSocketServerLite::ProcessEvents() {
    if (!m_running || !m_serverSocket) {
        return;
    }
    
    // Accept new connections
    auto acceptResult = m_serverSocket->accept();
    if (acceptResult.first.IsSuccess() && acceptResult.second) {
        std::string clientIP = GetClientIP(*acceptResult.second); // No HTTP request yet for initial connection
        
        if (m_securityEnabled && !IsConnectionAllowed(clientIP)) {
            std::cout << "🚫 Connection rejected: " << clientIP << " (security limits exceeded)" << std::endl;
            acceptResult.second->close();
            return;
        }
        
        // Handle connection in a separate thread
        std::thread clientThread([this, client = acceptResult.second.release(), clientIP]() {
            std::unique_ptr<Socket> clientSocket(client);
            HandleClientConnection(std::move(clientSocket));
        });
        clientThread.detach();
    }
}

int WebSocketServerLite::GetCurrentConnectionCount() const {
    return currentConnections.load();
}

Result WebSocketServerLite::InitializeServer() {
    // Check if port is available
    if (!Socket::IsPortAvailable(m_port, m_bindAddress)) {
        return Result(ERROR_CODE::SOCKET_BIND_FAILED, "Port " + std::to_string(m_port) + " is already in use");
    }
    
    // Create server socket
    m_serverSocket = std::make_unique<Socket>();
    
    // Determine socket family based on bind address
    SOCKET_FAMILY family = SOCKET_FAMILY::IPV4; // default
    if (Socket::IsIPv6Address(m_bindAddress) || m_bindAddress == "::") {
        family = SOCKET_FAMILY::IPV6;
    }
    
    auto createResult = m_serverSocket->create(family, SOCKET_TYPE::TCP);
    if (!createResult.IsSuccess()) {
        m_serverSocket.reset();
        return createResult;
    }
    
    // Set socket to non-blocking mode FIRST
    auto blockingResult = m_serverSocket->Blocking(false);
    if (!blockingResult.IsSuccess()) {
        std::cout << "⚠️ Warning: Failed to set non-blocking mode: " << blockingResult.GetErrorMessage() << std::endl;
        // Continue anyway, but this is a problem
    }
    
    // Set socket options
    auto reuseResult = m_serverSocket->ReuseAddress(true);
    if (!reuseResult.IsSuccess()) {
        std::cout << "⚠️ Warning: Failed to set reuse address: " << reuseResult.GetErrorMessage() << std::endl;
    }
    
    // Bind to address and port
    auto bindResult = m_serverSocket->bind(m_bindAddress, m_port);
    if (!bindResult.IsSuccess()) {
        m_serverSocket.reset();
        return bindResult;
    }
    
    // Start listening
    auto listenResult = m_serverSocket->listen(128);
    if (!listenResult.IsSuccess()) {
        m_serverSocket.reset();
        return listenResult;
    }
    
    std::cout << "✅ Server initialized in non-blocking mode" << std::endl;
    return Result();
}

void WebSocketServerLite::HandleClientConnection(std::unique_ptr<Socket> clientSocket) {
    if (!clientSocket) {
        return;
    }
    
    std::string clientIP = GetClientIP(*clientSocket);
    
    // Set client socket to non-blocking mode
    auto blockingResult = clientSocket->Blocking(false);
    if (!blockingResult.IsSuccess()) {
        std::cout << "⚠️ Warning: Failed to set client socket non-blocking: " << blockingResult.GetErrorMessage() << std::endl;
    }
    
    std::cout << "🔗 Client connected from " << clientIP << " (non-blocking)" << std::endl;
    
    if (m_onConnect) {
        m_onConnect(clientIP);
    }
    
    try {
        // Non-blocking receive loop
        std::string accumulatedRequest;
        const size_t MAX_REQUEST_SIZE = 65536;
        
        while (m_running) {
            auto receiveResult = clientSocket->Receive(4096);
            
            if (receiveResult.first.IsSuccess()) {
                if (receiveResult.second.empty()) {
                    // Connection closed gracefully
                    break;
                }
                
                accumulatedRequest.append(receiveResult.second.begin(), receiveResult.second.end());
                
                // Check if we have complete headers
                if (accumulatedRequest.find("\r\n\r\n") != std::string::npos) {
                    // Update client IP with HTTP header information (proxy detection)
                    std::string realClientIP = GetClientIP(*clientSocket, accumulatedRequest);
                    if (realClientIP != clientIP) {
                        std::cout << "🔄 Updated client IP: " << clientIP << " -> " << realClientIP << " (proxy detected)" << std::endl;
                        clientIP = realClientIP;
                    }
                    break;
                }
                
                // Check size limit
                if (accumulatedRequest.size() > MAX_REQUEST_SIZE) {
                    std::cout << "🚫 Request too large from " << clientIP << std::endl;
                    break;
                }
            } else {
                Result error = receiveResult.first;
                if (error.GetErrorCode() == ERROR_CODE::SOCKET_RECEIVE_FAILED) {
                    // Non-blocking receive would normally return "would block" - check if it's a real error
                    int systemError = error.GetSystemErrorCode();
#ifdef _WIN32
                    if (systemError != WSAEWOULDBLOCK) {
#else
                    if (systemError != EAGAIN && systemError != EWOULDBLOCK) {
#endif
                        // Real error occurred
                        std::cout << "❌ Receive error from " << clientIP << ": " << error.GetErrorMessage() << std::endl;
                        break;
                    }
                    // Would block - continue loop
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                    continue;
                } else {
                    // Other error
                    std::cout << "❌ Receive error from " << clientIP << ": " << error.GetErrorMessage() << std::endl;
                    break;
                }
            }
        }
        
        // Validate HTTP request
        if (m_securityEnabled && !ValidateHTTPRequest(accumulatedRequest)) {
            std::cout << "🚫 Invalid HTTP request from " << clientIP << std::endl;
            SendHTTPResponse(*clientSocket, "400 Bad Request", "text/plain", "Bad Request");
            RemoveConnection(clientIP);
            return;
        }
        
        // Perform WebSocket handshake
        auto handshakeResult = PerformWebSocketHandshake(*clientSocket, accumulatedRequest);
        if (!handshakeResult.IsSuccess()) {
            std::cout << "❌ WebSocket handshake failed: " << handshakeResult.GetErrorMessage() << std::endl;
            SendHTTPResponse(*clientSocket, "400 Bad Request", "text/plain", "WebSocket handshake failed");
            RemoveConnection(clientIP);
            return;
        }
        
        std::cout << "✅ WebSocket handshake successful for " << clientIP << std::endl;
        
        // Handle WebSocket messages (non-blocking)
        while (m_running) {
            // Simple frame receive implementation
            auto receiveResult = clientSocket->Receive(4096);
            if (!receiveResult.first.IsSuccess()) {
                Result error = receiveResult.first;
                if (error.GetErrorCode() == ERROR_CODE::WEBSOCKET_CONNECTION_CLOSED) {
                    break; // Normal closure
                } else if (error.GetErrorCode() == ERROR_CODE::SOCKET_RECEIVE_FAILED) {
                    // Check if it's just a non-blocking "would block" situation
                    int systemError = error.GetSystemErrorCode();
#ifdef _WIN32
                    if (systemError != WSAEWOULDBLOCK) {
#else
                    if (systemError != EAGAIN && systemError != EWOULDBLOCK) {
#endif
                        // Real error
                        std::cout << "❌ WebSocket receive error: " << error.GetErrorMessage() << std::endl;
                        break;
                    }
                    // Would block - continue
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                    continue;
                } else {
                    // Other error
                    std::cout << "❌ WebSocket error: " << error.GetErrorMessage() << std::endl;
                    break;
                }
            }
            
            if (m_onMessage && !receiveResult.second.empty()) {
                std::string message(receiveResult.second.begin(), receiveResult.second.end());
                m_onMessage(message);
            }
        }
        
    } catch (const std::exception& e) {
        std::cout << "❌ Exception in client handler: " << e.what() << std::endl;
    }
    
    std::cout << "🔌 Client disconnected: " << clientIP << std::endl;
    
    if (m_onDisconnect) {
        m_onDisconnect(clientIP);
    }
    
    RemoveConnection(clientIP);
}

bool WebSocketServerLite::ValidateHTTPRequest(const std::string& request) {
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
                std::cout << "🚫 Suspicious User-Agent blocked: " << userAgent << std::endl;
                return false;
            }
        }
    }
    
    return true;
}

Result WebSocketServerLite::PerformWebSocketHandshake(Socket& clientSocket, const std::string& request) {
    (void)clientSocket; // Suppress unused parameter warning
    HandshakeInfo info;
    return WebSocketProtocol::ValidateHandshakeRequest(request, info);
}

void WebSocketServerLite::SendHTTPResponse(Socket& clientSocket, const std::string& status, const std::string& contentType, const std::string& body) {
    std::stringstream response;
    response << "HTTP/1.1 " << status << "\r\n";
    response << "Content-Type: " << contentType << "\r\n";
    response << "Content-Length: " << body.length() << "\r\n";
    response << "Connection: close\r\n";
    response << "\r\n";
    response << body;
    
    std::string responseStr = response.str();
    clientSocket.Send(std::vector<uint8_t>(responseStr.begin(), responseStr.end()));
}

std::string WebSocketServerLite::GetClientIP(const Socket& socket, const std::string& httpRequest) {
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
    return socket.RemoteAddress();
}

bool WebSocketServerLite::IsConnectionAllowed(const std::string& clientIP) {
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

void WebSocketServerLite::RemoveConnection(const std::string& clientIP) {
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

} // namespace WebSocket
