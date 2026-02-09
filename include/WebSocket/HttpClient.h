#pragma once

#include "SocketBase.h"
#include <string>
#include <map>
#include <vector>
#include <chrono>

namespace nob {

// Default timeout for HTTP requests
constexpr std::chrono::milliseconds DEFAULT_HTTP_TIMEOUT{30000};

// Port enumeration for type-safe port specification
enum class Port : int {
    HTTP_DEFAULT = 80,
    SSL_DEFAULT = 443,
    PROXY_DEFAULT = 8080,
    LOCALHOST_DEFAULT = 3000
};

/**
 * @brief HTTP Response structure
 */
struct HttpResponse {
    int statusCode = 0;
    std::string statusMessage;
    std::map<std::string, std::string> headers;
    std::vector<uint8_t> body;
    
    bool isSuccess() const {
        return statusCode >= 200 && statusCode < 300;
    }
};

/**
 * @brief HTTP Client class that uses SocketBase as its base
 * 
 * This class provides HTTP functionality while maintaining the compiler firewall.
 * All socket operations go through SocketBase, ensuring no native socket headers
 * are exposed to the user.
 */
class HttpClient : public SocketBase {
public:
    /**
     * @brief Constructor
     */
    HttpClient();

    /**
     * @brief Destructor
     */
    ~HttpClient() = default;

    /**
     * @brief Set request timeout in milliseconds
     * @param timeout Timeout duration
     */
    void setTimeout(std::chrono::milliseconds timeout);

    /**
     * @brief Set custom user agent
     * @param userAgent User agent string
     */
    void setUserAgent(const std::string& userAgent);

    /**
     * @brief Add custom header
     * @param name Header name
     * @param value Header value
     */
    void setHeader(const std::string& name, const std::string& value);

    /**
     * @brief Perform HTTP GET request
     * @param url Target URL (e.g., "https://www.google.com")
     * @param port Port to use for connection (default: HTTP_DEFAULT)
     * @param timeout Optional timeout (default: DEFAULT_HTTP_TIMEOUT)
     * @return HTTP response
     */
    HttpResponse get(const std::string& url, Port port = Port::HTTP_DEFAULT, std::chrono::milliseconds timeout = DEFAULT_HTTP_TIMEOUT);

    /**
     * @brief Perform HTTP POST request
     * @param url Target URL
     * @param data Request body data
     * @param contentType Content type (default: "application/x-www-form-urlencoded")
     * @return HTTP response
     */
    HttpResponse post(const std::string& url, const std::string& data, 
                     const std::string& contentType = "application/x-www-form-urlencoded");

    /**
     * @brief Download content to file
     * @param url Target URL
     * @param filePath Local file path to save content
     * @return Success status
     */
    bool downloadToFile(const std::string& url, const std::string& filePath);

protected:
    struct ParsedUrl {
        std::string scheme;
        std::string host;
        int port;
        std::string path;
        bool useHttps;
    };

    virtual ParsedUrl parseUrl(const std::string& url, Port defaultPort = Port::HTTP_DEFAULT);

private:
    std::chrono::milliseconds m_timeout = DEFAULT_HTTP_TIMEOUT;
    std::string m_userAgent = "HttpClient/1.0";
    std::map<std::string, std::string> m_headers;

    // HTTP protocol methods
    std::string buildRequest(const std::string& method, const std::string& path, 
                           const std::map<std::string, std::string>& headers, 
                           const std::string& body = "");
    
    HttpResponse parseResponse(const std::string& response);
    
    // Connection methods (using SocketBase firewall)
    bool connectToHost(const std::string& host, int port, std::chrono::milliseconds timeout = std::chrono::milliseconds(30000));
    std::string sendHttpRequest(const std::string& request);
    std::string receiveHttpResponse();
    
    // Utility methods
    std::string urlEncode(const std::string& str);
    std::string getCurrentTime();
};

} // namespace nob
