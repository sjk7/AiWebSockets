#pragma once

#include <memory>
#include <string>

namespace nob {

// Forward declarations
class SocketImpl;
class Result;

// Platform-specific types - exposed for inheritance (but headers still hidden)
#ifdef _WIN32
struct NativeSocketTypes {
    using SocketType = void*;  // Opaque pointer type
    static constexpr void* INVALID_SOCKET = nullptr;
};
#else
struct NativeSocketTypes {
    using SocketType = int;
    static constexpr int INVALID_SOCKET = -1;
};
#endif

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

private:
    // Pimpl pattern - hide all native socket details
    std::unique_ptr<SocketImpl> m_impl;
};

} // namespace nob
