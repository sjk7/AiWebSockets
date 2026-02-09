#include "WebSocket/WebSocketClientLite.h"
#include "WebSocket/WebSocketProtocol.h"
#include <iostream>
#include <sstream>

namespace nob {

WebSocketClientLite::WebSocketClientLite(const std::string& host, uint16_t port)
    : m_serverHost(host), m_serverPort(port), m_connected(false) {
}

WebSocketClientLite::~WebSocketClientLite() {
    disconnect();
}

WebSocketClientLite& WebSocketClientLite::setServer(const std::string& host, uint16_t port) {
    if (m_connected) {
        throw std::runtime_error("Cannot change server while connected");
    }
    m_serverHost = host;
    m_serverPort = port;
    return *this;
}

WebSocketClientLite& WebSocketClientLite::onMessage(const std::function<void(const std::string&)>& callback) {
    m_onMessage = callback;
    return *this;
}

WebSocketClientLite& WebSocketClientLite::onConnect(const std::function<void()>& callback) {
    m_onConnect = callback;
    return *this;
}

WebSocketClientLite& WebSocketClientLite::onDisconnect(const std::function<void()>& callback) {
    m_onDisconnect = callback;
    return *this;
}

WebSocketClientLite& WebSocketClientLite::onError(const std::function<void(const Result&)>& callback) {
    m_onError = callback;
    return *this;
}

Result WebSocketClientLite::connect() {
    if (m_connected) {
        return Result(ErrorCode::invalidParameter, "Already connected");
    }
    
    // Create socket
    m_socket = std::make_unique<Socket>();
    auto createResult = m_socket->create(socketFamily::IPV4, socketType::TCP);
    if (!createResult.isSuccess()) {
        m_socket.reset();
        if (m_onError) {
            m_onError(createResult);
        }
        return createResult;
    }
    
    // Set socket to non-blocking mode BEFORE connecting
    auto blockingResult = m_socket->blocking(false);
    if (!blockingResult.isSuccess()) {
        std::cout << "Warning: Failed to set non-blocking mode: " << blockingResult.getErrorMessage() << std::endl;
    }
    
    // Connect to server (non-blocking)
    auto connectResult = m_socket->connect(m_serverHost, m_serverPort);
    if (!connectResult.isSuccess()) {
        // For non-blocking sockets, connect might return "in progress"
        int systemError = connectResult.getSystemErrorCode();
#ifdef _WIN32
        if (systemError != SocketErrors::WOULD_BLOCK) {
#else
        if (systemError != EINPROGRESS) {
#endif
            m_socket.reset();
            if (m_onError) {
                m_onError(connectResult);
            }
            return connectResult;
        }
        
        // For non-blocking connect, we need to wait for the connection to complete
        std::cout << "Connecting to server (non-blocking)..." << std::endl;
        
        // Simple polling approach - in production, use select/poll/epoll
        for (int i = 0; i < 100; ++i) { // Wait up to 5 seconds
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            
            // Check if socket is writable (connection completed)
            // For simplicity, we'll just try the handshake
            auto handshakeResult = performWebSocketHandshake();
            if (handshakeResult.isSuccess()) {
                m_connected = true;
                std::cout << "Connected to WebSocket server at " << m_serverHost << ":" << m_serverPort << " (non-blocking)" << std::endl;
                
                if (m_onConnect) {
                    m_onConnect();
                }
                return Result();
            }
            
            // Check if connection failed
            if (handshakeResult.getErrorCode() == ErrorCode::socketReceiveFailed) {
                int handshakeSystemError = handshakeResult.getSystemErrorCode();
#ifdef _WIN32
                if (handshakeSystemError != SocketErrors::WOULD_BLOCK && handshakeSystemError != SocketErrors::CONN_REFUSED) {
#else
                if (handshakeSystemError != EAGAIN && handshakeSystemError != EWOULDBLOCK && handshakeSystemError != ECONNREFUSED) {
#endif
                    m_socket.reset();
                    if (m_onError) {
                        m_onError(handshakeResult);
                    }
                    return handshakeResult;
                }
            }
        }
        
        // Connection timeout
        m_socket.reset();
        Result timeoutResult(ErrorCode::socketConnectFailed, "Connection timeout");
        if (m_onError) {
            m_onError(timeoutResult);
        }
        return timeoutResult;
    }
    
    // Perform WebSocket handshake
    auto handshakeResult = performWebSocketHandshake();
    if (!handshakeResult.isSuccess()) {
        m_socket->close();
        m_socket.reset();
        if (m_onError) {
            m_onError(handshakeResult);
        }
        return handshakeResult;
    }
    
    m_connected = true;
    std::cout << "Connected to WebSocket server at " << m_serverHost << ":" << m_serverPort << " (non-blocking)" << std::endl;
    
    if (m_onConnect) {
        m_onConnect();
    }
    
    return Result();
}

Result WebSocketClientLite::disconnect() {
    if (!m_connected) {
        return Result();
    }
    
    m_connected = false;
    
    if (m_socket) {
        // Send close frame
        std::vector<uint8_t> closeFrame = {0x88, 0x00}; // Close frame
        m_socket->send(closeFrame);
        
        m_socket->close();
        m_socket.reset();
    }
    
    std::cout << "Disconnected from WebSocket server" << std::endl;
    
    if (m_onDisconnect) {
        m_onDisconnect();
    }
    
    return Result();
}

Result WebSocketClientLite::sendMessage(const std::string& message) {
    if (!m_connected || !m_socket) {
        return Result(ErrorCode::invalidParameter, "Not connected");
    }
    
    std::vector<uint8_t> data(message.begin(), message.end());
    return sendWebSocketFrame(data, 0x1); // Text frame opcode
}

Result WebSocketClientLite::sendBinary(const std::vector<uint8_t>& data) {
    if (!m_connected || !m_socket) {
        return Result(ErrorCode::invalidParameter, "Not connected");
    }
    
    return sendWebSocketFrame(data, 0x2); // Binary frame opcode
}

MessageReceiveResult WebSocketClientLite::receiveMessage() {
    if (!m_connected || !m_socket) {
        return {Result(ErrorCode::invalidParameter, "Not connected"), ""};
    }
    
    auto receiveResult = m_socket->receive(4096);
    if (!receiveResult.first.isSuccess()) {
        return {receiveResult.first, ""};
    }
    
    std::string message(receiveResult.second.begin(), receiveResult.second.end());
    return {Result(), message};
}

void WebSocketClientLite::processMessages() {
    if (!m_connected || !m_socket) {
        return;
    }
    
    auto receiveResult = m_socket->receive(4096);
    if (!receiveResult.first.isSuccess()) {
        Result error = receiveResult.first;
        if (error.getErrorCode() == ErrorCode::websocketConnectionClosed) {
            m_connected = false;
            std::cout << "Server closed connection" << std::endl;
            if (m_onDisconnect) {
                m_onDisconnect();
            }
        } else if (error.getErrorCode() == ErrorCode::socketReceiveFailed) {
            // Check if it's just a non-blocking "would block" situation
            int systemError = error.getSystemErrorCode();
#ifdef _WIN32
            if (systemError != SocketErrors::WOULD_BLOCK) {
#else
            if (systemError != EAGAIN && systemError != EWOULDBLOCK) {
#endif
                // Real error occurred
                std::cout << "Receive error: " << error.getErrorMessage() << std::endl;
                m_connected = false;
                if (m_onDisconnect) {
                    m_onDisconnect();
                }
                if (m_onError) {
                    m_onError(error);
                }
            }
            // Would block - just return, no message available
        } else {
            // Other error
            std::cout << "WebSocket error: " << error.getErrorMessage() << std::endl;
            m_connected = false;
            if (m_onDisconnect) {
                m_onDisconnect();
            }
            if (m_onError) {
                m_onError(error);
            }
        }
        return;
    }
    
    if (m_onMessage && !receiveResult.second.empty()) {
        std::string message(receiveResult.second.begin(), receiveResult.second.end());
        m_onMessage(message);
    }
}

Result WebSocketClientLite::performWebSocketHandshake() {
    // Send WebSocket handshake request
    std::stringstream request;
    request << "GET / HTTP/1.1\r\n";
    request << "Host: " << m_serverHost << ":" << m_serverPort << "\r\n";
    request << "Upgrade: websocket\r\n";
    request << "Connection: Upgrade\r\n";
    request << "Sec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\n"; // Sample key
    request << "Sec-WebSocket-Version: 13\r\n";
    request << "\r\n";
    
    std::string requestStr = request.str();
    auto sendResult = m_socket->send(std::vector<uint8_t>(requestStr.begin(), requestStr.end()));
    if (!sendResult.isSuccess()) {
        return sendResult;
    }
    
    // Receive handshake response
    auto receiveResult = m_socket->receive(4096);
    if (!receiveResult.first.isSuccess()) {
        return receiveResult.first;
    }
    
    std::string response(receiveResult.second.begin(), receiveResult.second.end());
    
    // Validate handshake response
    if (response.find("HTTP/1.1 101") == std::string::npos) {
        return Result(ErrorCode::websocketHandshakeFailed, "Invalid handshake response");
    }
    
    if (response.find("Upgrade: websocket") == std::string::npos) {
        return Result(ErrorCode::websocketHandshakeFailed, "Missing Upgrade header");
    }
    
    return Result();
}

Result WebSocketClientLite::sendWebSocketFrame(const std::vector<uint8_t>& data, int opcode) {
    if (!m_socket) {
        return Result(ErrorCode::invalidParameter, "No socket available");
    }
    
    // Simple WebSocket frame construction
    std::vector<uint8_t> frame;
    
    // First byte: FIN=1, RSV=000, Opcode
    frame.push_back(0x80 | opcode);
    
    // Payload length
    if (data.size() < 126) {
        frame.push_back(static_cast<uint8_t>(data.size()));
    } else if (data.size() < 65536) {
        frame.push_back(126);
        frame.push_back(static_cast<uint8_t>(data.size() >> 8));
        frame.push_back(static_cast<uint8_t>(data.size() & 0xFF));
    } else {
        // Not supporting very large frames in this simple implementation
        return Result(ErrorCode::websocketPayloadTooLarge, "Payload too large");
    }
    
    // Payload
    frame.insert(frame.end(), data.begin(), data.end());
    
    return m_socket->send(frame);
}

} // namespace nob
