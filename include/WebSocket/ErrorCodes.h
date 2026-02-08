#pragma once

#include <string>

namespace WebSocket {

enum class ERROR_CODE {
    SUCCESS = 0,
    SOCKET_CREATE_FAILED,
    SOCKET_BIND_FAILED,
    SOCKET_LISTEN_FAILED,
    SOCKET_ACCEPT_FAILED,
    SOCKET_CONNECT_FAILED,
    SOCKET_SEND_FAILED,
    SOCKET_RECEIVE_FAILED,
    SOCKET_SET_OPTION_FAILED,
    SOCKET_GETSOCKNAME_FAILED,
    SOCKET_ADDRESS_PARSE_FAILED,
    INVALID_PARAMETER,
    MEMORY_ALLOCATION_FAILED,
    WEBSOCKET_HANDSHAKE_FAILED,
    WEBSOCKET_FRAME_PARSE_FAILED,
    WEBSOCKET_INVALID_OPCODE,
    WEBSOCKET_PAYLOAD_TOO_LARGE,
    WEBSOCKET_CONNECTION_CLOSED,
    THREAD_CREATION_FAILED,
    UNKNOWN_ERROR
};

class Result {
private:
    ERROR_CODE m_errorCode;
    int m_systemErrorCode;  // Cached system error number (WSAGetLastError/errno)
    mutable std::string m_cachedErrorMessage;  // Lazy-evaluated error message
    mutable bool m_messageCached;  // Track if message has been cached
    
public:
    // Constructor for success case
    Result() : m_errorCode(ERROR_CODE::SUCCESS), m_systemErrorCode(0), m_messageCached(false) {}
    
    // Constructor for error with system error code
    Result(ERROR_CODE code, int systemErrorCode = 0) 
        : m_errorCode(code), m_systemErrorCode(systemErrorCode), m_messageCached(false) {}
    
    // Constructor for error with custom message (backward compatibility)
    Result(ERROR_CODE code, const std::string& customMessage) 
        : m_errorCode(code), m_systemErrorCode(0), m_cachedErrorMessage(customMessage), m_messageCached(true) {}
    
    // Copy constructor
    Result(const Result& other) 
        : m_errorCode(other.m_errorCode), m_systemErrorCode(other.m_systemErrorCode), 
          m_cachedErrorMessage(other.m_cachedErrorMessage), m_messageCached(other.m_messageCached) {}
    
    // Assignment operator
    Result& operator=(const Result& other) {
        if (this != &other) {
            m_errorCode = other.m_errorCode;
            m_systemErrorCode = other.m_systemErrorCode;
            m_cachedErrorMessage = other.m_cachedErrorMessage;
            m_messageCached = other.m_messageCached;
        }
        return *this;
    }
    
    // Accessors
    ERROR_CODE GetErrorCode() const { return m_errorCode; }
    int GetSystemErrorCode() const { return m_systemErrorCode; }
    
    // Lazy error message generation - only converts when accessed
    const std::string& GetErrorMessage() const {
        if (!m_messageCached) {
            m_cachedErrorMessage = GenerateErrorMessage();
            m_messageCached = true;
        }
        return m_cachedErrorMessage;
    }
    
    // Backward compatibility
    bool IsSuccess() const { return m_errorCode == ERROR_CODE::SUCCESS; }
    bool IsError() const { return m_errorCode != ERROR_CODE::SUCCESS; }
    
    // Backward compatibility property
    const std::string& ErrorMessage() const { return GetErrorMessage(); }

private:
    std::string GenerateErrorMessage() const;
};

// Helper function to get string representation of error codes
const char* GetErrorCodeString(ERROR_CODE code);

// Platform-specific error message retrieval (optimized - returns error number)
int GetLastSystemErrorCode();

// Platform-specific error message retrieval (full string - use sparingly)
std::string GetSystemErrorMessage(int errorCode);

} // namespace WebSocket
