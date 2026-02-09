#pragma once

#include "Types.h"
#include <string>

namespace WebSocket {

class Result {
private:
    ErrorCode m_errorCode;
    int m_systemErrorCode;  // Cached system error number (WSAGetLastError/errno)
    mutable std::string m_cachedErrorMessage;  // Lazy-evaluated error message
    mutable bool m_messageCached;  // Track if message has been cached
    
public:
    // Constructor for success case
    Result() : m_errorCode(ErrorCode::success), m_systemErrorCode(0), m_messageCached(false) {}
    
    // Constructor for error with system error code
    Result(ErrorCode code, int systemErrorCode = 0) 
        : m_errorCode(code), m_systemErrorCode(systemErrorCode), m_messageCached(false) {}
    
    // Constructor for error with custom message (backward compatibility)
    Result(ErrorCode code, const std::string& customMessage) 
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
    ErrorCode getErrorCode() const { return m_errorCode; }
    int getSystemErrorCode() const { return m_systemErrorCode; }
    
    // Lazy error message generation - only converts when accessed
    const std::string& getErrorMessage() const {
        if (!m_messageCached) {
            m_cachedErrorMessage = generateErrorMessage();
            m_messageCached = true;
        }
        return m_cachedErrorMessage;
    }
    
    // Backward compatibility
    bool isSuccess() const { return m_errorCode == ErrorCode::success; }
    bool isError() const { return m_errorCode != ErrorCode::success; }
    
    // Backward compatibility property
    const std::string& errorMessage() const { return getErrorMessage(); }

private:
    std::string generateErrorMessage() const;
};

// Helper function to get string representation of error codes
const char* getErrorCodeString(ErrorCode code);

// Platform-specific error message retrieval (optimized - returns error number)
int getLastSystemErrorCode();

// Platform-specific error message retrieval (full string - use sparingly)
std::string getSystemErrorMessage(int errorCode);

} // namespace WebSocket
