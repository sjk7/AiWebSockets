#pragma once

#include "Types.h"
#include "ErrorCodes.h"
#include "Socket.h"
#include <memory>
#include <vector>
#include <thread>
#include <atomic>
#include <mutex>

namespace WebSocket {

/**
 * @brief WebSocket Connection class
 * 
 * Represents a single WebSocket connection to a client.
 */
class WebSocketConnection {
public:
    explicit WebSocketConnection(std::unique_ptr<Socket> socket);
    ~WebSocketConnection();
    
    // Disable copying, allow moving with custom implementation
    WebSocketConnection(const WebSocketConnection&) = delete;
    WebSocketConnection& operator=(const WebSocketConnection&) = delete;
    WebSocketConnection(WebSocketConnection&& other) noexcept;
    WebSocketConnection& operator=(WebSocketConnection&& other) noexcept;
    
    // Connection management
    Result Send(const WebSocketMessage& message);
    Result SendText(const std::string& text);
    Result SendBinary(const std::vector<uint8_t>& data);
    Result SendPing(const std::vector<uint8_t>& data = {});
    Result SendPong(const std::vector<uint8_t>& data = {});
    Result Close(uint16_t code = 1000, const std::string& reason = "");
    
    // Getters
    WEBSOCKET_STATE GetState() const { return m_state; }
    std::string GetRemoteAddress() const;
    uint16_t GetRemotePort() const;
    bool IsConnected() const { return m_state == WEBSOCKET_STATE::OPEN; }
    
    // Internal methods (used by WebSocketServer)
    Result PerformHandshake(const std::string& request);
    Result ProcessIncomingData();
    void SetState(WEBSOCKET_STATE state) { m_state = state; }
    
private:
    std::unique_ptr<Socket> m_socket;
    WEBSOCKET_STATE m_state;
    std::vector<uint8_t> m_receiveBuffer;
    mutable std::mutex m_mutex;
    
    Result HandleFrame(const WebSocketFrame& frame);
    Result SendFrame(const WebSocketFrame& frame);
};

// WebSocketConnection move constructor implementation
inline WebSocketConnection::WebSocketConnection(WebSocketConnection&& other) noexcept
    : m_socket(std::move(other.m_socket))
    , m_state(other.m_state)
    , m_receiveBuffer(std::move(other.m_receiveBuffer))
    // Note: m_mutex is not moved, each connection gets its own mutex
{
    other.m_state = WEBSOCKET_STATE::CLOSED;
}

// WebSocketConnection move assignment implementation
inline WebSocketConnection& WebSocketConnection::operator=(WebSocketConnection&& other) noexcept {
    if (this != &other) {
        // Lock both mutexes for thread safety
        std::lock(m_mutex, other.m_mutex);
        std::lock_guard<std::mutex> lock1(m_mutex, std::adopt_lock);
        std::lock_guard<std::mutex> lock2(other.m_mutex, std::adopt_lock);
        
        // Move members
        m_socket = std::move(other.m_socket);
        m_state = other.m_state;
        m_receiveBuffer = std::move(other.m_receiveBuffer);
        
        // Reset other
        other.m_state = WEBSOCKET_STATE::CLOSED;
        other.m_receiveBuffer.clear();
    }
    return *this;
}

/**
 * @brief WebSocket Server class
 * 
 * Main server class that handles WebSocket connections, handshake,
 * and message routing using a single-threaded event loop.
 */
class WebSocketServer {
public:
    explicit WebSocketServer(const ServerConfig& config);
    ~WebSocketServer();
    
    // Disable copying
    WebSocketServer(const WebSocketServer&) = delete;
    WebSocketServer& operator=(const WebSocketServer&) = delete;
    
    // Server lifecycle
    Result Start();
    Result Stop();
    bool IsRunning() const { return m_running; }
    
    // Callback registration
    void SetConnectionCallback(ConnectionCallback callback) { m_connectionCallback = callback; }
    void SetMessageCallback(MessageCallback callback) { m_messageCallback = callback; }
    void SetCloseCallback(CloseCallback callback) { m_closeCallback = callback; }
    void SetErrorCallback(ErrorCallback callback) { m_errorCallback = callback; }
    
    // Event loop
    Result Update(); // Process one iteration of the event loop
    void Run(); // Run the event loop continuously
    
    // Configuration
    const ServerConfig& GetConfig() const { return m_config; }
    
private:
    ServerConfig m_config;
    std::unique_ptr<Socket> m_listenSocket;
    std::vector<std::shared_ptr<WebSocketConnection>> m_connections;
    std::atomic<bool> m_running;
    std::mutex m_mutex;
    
    // Callbacks
    ConnectionCallback m_connectionCallback;
    MessageCallback m_messageCallback;
    CloseCallback m_closeCallback;
    ErrorCallback m_errorCallback;
    
    // Internal methods
    Result AcceptNewConnection();
    void RemoveConnection(std::shared_ptr<WebSocketConnection> connection);
    void ProcessConnection(std::shared_ptr<WebSocketConnection> connection);
    void TriggerError(std::shared_ptr<WebSocketConnection> connection, const Result& error);
    void TriggerClose(std::shared_ptr<WebSocketConnection> connection, uint16_t code, const std::string& reason);
};

} // namespace WebSocket
