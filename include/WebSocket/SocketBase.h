#pragma once

#include <memory>
#include "OS.h"  // Include OS.h to get the correct opaque types

namespace nob {

// Forward declarations
class SocketImpl;
class Result;

/**
 * @brief SocketBase - Compiler firewall class that shields users from native socket headers
 * 
 * This class provides a clean interface without exposing native socket types
 * or platform-specific headers to the user. All native socket operations
 * are handled through the pimpl pattern.
 */
class SocketBase {
public:
    /**
     * @brief Default constructor - creates an invalid socket
     */
    SocketBase();

    /**
     * @brief Destructor - ensures proper cleanup
     */
    ~SocketBase();

    /**
     * @brief Move constructor
     * @param other Socket to move from
     */
    SocketBase(SocketBase&& other) noexcept;

    /**
     * @brief Move assignment operator
     * @param other Socket to move from
     * @return Reference to this Socket
     */
    SocketBase& operator=(SocketBase&& other) noexcept;

    /**
     * @brief Check if socket is valid
     * @return True if socket is valid, false otherwise
     */
    bool isValid() const;

protected:
    /**
     * @brief Get the implementation pointer
     * @return Pointer to SocketImpl
     */
    SocketImpl* getImpl() const;

    /**
     * @brief Get native socket handle
     * @return Native socket handle
     */
    NativeSocketTypes::SocketType getNativeSocket() const;

    /**
     * @brief Set native socket handle
     * @param nativeSocket Native socket handle
     */
    void setNativeSocket(NativeSocketTypes::SocketType nativeSocket);

    /**
     * @brief Create native socket
     * @param family Address family
     * @param type Socket type
     * @param protocol Protocol
     * @return Result of operation
     */
    Result createNativeSocket(int family, int type, int protocol);

    /**
     * @brief Bind native socket
     * @param addr Socket address
     * @param addrLen Address length
     * @return Result of operation
     */
    Result bindNativeSocket(const void* addr, int addrLen);

    /**
     * @brief Listen on native socket
     * @param backlog Backlog size
     * @return Result of operation
     */
    Result listenNativeSocket(int backlog);

    /**
     * @brief Accept connection on native socket
     * @param addr Client address
     * @param addrLen Address length
     * @return Native socket handle
     */
    NativeSocketTypes::SocketType acceptNativeSocket(void* addr, int* addrLen);

    /**
     * @brief Connect native socket
     * @param addr Socket address
     * @param addrLen Address length
     * @return Result of operation
     */
    Result connectNativeSocket(const void* addr, int addrLen);

    /**
     * @brief Close native socket
     * @return Result of operation
     */
    Result closeNativeSocket();

    // Additional native operations that need to be moved from Socket.cpp
    Result sendNativeSocket(const void* data, size_t length, size_t* bytesSent);
    Result receiveNativeSocket(void* buffer, size_t bufferSize, size_t* bytesReceived);
    Result shutdownNativeSocket(int how);
    Result setSocketOptionNative(int level, int option, const void* value, size_t length);
    Result getSocketOptionNative(int level, int option, void* value, size_t* length) const;
    Result getSocketNameNative(void* addr, int* addrLen) const;
    Result getPeerNameNative(void* addr, int* addrLen) const;
    Result setBlocking(bool blocking);
    Result selectNativeSocket(int timeoutMs, bool* canRead, bool* canWrite) const;
    
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
