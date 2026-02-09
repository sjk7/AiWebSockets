#include "WebSocket/SocketBase.h"
#include "WebSocket/ErrorCodes.h"

// Native socket headers - ONLY in this implementation file
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
    
#ifdef _WIN32
    int result = closesocket(m_impl->socket);
#else
    int result = close(m_impl->socket);
#endif
    
    m_impl->socket = 
#ifdef _WIN32
        INVALID_SOCKET
#else
        -1
#endif
    ;
    m_impl->isValid = false;
    
    if (result != 0) {
        return Result(ErrorCode::socketCreateFailed, getLastSystemErrorCode());
    }
    
    return Result();
}

} // namespace nob
