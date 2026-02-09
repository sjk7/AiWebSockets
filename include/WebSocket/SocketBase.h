#pragma once

#include <memory>
#include <string>

namespace nob {

// Forward declarations to hide native socket types
class SocketImpl;

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
     * @return Reference to this socket
     */
    SocketBase& operator=(SocketBase&& other) noexcept;

    /**
     * @brief Copy constructor (deleted)
     */
    SocketBase(const SocketBase&) = delete;

    /**
     * @brief Copy assignment operator (deleted)
     */
    SocketBase& operator=(const SocketBase&) = delete;

    /**
     * @brief Check if socket is valid/connected
     * @return true if socket is valid, false otherwise
     */
    bool isValid() const;

    /**
     * @brief Check if socket is in blocking mode
     * @return true if blocking, false if non-blocking
     */
    bool isBlocking() const;

    /**
     * @brief Get local address
     * @return Local IP address as string
     */
    std::string localAddress() const;

    /**
     * @brief Get local port
     * @return Local port number
     */
    uint16_t localPort() const;

    /**
     * @brief Get remote address
     * @return Remote IP address as string
     */
    std::string remoteAddress() const;

    /**
     * @brief Get remote port
     * @return Remote port number
     */
    uint16_t remotePort() const;

    /**
     * @brief Check if async I/O is enabled
     * @return true if async I/O is enabled
     */
    bool isAsyncEnabled() const;

    /**
     * @brief Check if event loop is running
     * @return true if event loop is running
     */
    bool isEventLoopRunning() const;

private:
    // Pimpl pattern - hide all native socket details
    std::unique_ptr<SocketImpl> m_impl;
};

} // namespace nob
