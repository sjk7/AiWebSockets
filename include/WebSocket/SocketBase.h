#pragma once

#include <memory>
#include <string>
#include <chrono>
#include <cstdint>

namespace nob {

// Forward declarations
class SocketImpl;
class Result;

// Native socket type definitions (abstract compliant)
namespace NativeSocketTypes {
#ifdef _WIN32
    using SocketType = void*;  // Opaque pointer type for Windows
#else
    using SocketType = int;     // Opaque integer type for Unix
#endif
}

// Socket constants (abstract compliant)
constexpr int AF_INET_VALUE = 2;
constexpr int SOCK_STREAM_VALUE = 1;
constexpr int SOL_SOCKET_VALUE = 1;
constexpr int SO_ERROR_VALUE = 100;

// Constants for invalid sockets
constexpr NativeSocketTypes::SocketType INVALID_SOCKET_NATIVE = 
#ifdef _WIN32
    nullptr;
#else
    static_cast<NativeSocketTypes::SocketType>(-1);
#endif

/**
 * @brief SocketBase - Compiler abstract class that shields users from native socket headers
 * 
 * This class provides a platform-independent interface for socket operations.
 * It hides all native socket details behind the compiler abstract using Pimpl pattern.
 * No native socket headers are exposed in the public interface.
 */
class SocketBase {
public:
    SocketBase();
    virtual ~SocketBase();
    
    // Disable copying, allow moving
    SocketBase(const SocketBase&) = delete;
    SocketBase& operator=(const SocketBase&) = delete;
    SocketBase(SocketBase&& other) noexcept;
    SocketBase& operator=(SocketBase&& other) noexcept;
    
    // Basic socket operations
    bool isValid() const;
    NativeSocketTypes::SocketType getNativeSocket() const;
    void setNativeSocket(NativeSocketTypes::SocketType nativeSocket);
    
    // Type-safe address structures (abstract compliant)
    struct SocketAddress {
        int family;
        uint32_t addr;  // IPv4 address in network byte order
        uint16_t port;  // Port in network byte order
        
        SocketAddress() : family(AF_INET_VALUE), addr(0), port(0) {}
        SocketAddress(uint32_t address, uint16_t port) : family(AF_INET_VALUE), addr(address), port(port) {}
        
        // Helper methods for common operations
        void setAddress(uint32_t address) { addr = address; }
        void setPort(uint16_t port) { this->port = port; }
        void setFamily(int family) { this->family = family; }
        uint32_t getAddress() const { return addr; }
        uint16_t getPort() const { return port; }
    };
    
    // Native socket operations (abstract compliant)
    Result createNativeSocket(int family, int type, int protocol);
    Result bindNativeSocket(const SocketAddress& addr);
    Result listenNativeSocket(int backlog);
    NativeSocketTypes::SocketType acceptNativeSocket(SocketAddress& addr);
    Result connectNativeSocket(const SocketAddress& addr);
    Result closeNativeSocket();

    // Additional native operations that need to be moved from Socket.cpp
    Result sendNativeSocket(const void* data, size_t length, size_t* bytesSent);
    Result receiveNativeSocket(void* buffer, size_t bufferSize, size_t* bytesReceived);
    Result shutdownNativeSocket(int how);
    Result setSocketOptionNative(int level, int option, const void* value, size_t length);
    Result getSocketOptionNative(int level, int option, void* value, size_t* length) const;
    Result getSocketNameNative(SocketAddress& addr) const;
    Result getPeerNameNative(SocketAddress& addr) const;
    Result setBlocking(bool blocking);
    Result selectNativeSocket(int timeoutMs, bool* canRead, bool* canWrite) const;
    
    // DNS resolution methods
    Result resolveHostname(const std::string& hostname, SocketAddress& addr);
    
    // Async I/O operations
    Result initializeAsyncIO();
    Result cleanupAsyncIO();
    Result sendAsync(const void* data, size_t length, size_t* bytesSent);
    Result receiveAsync(void* buffer, size_t bufferSize, size_t* bytesReceived);
    bool isAsyncEnabled() const;

private:
    // Pimpl pattern - hide all native socket details
    std::unique_ptr<SocketImpl> m_impl;
};

} // namespace nob
