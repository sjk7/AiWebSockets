#include "WebSocket/SocketBase.h"
#include "WebSocket/ErrorCodes.h"
#include "WebSocket/OS.h"

// All native socket headers are now included in OS.h

namespace nob {

// SocketImpl class - contains all native socket details
class SocketImpl {
public:
#ifdef _WIN32
    SOCKET socket;
#else
    int socket;
#endif
    bool isValid;
    
    SocketImpl() 
#ifdef _WIN32
        : socket(INVALID_SOCKET)
#else
        : socket(-1)
#endif
        , isValid(false) {}
    
    SocketImpl(
#ifdef _WIN32
        SOCKET nativeSocket
#else
        int nativeSocket
#endif
    ) : socket(nativeSocket), isValid(
#ifdef _WIN32
        nativeSocket != INVALID_SOCKET
#else
        nativeSocket != -1
#endif
    ) {}
    
    ~SocketImpl() {
        if (isValid) {
#ifdef _WIN32
            closesocket(socket);
#else
            close(socket);
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

SocketImpl* SocketBase::getImpl() const {
    return m_impl.get();
}

NativeSocketTypes::SocketType SocketBase::getNativeSocket() const {
    if (!m_impl) {
        return NativeSocketTypes::INVALID_SOCKET;
    }
#ifdef _WIN32
    return reinterpret_cast<NativeSocketTypes::SocketType>(m_impl->socket);
#else
    return static_cast<NativeSocketTypes::SocketType>(m_impl->socket);
#endif
}

void SocketBase::setNativeSocket(NativeSocketTypes::SocketType nativeSocket) {
    if (!m_impl) {
        return;
    }
#ifdef _WIN32
    m_impl->socket = reinterpret_cast<SOCKET>(nativeSocket);
#else
    m_impl->socket = static_cast<int>(nativeSocket);
#endif
    m_impl->isValid = (nativeSocket != NativeSocketTypes::INVALID_SOCKET);
}

Result SocketBase::createNativeSocket(int family, int type, int protocol) {
    if (!m_impl) {
        return Result(ErrorCode::invalidParameter, "Socket implementation not initialized");
    }
    
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

Result SocketBase::bindNativeSocket(const void* addr, int addrLen) {
    if (!m_impl || !m_impl->isValid) {
        return Result(ErrorCode::invalidParameter, "Socket not valid");
    }
    
    if (::bind(m_impl->socket, static_cast<const struct sockaddr*>(addr), addrLen) != 0) {
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

NativeSocketTypes::SocketType SocketBase::acceptNativeSocket(void* addr, int* addrLen) {
    if (!m_impl || !m_impl->isValid) {
        return NativeSocketTypes::INVALID_SOCKET;
    }
    
    auto clientSocket = ::accept(m_impl->socket, static_cast<struct sockaddr*>(addr), addrLen);
    if (clientSocket == 
#ifdef _WIN32
        INVALID_SOCKET
#else
        -1
#endif
    ) {
        return NativeSocketTypes::INVALID_SOCKET;
    }
    
#ifdef _WIN32
    return reinterpret_cast<NativeSocketTypes::SocketType>(clientSocket);
#else
    return static_cast<NativeSocketTypes::SocketType>(clientSocket);
#endif
}

Result SocketBase::connectNativeSocket(const void* addr, int addrLen) {
    if (!m_impl || !m_impl->isValid) {
        return Result(ErrorCode::invalidParameter, "Socket not valid");
    }
    
    if (::connect(m_impl->socket, static_cast<const struct sockaddr*>(addr), addrLen) != 0) {
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
        return Result(ErrorCode::socketCloseFailed, getLastSystemErrorCode());
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

Result SocketBase::getSocketNameNative(void* addr, int* addrLen) const {
    if (!isValid()) {
        return Result(ErrorCode::invalidParameter, "Socket not created");
    }

    if (getsockname(m_impl->socket, (struct sockaddr*)addr, (socklen_t*)addrLen) != 0) {
        return Result(ErrorCode::unknownError, getLastSystemErrorCode());
    }

    return Result();
}

Result SocketBase::getPeerNameNative(void* addr, int* addrLen) const {
    if (!isValid()) {
        return Result(ErrorCode::invalidParameter, "Socket not created");
    }

    if (getpeername(m_impl->socket, (struct sockaddr*)addr, (socklen_t*)addrLen) != 0) {
        return Result(ErrorCode::unknownError, getLastSystemErrorCode());
    }

    return Result();
}

Result SocketBase::setBlockingNative(bool blocking) {
    if (!isValid()) {
        return Result(ErrorCode::invalidParameter, "Socket not created");
    }

#ifdef _WIN32
    u_long mode = blocking ? 0 : 1;
    int result = ioctlsocket(m_impl->socket, FIONBIO, &mode);
    if (result != 0) {
        return Result(ErrorCode::socketSetOptionFailed, getLastSystemErrorCode());
    }
#else
    int flags = fcntl(m_impl->socket, F_GETFL, 0);
    if (flags == -1) {
        return Result(ErrorCode::socketSetOptionFailed, getLastSystemErrorCode());
    }

    int result = fcntl(m_impl->socket, F_SETFL, blocking ? (flags & ~O_NONBLOCK) : (flags | O_NONBLOCK));
    if (result == -1) {
        return Result(ErrorCode::socketSetOptionFailed, getLastSystemErrorCode());
    }
#endif

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

} // namespace nob
