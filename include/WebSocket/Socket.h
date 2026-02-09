#pragma once

#include "ErrorCodes.h"
#include "Types.h"
#include <memory>
#include <string>
#include <thread>
#include <mutex>
#include <atomic>
#include <functional>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <mswsock.h>
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/epoll.h>
#endif

namespace WebSocket {

// Platform-specific types
#ifdef _WIN32
using SOCKET_TYPE_NATIVE = SOCKET;
static const SOCKET_TYPE_NATIVE INVALID_SOCKET_NATIVE = INVALID_SOCKET;
#else
using SOCKET_TYPE_NATIVE = int;
static const SOCKET_TYPE_NATIVE INVALID_SOCKET_NATIVE = -1;
#endif

// Type aliases for cleaner code
using AcceptResult = std::pair<Result, std::unique_ptr<Socket>>;
using SendResult = std::pair<Result, size_t>;
using ReceiveResult = std::pair<Result, std::vector<uint8_t>>;

/**
 * @brief Cross-platform socket wrapper class
 * 
 * This class provides a platform-independent interface for socket operations.
 * It handles both IPv4 and IPv6, TCP and UDP sockets.
 * Uses C-style I/O and proper error handling without exceptions.
 */
class Socket {
public:
    Socket();
    ~Socket();

    // Disable copying, allow moving
    Socket(const Socket&) = delete;
    Socket& operator=(const Socket&) = delete;
    Socket(Socket&& other) noexcept;
    Socket& operator=(Socket&& other) noexcept;

    // Socket creation and configuration
    Result create(socketFamily family, socketType type);
    Result bind(const std::string& address, uint16_t port);
    Result listen(int backlog = 128);
    std::pair<Result, std::unique_ptr<Socket>> accept();
    Result connect(const std::string& address, uint16_t port);
    Result shutdown();
    Result close();

    // Data transmission - Raw methods
    SendResult sendRaw(const void* data, size_t length);
    std::pair<Result, std::vector<uint8_t>> receiveRaw(void* buffer, size_t bufferSize);

    // Data transmission - WebSocket-aware methods
    Result send(const std::vector<uint8_t>& data);
    std::pair<Result, std::vector<uint8_t>> receive(size_t maxLength);
    std::pair<Result, std::vector<uint8_t>> receive(size_t maxLength, int timeoutMs);

    // Socket options
    Result blocking(bool blocking);
    Result reuseAddress(bool reuse);
    Result keepAlive(bool keepAlive);
    Result sendBufferSize(size_t size);
    Result receiveBufferSize(size_t size);

    // Getters
    bool valid() const;
    bool blocking() const;
    std::string localAddress() const;
    uint16_t localPort() const;
    std::string remoteAddress() const;
    uint16_t remotePort() const;

    // Utility methods
    static bool isIPAddress(const std::string& address);
    static bool isIPv4Address(const std::string& address);
    static bool isIPv6Address(const std::string& address);
    static bool isPortAvailable(uint16_t port, const std::string& address = "127.0.0.1");
    static std::vector<std::string> getLocalIPAddresses();

    // Async I/O methods (high performance)
    Result enableAsyncIO();
    Result sendAsync(const std::vector<uint8_t>& data);
    Result receiveAsync(size_t maxLength);
    bool isAsyncEnabled() const;

    // Event loop methods (for server sockets)
    Result startEventLoop();
    Result stopEventLoop();
    bool eventLoopRunning() const;

    // Callback types for async events
    using AcceptCallbackFn = std::function<void(std::unique_ptr<Socket>)>;
    using ReceiveCallbackFn = std::function<void(const std::vector<uint8_t>&)>;
    using ErrorCallbackFn = std::function<void(const Result&)>;

    // Event callbacks
    void acceptCallback(AcceptCallbackFn callback);
    void receiveCallback(ReceiveCallbackFn callback);
    void errorCallback(ErrorCallbackFn callback);

private:
    // Private constructor for internal use (e.g., Accept)
    explicit Socket(SOCKET_TYPE_NATIVE nativeSocket);

    // Factory method for creating sockets from native handles
    static std::unique_ptr<Socket> createFromNative(SOCKET_TYPE_NATIVE nativeSocket);

    SOCKET_TYPE_NATIVE m_socket;
    bool m_isBlocking;
    bool m_isListening{false};
    mutable std::mutex m_mutex;

    // Static members for automatic socket system management
    static std::atomic<int> s_socketCount;
    static std::mutex s_initMutex;
    
    // Private socket system management - automatic only
    static Result InitializeSocketSystem();
    static void CleanupSocketSystem();

    // Callbacks
    AcceptCallbackFn m_acceptCallback;
    ReceiveCallbackFn m_receiveCallback;
    ErrorCallbackFn m_errorCallback;
    
    // Async I/O members
    std::atomic<bool> m_asyncEnabled{false};
#ifdef _WIN32
    WSAOVERLAPPED m_sendOverlapped{};
    WSAOVERLAPPED m_recvOverlapped{};
    HANDLE m_completionPort{nullptr};
#else
    int m_epollFd{-1};
    struct epoll_event m_epollEvents[16];
#endif
    
    // Event loop members
    std::unique_ptr<std::thread> m_eventLoopThread;
    std::atomic<bool> m_eventLoopRunning{false};
    mutable std::mutex m_eventLoopMutex;
    
    // Event loop methods
    void eventLoopFunction();
    Result processSocketEvents();
    void handleAcceptEvent();
    void handleReceiveEvent();

    // Platform-specific helper methods (private)
    Result setSocketOption(int level, int option, const void* value, size_t length);
    Result getSocketOption(int level, int option, void* value, size_t* length) const;
    void updateLastError();
    std::pair<std::string, uint16_t> getSocketAddress(const struct sockaddr* addr) const;
    std::pair<Result, std::pair<std::string, uint16_t>> getSocketAddress() const;
    static std::string getAddressString(const struct sockaddr* addr);
};

} // namespace WebSocket
