#include "WebSocket/ErrorCodes.h"
#include <cstdio>

#ifdef _WIN32
#include <winsock2.h>
#else
#include <errno.h>
#include <string.h>
#endif

namespace WebSocket {

const char* getErrorCodeString(ERROR_CODE code) {
    switch (code) {
        case ERROR_CODE::SUCCESS:
            return "Success";
        case ERROR_CODE::SOCKET_CREATE_FAILED:
            return "Socket creation failed";
        case ERROR_CODE::SOCKET_BIND_FAILED:
            return "Socket bind failed";
        case ERROR_CODE::SOCKET_LISTEN_FAILED:
            return "Socket listen failed";
        case ERROR_CODE::SOCKET_ACCEPT_FAILED:
            return "Socket accept failed";
        case ERROR_CODE::SOCKET_CONNECT_FAILED:
            return "Socket connect failed";
        case ERROR_CODE::SOCKET_SEND_FAILED:
            return "Socket send failed";
        case ERROR_CODE::SOCKET_RECEIVE_FAILED:
            return "Socket receive failed";
        case ERROR_CODE::SOCKET_SET_OPTION_FAILED:
            return "Socket set option failed";
        case ERROR_CODE::SOCKET_GETSOCKNAME_FAILED:
            return "Socket getsockname failed";
        case ERROR_CODE::SOCKET_ADDRESS_PARSE_FAILED:
            return "Socket address parse failed";
        case ERROR_CODE::INVALID_PARAMETER:
            return "Invalid parameter";
        case ERROR_CODE::MEMORY_ALLOCATION_FAILED:
            return "Memory allocation failed";
        case ERROR_CODE::WEBSOCKET_HANDSHAKE_FAILED:
            return "WebSocket handshake failed";
        case ERROR_CODE::WEBSOCKET_FRAME_PARSE_FAILED:
            return "WebSocket frame parse failed";
        case ERROR_CODE::WEBSOCKET_INVALID_OPCODE:
            return "WebSocket invalid opcode";
        case ERROR_CODE::WEBSOCKET_PAYLOAD_TOO_LARGE:
            return "WebSocket payload too large";
        case ERROR_CODE::WEBSOCKET_CONNECTION_CLOSED:
            return "WebSocket connection closed";
        case ERROR_CODE::THREAD_CREATION_FAILED:
            return "Thread creation failed";
        case ERROR_CODE::UNKNOWN_ERROR:
        default:
            return "Unknown error";
    }
}

// Optimized version - returns just the error code number
int getLastSystemErrorCode() {
#ifdef _WIN32
    return WSAGetLastError();
#else
    return errno;
#endif
}

// Full string version - use sparingly for logging
std::string getSystemErrorMessage(int errorCode) {
    if (errorCode == 0) {
        return "";
    }
    
    std::string error;
    
#ifdef _WIN32
    LPSTR messageBuffer = nullptr;
    size_t size = FormatMessageA(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL, errorCode, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPSTR)&messageBuffer, 0, NULL);
    
    if (size > 0 && messageBuffer) {
        error = std::string(messageBuffer, size);
        LocalFree(messageBuffer);
    } else {
        error = "Unknown Windows error: " + std::to_string(errorCode);
    }
#else
    char buffer[256];
    if (strerror_r(errorCode, buffer, sizeof(buffer)) == 0) {
        error = buffer;
    } else {
        error = "Unknown POSIX error: " + std::to_string(errorCode);
    }
#endif

    return error;
}

// Legacy function for backward compatibility - now uses optimized version
std::string GetLastSystemError() {
    return getSystemErrorMessage(getLastSystemErrorCode());
}

// Result class implementation
std::string Result::generateErrorMessage() const {
    std::string result = getErrorCodeString(m_errorCode);
    
    // If we have a system error code, append the system error message
    if (m_systemErrorCode != 0) {
        std::string systemError = getSystemErrorMessage(m_systemErrorCode);
        if (!systemError.empty()) {
            result += ": " + systemError;
        } else {
            result += " (System error code: " + std::to_string(m_systemErrorCode) + ")";
        }
    }
    
    return result;
}

} // namespace WebSocket
