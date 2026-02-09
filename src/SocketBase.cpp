#include "WebSocket/SocketBase.h"
#include "WebSocket/OS.h"
#include "WebSocket/ErrorCodes.h"
#include <memory>
#include <cstring>
#include <algorithm>
#include <mutex>

// All native socket headers are now included in OS.h

namespace nob {

// SocketImpl class - contains all native socket details
class SocketImpl {
public:
#ifdef _WIN32
    SOCKET socket;
    HANDLE completionPort;
    WSAOVERLAPPED sendOverlapped;
    WSAOVERLAPPED recvOverlapped;
#else
    int socket;
    int epollFd;
    struct epoll_event epollEvents[16];
#endif
    bool isValid;
    bool asyncEnabled;

    SocketImpl() 
#ifdef _WIN32
        : socket(INVALID_SOCKET)
#else
        : socket(-1)
#endif
        , isValid(false), asyncEnabled(false) {
#ifdef _WIN32
        completionPort = nullptr;
        memset(&sendOverlapped, 0, sizeof(sendOverlapped));
        memset(&recvOverlapped, 0, sizeof(recvOverlapped));
#else
        epollFd = -1;
        memset(epollEvents, 0, sizeof(epollEvents));
#endif
    }
    
    ~SocketImpl() {
        if (isValid) {
#ifdef _WIN32
            closesocket(socket);
            if (completionPort) {
                CloseHandle(completionPort);
            }
#else
            close(socket);
            if (epollFd != -1) {
                close(epollFd);
            }
#endif
        }
    }
};

// SocketBase implementation using pimpl pattern
SocketBase::SocketBase() : m_impl(std::make_unique<SocketImpl>()) {}

SocketBase::~SocketBase() = default;

SocketBase::SocketBase(SocketBase&& other) noexcept = default;
SocketBase& SocketBase::operator=(SocketBase&& other) noexcept = default;

bool SocketBase::isValid() const {
    return m_impl && m_impl->isValid;
}

NativeSocketTypes::SocketType SocketBase::getNativeSocket() const {
    if (!m_impl) {
#ifdef _WIN32
        return reinterpret_cast<NativeSocketTypes::SocketType>(INVALID_SOCKET);
#else
        return static_cast<NativeSocketTypes::SocketType>(-1);
#endif
    }
#ifdef _WIN32
    return reinterpret_cast<NativeSocketTypes::SocketType>(m_impl->socket);
#else
    return static_cast<NativeSocketTypes::SocketType>(m_impl->socket);
#endif
}

void SocketBase::setNativeSocket(NativeSocketTypes::SocketType nativeSocket) {
    if (!m_impl) {
        m_impl = std::make_unique<SocketImpl>();
    }

#ifdef _WIN32
    m_impl->socket = reinterpret_cast<SOCKET>(nativeSocket);
    m_impl->isValid = (reinterpret_cast<SOCKET>(nativeSocket) != INVALID_SOCKET);
#else
    m_impl->socket = static_cast<int>(nativeSocket);
    m_impl->isValid = (static_cast<int>(nativeSocket) != -1);
#endif
}

Result SocketBase::createNativeSocket(int family, int type, int protocol) {
    if (!m_impl) {
        return Result(ErrorCode::invalidParameter, "Socket implementation not initialized");
    }
    
#ifdef _WIN32
    // Initialize Winsock if not already initialized
    static bool wsaInitialized = false;
    static std::mutex wsaMutex;
    
    if (!wsaInitialized) {
        std::lock_guard<std::mutex> lock(wsaMutex);
        if (!wsaInitialized) {
            WSADATA wsaData;
            int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
            if (result != 0) {
                return Result(ErrorCode::socketCreateFailed, "WSAStartup failed: " + std::to_string(result));
            }
            wsaInitialized = true;
            
            // Register cleanup function to be called at program exit
            static struct WSACleanupGuard {
                ~WSACleanupGuard() {
                    WSACleanup();
                }
            } cleanupGuard;
        }
    }
#endif
    
    m_impl->socket = socket(family, type, protocol);
    if (m_impl->socket == 
#ifdef _WIN32
        INVALID_SOCKET
#else
        -1
#endif
    ) {
        return Result(ErrorCode::socketCreateFailed, getLastSystemErrorCode());
    }
    
    m_impl->isValid = true;
    return Result();
}

Result SocketBase::bindNativeSocket(const SocketAddress& addr) {
    if (!m_impl || !m_impl->isValid) {
        return Result(ErrorCode::invalidParameter, "Socket not valid");
    }

    // Convert SocketAddress to sockaddr_in
    struct sockaddr_in sockAddr;
    sockAddr.sin_family = addr.family;
    sockAddr.sin_addr.s_addr = addr.addr;
    sockAddr.sin_port = addr.port;
    memset(sockAddr.sin_zero, 0, sizeof(sockAddr.sin_zero));

    // Native socket operation (hidden from public interface)
    if (::bind(m_impl->socket, reinterpret_cast<const struct sockaddr*>(&sockAddr), sizeof(sockAddr)) != 0) {
        return Result(ErrorCode::socketBindFailed, getLastSystemErrorCode());
    }

    return Result();
}

Result SocketBase::listenNativeSocket(int backlog) {
    if (!m_impl || !m_impl->isValid) {
        return Result(ErrorCode::invalidParameter, "Socket not valid");
    }
    
    if (::listen(m_impl->socket, backlog) != 0) {
        return Result(ErrorCode::socketListenFailed, getLastSystemErrorCode());
    }
    
    return Result();
}

NativeSocketTypes::SocketType SocketBase::acceptNativeSocket(SocketAddress& addr) {
    if (!isValid()) {
#ifdef _WIN32
        return reinterpret_cast<NativeSocketTypes::SocketType>(INVALID_SOCKET);
#else
        return static_cast<NativeSocketTypes::SocketType>(-1);
#endif
    }

    // Convert SocketAddress to sockaddr_in
    struct sockaddr_in sockAddr;
    sockAddr.sin_family = addr.family;
    sockAddr.sin_addr.s_addr = addr.addr;
    sockAddr.sin_port = addr.port;
    memset(sockAddr.sin_zero, 0, sizeof(sockAddr.sin_zero));

    int addrLen = sizeof(sockAddr);
    SOCKET nativeClientSocket = ::accept(m_impl->socket, reinterpret_cast<struct sockaddr*>(&sockAddr), &addrLen);
    
    if (nativeClientSocket == INVALID_SOCKET) {
        return INVALID_SOCKET_NATIVE;
    }
    
    // Convert native socket to abstract type
    NativeSocketTypes::SocketType clientSocket = 
#ifdef _WIN32
        reinterpret_cast<NativeSocketTypes::SocketType>(nativeClientSocket);
#else
        static_cast<NativeSocketTypes::SocketType>(nativeClientSocket);
#endif

    // Update the SocketAddress with the client address
    addr.family = sockAddr.sin_family;
    addr.addr = sockAddr.sin_addr.s_addr;
    addr.port = sockAddr.sin_port;

    // Create new SocketImpl for the accepted socket
    auto newImpl = std::make_unique<SocketImpl>();
    
    // Convert abstract socket type back to native for SocketImpl
#ifdef _WIN32
    newImpl->socket = reinterpret_cast<SOCKET>(clientSocket);
#else
    newImpl->socket = static_cast<int>(clientSocket);
#endif
    
    newImpl->isValid = true;
    newImpl->asyncEnabled = m_impl->asyncEnabled;
    
    // Copy async I/O state if needed
    if (m_impl->asyncEnabled) {
        newImpl->completionPort = m_impl->completionPort;
        memcpy(&newImpl->sendOverlapped, &m_impl->sendOverlapped, sizeof(m_impl->sendOverlapped));
        memcpy(&newImpl->recvOverlapped, &m_impl->recvOverlapped, sizeof(m_impl->recvOverlapped));
    }

    // Replace current impl with new one
    m_impl = std::move(newImpl);

    return reinterpret_cast<NativeSocketTypes::SocketType>(clientSocket);
}

Result SocketBase::connectNativeSocket(const SocketAddress& addr) {
    if (!m_impl || !m_impl->isValid) {
        return Result(ErrorCode::invalidParameter, "Socket not valid");
    }
    
    // Convert SocketAddress to sockaddr_in
    struct sockaddr_in sockAddr;
    sockAddr.sin_family = addr.family;
    sockAddr.sin_addr.s_addr = addr.addr;
    sockAddr.sin_port = addr.port;
    memset(sockAddr.sin_zero, 0, sizeof(sockAddr.sin_zero));
    
    // Native socket operation (hidden from public interface)
    if (::connect(m_impl->socket, reinterpret_cast<const struct sockaddr*>(&sockAddr), sizeof(sockAddr)) != 0) {
        return Result(ErrorCode::socketConnectFailed, getLastSystemErrorCode());
    }
    
    return Result();
}

Result SocketBase::closeNativeSocket() {
    if (!m_impl || !m_impl->isValid) {
        return Result();
    }
    
    auto socket = m_impl->socket;
    m_impl->isValid = false;

#ifdef _WIN32
    int result = closesocket(socket);
#else
    int result = close(socket);
#endif

    if (result != 0) {
        return Result(ErrorCode::socketCreateFailed, getLastSystemErrorCode());
    }

    return Result();
}

Result SocketBase::sendNativeSocket(const void* data, size_t length, size_t* bytesSent) {
    if (!isValid()) {
        return Result(ErrorCode::invalidParameter, "Socket not created");
    }

    if (!data || length == 0 || !bytesSent) {
        return Result(ErrorCode::invalidParameter, "Invalid parameters");
    }

#ifdef _WIN32
    int result = ::send(m_impl->socket, (const char*)data, (int)length, 0);
#else
    ssize_t result = ::send(m_impl->socket, data, length, 0);
#endif

    if (result < 0) {
        *bytesSent = 0;
        return Result(ErrorCode::socketSendFailed, getLastSystemErrorCode());
    }

    *bytesSent = static_cast<size_t>(result);
    return Result();
}

Result SocketBase::receiveNativeSocket(void* buffer, size_t bufferSize, size_t* bytesReceived) {
    if (!isValid()) {
        return Result(ErrorCode::invalidParameter, "Socket not created");
    }

    if (!buffer || bufferSize == 0 || !bytesReceived) {
        return Result(ErrorCode::invalidParameter, "Invalid parameters");
    }

#ifdef _WIN32
    int result = recv(m_impl->socket, (char*)buffer, (int)bufferSize, 0);
#else
    ssize_t result = recv(m_impl->socket, buffer, bufferSize, 0);
#endif

    if (result < 0) {
        *bytesReceived = 0;
        return Result(ErrorCode::socketReceiveFailed, getLastSystemErrorCode());
    }

    *bytesReceived = static_cast<size_t>(result);
    return Result();
}

Result SocketBase::shutdownNativeSocket(int how) {
    if (!isValid()) {
        return Result();
    }

#ifdef _WIN32
    int result = ::shutdown(m_impl->socket, how);
#else
    int result = ::shutdown(m_impl->socket, how);
#endif

    if (result != 0) {
        return Result(ErrorCode::unknownError, getLastSystemErrorCode());
    }

    return Result();
}

Result SocketBase::setSocketOptionNative(int level, int option, const void* value, size_t length) {
    if (!isValid()) {
        return Result(ErrorCode::invalidParameter, "Socket not created");
    }

    if (setsockopt(m_impl->socket, level, option, (const char*)value, (int)length) != 0) {
        return Result(ErrorCode::socketSetOptionFailed, getLastSystemErrorCode());
    }

    return Result();
}

Result SocketBase::getSocketOptionNative(int level, int option, void* value, size_t* length) const {
    if (!isValid()) {
        return Result(ErrorCode::invalidParameter, "Socket not created");
    }

    socklen_t len = (socklen_t)*length;
    if (getsockopt(m_impl->socket, level, option, (char*)value, &len) != 0) {
        return Result(ErrorCode::socketSetOptionFailed, getLastSystemErrorCode());
    }

    *length = len;
    return Result();
}

Result SocketBase::getSocketNameNative(SocketAddress& addr) const {
    if (!isValid()) {
        return Result(ErrorCode::invalidParameter, "Socket not created");
    }

    // Convert SocketAddress to sockaddr_in
    struct sockaddr_in sockAddr;
    sockAddr.sin_family = addr.family;
    sockAddr.sin_addr.s_addr = addr.addr;
    sockAddr.sin_port = addr.port;
    memset(sockAddr.sin_zero, 0, sizeof(sockAddr.sin_zero));

    int addrLen = sizeof(sockAddr);
    // Native socket operation (hidden from public interface)
    if (getsockname(m_impl->socket, reinterpret_cast<struct sockaddr*>(&sockAddr), &addrLen) != 0) {
        return Result(ErrorCode::unknownError, getLastSystemErrorCode());
    }

    // Update the SocketAddress with the actual socket address
    addr.family = sockAddr.sin_family;
    addr.addr = sockAddr.sin_addr.s_addr;
    addr.port = sockAddr.sin_port;

    return Result();
}

Result SocketBase::setBlocking(bool blocking) {
    if (!isValid()) {
        return Result(ErrorCode::invalidParameter, "Socket not created");
    }

#ifdef _WIN32
    u_long mode = blocking ? 0 : 1; // 0 = blocking, 1 = non-blocking
    int result = ioctlsocket(m_impl->socket, FIONBIO, &mode);
#else
    int flags = fcntl(m_impl->socket, F_GETFL, 0);
    if (flags == -1) {
        return Result(ErrorCode::unknownError, getLastSystemErrorCode());
    }
    
    int newFlags = blocking ? (flags & ~O_NONBLOCK) : (flags | O_NONBLOCK);
    int result = fcntl(m_impl->socket, F_SETFL, newFlags);
#endif

    if (result == -1) {
        return Result(ErrorCode::unknownError, getLastSystemErrorCode());
    }

    return Result();
}

Result SocketBase::selectNativeSocket(int timeoutMs, bool* canRead, bool* canWrite) const {
    if (!isValid()) {
        return Result(ErrorCode::invalidParameter, "Socket not created");
    }

    fd_set readfds, writefds;
    FD_ZERO(&readfds);
    FD_ZERO(&writefds);

    FD_SET(m_impl->socket, &readfds);
    FD_SET(m_impl->socket, &writefds);

    struct timeval timeout;
    timeout.tv_sec = timeoutMs / 1000;
    timeout.tv_usec = (timeoutMs % 1000) * 1000;

    int selectResult = select(m_impl->socket + 1, &readfds, &writefds, nullptr, &timeout);

    if (selectResult < 0) {
        *canRead = *canWrite = false;
        return Result(ErrorCode::socketReceiveFailed, getLastSystemErrorCode());
    }

    if (selectResult == 0) {
        *canRead = *canWrite = false;
        return Result();
    }

    *canRead = FD_ISSET(m_impl->socket, &readfds);
    *canWrite = FD_ISSET(m_impl->socket, &writefds);
    return Result();
}

Result SocketBase::initializeAsyncIO() {
    if (!isValid()) {
        return Result(ErrorCode::invalidParameter, "Socket not created");
    }

    if (m_impl->asyncEnabled) {
        return Result(); // Already initialized
    }

#ifdef _WIN32
    // Create I/O Completion Port
    m_impl->completionPort = CreateIoCompletionPort((HANDLE)m_impl->socket, nullptr, (ULONG_PTR)this, 0);
    if (m_impl->completionPort == nullptr) {
        return Result(ErrorCode::unknownError, "Failed to create completion port");
    }

    // Initialize overlapped structures
    memset(&m_impl->sendOverlapped, 0, sizeof(m_impl->sendOverlapped));
    memset(&m_impl->recvOverlapped, 0, sizeof(m_impl->recvOverlapped));

#else
    // Create epoll instance
    m_impl->epollFd = epoll_create1(0);
    if (m_impl->epollFd == -1) {
        return Result(ErrorCode::unknownError, "Failed to create epoll instance");
    }

    // Add socket to epoll
    struct epoll_event event;
    event.events = EPOLLIN | EPOLLOUT;
    event.data.fd = m_impl->socket;
    if (epoll_ctl(m_impl->epollFd, EPOLL_CTL_ADD, m_impl->socket, &event) == -1) {
        close(m_impl->epollFd);
        m_impl->epollFd = -1;
        return Result(ErrorCode::unknownError, "Failed to add socket to epoll");
    }
#endif

    m_impl->asyncEnabled = true;
    return Result();
}

Result SocketBase::cleanupAsyncIO() {
    if (!m_impl->asyncEnabled) {
        return Result(); // Not initialized
    }

#ifdef _WIN32
    if (m_impl->completionPort) {
        CloseHandle(m_impl->completionPort);
        m_impl->completionPort = nullptr;
    }
#else
    if (m_impl->epollFd != -1) {
        close(m_impl->epollFd);
        m_impl->epollFd = -1;
    }
#endif

    m_impl->asyncEnabled = false;
    return Result();
}

Result SocketBase::sendAsync(const void* data, size_t length, size_t* bytesSent) {
    if (!isValid() || !m_impl->asyncEnabled) {
        return Result(ErrorCode::invalidParameter, "Socket not created or async not enabled");
    }

    if (!data || length == 0 || !bytesSent) {
        return Result(ErrorCode::invalidParameter, "Invalid parameters");
    }

#ifdef _WIN32
    WSABUF wsaBuf;
    wsaBuf.buf = (CHAR*)data;
    wsaBuf.len = (ULONG)length;

    DWORD bytesSentWin = 0;
    int result = WSASend(m_impl->socket, &wsaBuf, 1, &bytesSentWin, 0, &m_impl->sendOverlapped, nullptr);

    if (result == SOCKET_ERROR) {
        int error = WSAGetLastError();
        if (error != WSA_IO_PENDING) {
            *bytesSent = 0;
            return Result(ErrorCode::socketSendFailed, error);
        }
        // Operation pending - will be completed later
        *bytesSent = 0;
        return Result();
    }

    *bytesSent = bytesSentWin;
    return Result();
#else
    ssize_t result = send(m_impl->socket, data, length, MSG_DONTWAIT);
    if (result < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            *bytesSent = 0;
            return Result(); // Would block, try again later
        }
        *bytesSent = 0;
        return Result(ErrorCode::socketSendFailed, errno);
    }

    *bytesSent = result;
    return Result();
#endif
}

Result SocketBase::receiveAsync(void* buffer, size_t bufferSize, size_t* bytesReceived) {
    if (!isValid() || !m_impl->asyncEnabled) {
        return Result(ErrorCode::invalidParameter, "Socket not created or async not enabled");
    }

    if (!buffer || bufferSize == 0 || !bytesReceived) {
        return Result(ErrorCode::invalidParameter, "Invalid parameters");
    }

#ifdef _WIN32
    WSABUF wsaBuf;
    wsaBuf.buf = (CHAR*)buffer;
    wsaBuf.len = (ULONG)bufferSize;

    DWORD bytesReceivedWin = 0;
    DWORD flags = 0;
    int result = WSARecv(m_impl->socket, &wsaBuf, 1, &bytesReceivedWin, &flags, &m_impl->recvOverlapped, nullptr);

    if (result == SOCKET_ERROR) {
        int error = WSAGetLastError();
        if (error != WSA_IO_PENDING) {
            *bytesReceived = 0;
            return Result(ErrorCode::socketReceiveFailed, error);
        }
        // Operation pending - will be completed later
        *bytesReceived = 0;
        return Result();
    }

    *bytesReceived = bytesReceivedWin;
    return Result();
#else
    ssize_t result = recv(m_impl->socket, buffer, bufferSize, MSG_DONTWAIT);
    if (result < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            *bytesReceived = 0;
            return Result(); // Would block, try again later
        }
        *bytesReceived = 0;
        return Result(ErrorCode::socketReceiveFailed, errno);
    }

    *bytesReceived = result;
    return Result();
#endif
}

Result SocketBase::resolveHostname(const std::string& hostname, SocketAddress& addr) {
    if (!addr.family) {
        return Result(ErrorCode::invalidParameter, "Invalid address parameters");
    }

    // Use getaddrinfo for DNS resolution (this is the modern approach)
    struct addrinfo hints = {};
    hints.ai_family = AF_INET; // Force IPv4 for simplicity
    hints.ai_socktype = SOCK_STREAM;
    
    struct addrinfo* results = nullptr;
    int ret = getaddrinfo(hostname.c_str(), nullptr, &hints, &results);
    
    if (ret != 0 || !results) {
        return Result(ErrorCode::unknownError, getLastSystemErrorCode());
    }
    
    // Copy the first result (IPv4)
    if (results->ai_family == AF_INET) {
        struct sockaddr_in* sockAddr = reinterpret_cast<struct sockaddr_in*>(results->ai_addr);
        addr.family = results->ai_family;
        addr.addr = sockAddr->sin_addr.s_addr;
        addr.port = sockAddr->sin_port;
    }
    
    freeaddrinfo(results);
    return Result();
}

Result SocketBase::getPeerNameNative(SocketAddress& addr) const {
    if (!isValid()) {
        return Result(ErrorCode::invalidParameter, "Socket not created");
    }

    // Convert SocketAddress to sockaddr_in
    struct sockaddr_in sockAddr;
    sockAddr.sin_family = addr.family;
    sockAddr.sin_addr.s_addr = addr.addr;
    sockAddr.sin_port = addr.port;
    memset(sockAddr.sin_zero, 0, sizeof(sockAddr.sin_zero));

    int addrLen = sizeof(sockAddr);
    // Native socket operation (hidden from public interface)
    if (getpeername(m_impl->socket, reinterpret_cast<struct sockaddr*>(&sockAddr), &addrLen) != 0) {
        return Result(ErrorCode::unknownError, getLastSystemErrorCode());
    }

    // Update the SocketAddress with the actual peer address
    addr.family = sockAddr.sin_family;
    addr.addr = sockAddr.sin_addr.s_addr;
    addr.port = sockAddr.sin_port;

    return Result();
}

bool SocketBase::isAsyncEnabled() const {
    return m_impl && m_impl->asyncEnabled;
}

} // namespace nob
