#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include "WebSocket/HttpClient.h"
#include "WebSocket/ErrorCodes.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <thread>
#include <chrono>
#include <cstring>
#include <fstream>

// Helper function for network byte order
uint16_t htons(uint16_t hostshort) {
    return (hostshort << 8) | ((hostshort >> 8) & 0xFF);
}

namespace nob {

HttpClient::HttpClient() {
    // Set default headers
    m_headers["User-Agent"] = m_userAgent;
    m_headers["Connection"] = "close";
    m_headers["Accept"] = "*/*";
}

void HttpClient::setTimeout(int timeoutSeconds) {
    m_timeoutSeconds = timeoutSeconds;
}

void HttpClient::setUserAgent(const std::string& userAgent) {
    m_userAgent = userAgent;
    m_headers["User-Agent"] = userAgent;
}

void HttpClient::setHeader(const std::string& name, const std::string& value) {
    m_headers[name] = value;
}

HttpClient::ParsedUrl HttpClient::parseUrl(const std::string& url) {
    ParsedUrl parsed;
    
    // Default values
    parsed.useHttps = false;
    parsed.port = 80;
    
    // Find scheme
    size_t schemeEnd = url.find("://");
    if (schemeEnd != std::string::npos) {
        parsed.scheme = url.substr(0, schemeEnd);
        parsed.useHttps = (parsed.scheme == "https");
        parsed.port = parsed.useHttps ? 443 : 80;
        
        size_t hostStart = schemeEnd + 3;
        size_t pathStart = url.find('/', hostStart);
        
        if (pathStart == std::string::npos) {
            parsed.host = url.substr(hostStart);
            parsed.path = "/";
        } else {
            parsed.host = url.substr(hostStart, pathStart - hostStart);
            parsed.path = url.substr(pathStart);
        }
    } else {
        // No scheme, assume http
        parsed.scheme = "http";
        parsed.host = url;
        parsed.path = "/";
    }
    
    // Handle port in host (e.g., "example.com:8080")
    size_t portPos = parsed.host.find(':');
    if (portPos != std::string::npos) {
        parsed.port = std::stoi(parsed.host.substr(portPos + 1));
        parsed.host = parsed.host.substr(0, portPos);
    }
    
    return parsed;
}

std::string HttpClient::buildRequest(const std::string& method, const std::string& path,
                                   const std::map<std::string, std::string>& headers,
                                   const std::string& body) {
    std::stringstream request;
    
    // Request line
    request << method << " " << path << " HTTP/1.1\r\n";
    
    // Host header (will be set separately)
    
    // Other headers
    for (const auto& header : headers) {
        request << header.first << ": " << header.second << "\r\n";
    }
    
    // Content-Length for POST requests
    if (!body.empty()) {
        request << "Content-Length: " << body.length() << "\r\n";
    }
    
    // End of headers
    request << "\r\n";
    
    // Body (for POST requests)
    if (!body.empty()) {
        request << body;
    }
    
    return request.str();
}

std::string HttpClient::getCurrentTime() {
    // Simple current time implementation - avoid complex time functions
    return "Mon, 01 Jan 2024 00:00:00 GMT";
}

bool HttpClient::connectToHost(const std::string& host, int port, std::chrono::milliseconds timeout) {
    // Create socket using SocketBase abstract
    Result result = createNativeSocket(AF_INET_VALUE, SOCK_STREAM_VALUE, 0);
    if (result.isError()) {
        std::cerr << result.errorMessage() << std::endl;
        return false;
    }
    
    // Use SocketBase's DNS resolution method (abstract compliant)
    SocketBase::SocketAddress addr;
    result = resolveHostname(host, addr);
    if (result.isError()) {
        std::cerr << result.errorMessage() << std::endl;
        closeNativeSocket();
        return false;
    }
    
    // Set port using our helper function
    addr.setPort(htons(port));
    
    // Non-blocking connection loop with timeout
    result = connectNativeSocket(addr);
    if (result.isError()) {
        // On non-blocking sockets, connect typically returns EINPROGRESS/WSAEWOULDBLOCK
        // This is expected, so we wait for the connection to complete
        auto startTime = std::chrono::steady_clock::now();
        
        while (true) {
            // Check for timeout
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - startTime).count();
            
            if (elapsed > timeout.count()) {
                std::cerr << "Connection timeout after " << timeout.count() << "ms for " << host << std::endl;
                return false;
            }
            
            // Check if socket is ready for writing (connection complete)
            bool canWrite = false;
            bool canRead = false;
            Result selectResult = selectNativeSocket(100, &canRead, &canWrite); // 100ms timeout
            
            if (selectResult.isError()) {
                std::cerr << selectResult.errorMessage() << std::endl;
                return false;
            }
            
            if (canWrite) {
                // Connection is ready, check if it succeeded
                int error = 0;
                size_t errorSize = sizeof(error);
                Result getResult = getSocketOptionNative(SOL_SOCKET_VALUE, SO_ERROR_VALUE, &error, &errorSize);
                
                if (getResult.isError()) {
                    std::cerr << getResult.errorMessage() << std::endl;
                    return false;
                }
                
                if (error == 0) {
                    // Connection successful
                    return true;
                } else {
                    std::cerr << "Connection failed: " << error << std::endl;
                    return false;
                }
            }
            
            // Small delay to prevent busy waiting
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }
    
    return true;
}

std::string HttpClient::sendHttpRequest(const std::string& request) {
    // Send request
    size_t bytesSent = 0;
    Result result = sendNativeSocket(request.c_str(), request.length(), &bytesSent);
    if (result.isError() || bytesSent != request.length()) {
        return "";
    }
    
    // Receive response
    return receiveHttpResponse();
}

std::string HttpClient::receiveHttpResponse() {
    std::string response;
    std::vector<char> buffer(4096);
    
    while (true) {
        size_t bytesReceived = 0;
        Result result = receiveNativeSocket(buffer.data(), buffer.size(), &bytesReceived);
        
        if (result.isError() || bytesReceived == 0) {
            break;
        }
        
        response.append(buffer.data(), bytesReceived);
        
        // Simple check for end of response (double CRLF)
        if (response.find("\r\n\r\n") != std::string::npos) {
            // Check if we have Content-Length header
            size_t headerEnd = response.find("\r\n\r\n");
            std::string headers = response.substr(0, headerEnd);
            
            size_t contentLengthPos = headers.find("Content-Length: ");
            if (contentLengthPos != std::string::npos) {
                size_t lineEnd = headers.find("\r\n", contentLengthPos);
                std::string lengthStr = headers.substr(contentLengthPos + 16, lineEnd - contentLengthPos - 16);
                size_t contentLength = std::stoul(lengthStr);
                
                size_t bodyStart = headerEnd + 4;
                size_t currentBodySize = response.length() - bodyStart;
                
                if (currentBodySize >= contentLength) {
                    break;
                }
            }
        }
    }
    
    return response;
}

HttpResponse HttpClient::parseResponse(const std::string& response) {
    HttpResponse httpResponse;
    
    // Find end of headers
    size_t headerEnd = response.find("\r\n\r\n");
    if (headerEnd == std::string::npos) {
        return httpResponse; // Invalid response
    }
    
    std::string headers = response.substr(0, headerEnd);
    std::string body = response.substr(headerEnd + 4);
    
    // Parse status line
    size_t statusEnd = headers.find("\r\n");
    std::string statusLine = headers.substr(0, statusEnd);
    
    std::stringstream ss(statusLine);
    std::string httpVersion;
    ss >> httpVersion >> httpResponse.statusCode >> httpResponse.statusMessage;
    
    // Parse headers
    std::stringstream headerStream(headers.substr(statusEnd + 2));
    std::string headerLine;
    
    while (std::getline(headerStream, headerLine)) {
        // Remove \r
        if (!headerLine.empty() && headerLine.back() == '\r') {
            headerLine.pop_back();
        }
        
        if (headerLine.empty()) break;
        
        size_t colonPos = headerLine.find(':');
        if (colonPos != std::string::npos) {
            std::string name = headerLine.substr(0, colonPos);
            std::string value = headerLine.substr(colonPos + 1);
            
            // Trim whitespace
            name.erase(0, name.find_first_not_of(" \t"));
            name.erase(name.find_last_not_of(" \t") + 1);
            value.erase(0, value.find_first_not_of(" \t"));
            value.erase(value.find_last_not_of(" \t") + 1);
            
            httpResponse.headers[name] = value;
        }
    }
    
    // Store body
    httpResponse.body.assign(body.begin(), body.end());
    
    return httpResponse;
}

std::string HttpClient::urlEncode(const std::string& str) {
    std::string encoded;
    for (char c : str) {
        if (std::isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            encoded += c;
        } else {
            encoded += '%';
            char hex[3];
            snprintf(hex, sizeof(hex), "%02X", static_cast<unsigned char>(c));
            encoded += hex;
        }
    }
    return encoded;
}

HttpResponse HttpClient::get(const std::string& url, std::chrono::milliseconds timeout) {
    ParsedUrl parsed = parseUrl(url);
    
    // Update headers
    m_headers["Host"] = parsed.host;
    m_headers["Date"] = getCurrentTime();
    
    // Build request
    std::string request = buildRequest("GET", parsed.path, m_headers);
    
    // Connect and send with timeout
    if (!connectToHost(parsed.host, parsed.port, timeout)) {
        return HttpResponse(); // Connection failed
    }
    
    std::string response = sendHttpRequest(request);
    
    // Close connection
    closeNativeSocket();
    
    return parseResponse(response);
}

HttpResponse HttpClient::post(const std::string& url, const std::string& data, 
                           const std::string& contentType) {
    ParsedUrl parsed = parseUrl(url);
    
    // Update headers
    m_headers["Host"] = parsed.host;
    m_headers["Date"] = getCurrentTime();
    m_headers["Content-Type"] = contentType;
    
    // Build request
    std::string request = buildRequest("POST", parsed.path, m_headers, data);
    
    // Connect and send
    if (!connectToHost(parsed.host, parsed.port)) {
        return HttpResponse(); // Connection failed
    }
    
    std::string response = sendHttpRequest(request);
    
    // Close connection
    closeNativeSocket();
    
    return parseResponse(response);
}

bool HttpClient::downloadToFile(const std::string& url, const std::string& filePath) {
    HttpResponse response = get(url);
    
    if (!response.isSuccess()) {
        return false;
    }
    
    std::ofstream file(filePath, std::ios::binary);
    if (!file.is_open()) {
        return false;
    }
    
    file.write(reinterpret_cast<const char*>(response.body.data()), response.body.size());
    return file.good();
}

} // namespace nob
