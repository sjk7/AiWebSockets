#pragma once

// Platform-specific headers - ONLY in this file
// This is the ONLY place where native OS/socket headers should be included
#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <mswsock.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/epoll.h>
#endif

namespace nob {

// Platform-specific types
#ifdef _WIN32
using SOCKET_TYPE_NATIVE = SOCKET;
static const SOCKET_TYPE_NATIVE INVALID_SOCKET_NATIVE = INVALID_SOCKET;
using SOCKET_ERROR_TYPE = int;
static constexpr SOCKET_ERROR_TYPE SOCK_ERROR = SOCKET_ERROR;
#else
using SOCKET_TYPE_NATIVE = int;
static const SOCKET_TYPE_NATIVE INVALID_SOCKET_NATIVE = -1;
using SOCKET_ERROR_TYPE = int;
static constexpr SOCKET_ERROR_TYPE SOCK_ERROR = -1;
#endif

// Opaque type for headers (only used in SocketBase.h)
namespace nob {
    namespace detail {
        #ifdef _WIN32
        using OpaqueSocketType = void*;
        static constexpr OpaqueSocketType OPAQUE_INVALID_SOCKET = nullptr;
        #else
        using OpaqueSocketType = int;
        static constexpr OpaqueSocketType OPAQUE_INVALID_SOCKET = -1;
        #endif
    }
}

// Platform-specific address structures
using sockaddr_storage = struct sockaddr_storage;

} // namespace nob
