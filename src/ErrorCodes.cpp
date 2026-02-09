#include "WebSocket/ErrorCodes.h"
#include <cstdio>

#ifdef _WIN32
#include <winsock2.h>
#else
#include <errno.h>
#include <string.h>
#endif

namespace WebSocket {

const char* getErrorCodeString(ErrorCode code) {
    switch (code) {
        case ErrorCode::success:
            return "Success";
        case ErrorCode::socketCreateFailed:
            return "Socket creation failed";
        case ErrorCode::socketBindFailed:
            return "Socket bind failed";
        case ErrorCode::socketListenFailed:
            return "Socket listen failed";
        case ErrorCode::socketAcceptFailed:
            return "Socket accept failed";
        case ErrorCode::socketConnectFailed:
            return "Socket connect failed";
        case ErrorCode::socketSendFailed:
            return "Socket send failed";
        case ErrorCode::socketReceiveFailed:
            return "Socket receive failed";
        case ErrorCode::socketSetOptionFailed:
            return "Socket set option failed";
        case ErrorCode::socketGetSocknameFailed:
            return "Socket getsockname failed";
        case ErrorCode::socketAddressParseFailed:
            return "Socket address parse failed";
        case ErrorCode::invalidParameter:
            return "Invalid parameter";
        case ErrorCode::memoryAllocationFailed:
            return "Memory allocation failed";
        case ErrorCode::websocketHandshakeFailed:
            return "WebSocket handshake failed";
        case ErrorCode::websocketFrameParseFailed:
            return "WebSocket frame parse failed";
        case ErrorCode::websocketInvalidOpcode:
            return "WebSocket invalid opcode";
        case ErrorCode::websocketPayloadTooLarge:
            return "WebSocket payload too large";
        case ErrorCode::websocketConnectionClosed:
            return "WebSocket connection closed";
        case ErrorCode::threadCreationFailed:
            return "Thread creation failed";
        case ErrorCode::unknownError:
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
