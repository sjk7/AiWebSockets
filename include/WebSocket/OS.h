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
#include <netdb.h>
#endif

namespace nob {

// Platform-specific types
#ifdef _WIN32
using SOCKET_TYPE_NATIVE = SOCKET;
using SOCKET_ERROR_TYPE = int;
static constexpr SOCKET_ERROR_TYPE SOCK_ERROR = SOCKET_ERROR;

// Opaque types for firewall - these hide the actual native types
namespace detail {
    using OpaqueSocketType = SOCKET;  // Still SOCKET but only visible in implementation
    static constexpr OpaqueSocketType OPAQUE_INVALID_SOCKET = INVALID_SOCKET;
}
#else
using SOCKET_TYPE_NATIVE = int;
static const SOCKET_TYPE_NATIVE INVALID_SOCKET_NATIVE = -1;
using SOCKET_ERROR_TYPE = int;
static constexpr SOCKET_ERROR_TYPE SOCK_ERROR = -1;

// Opaque types for firewall - these hide the actual native types
namespace detail {
    using OpaqueSocketType = int;  // Still int but only visible in implementation
    static constexpr OpaqueSocketType OPAQUE_INVALID_SOCKET = -1;
}
#endif

// Platform-specific address structures
using sockaddr_storage = struct sockaddr_storage;

} // namespace nob
