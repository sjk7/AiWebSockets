#pragma once

#include "SocketBase.h"
#include <string>
#include <map>
#include <vector>

namespace nob {

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
     * @brief Set request timeout in seconds
     * @param timeoutSeconds Timeout duration
     */
    void setTimeout(int timeoutSeconds);

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
     * @return HTTP response
     */
    HttpResponse get(const std::string& url);

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

private:
    struct ParsedUrl {
        std::string scheme;
        std::string host;
        int port;
        std::string path;
        bool useHttps;
    };

    int m_timeoutSeconds = 30;
    std::string m_userAgent = "HttpClient/1.0";
    std::map<std::string, std::string> m_headers;

    // URL parsing
    ParsedUrl parseUrl(const std::string& url);
    
    // HTTP protocol methods
    std::string buildRequest(const std::string& method, const std::string& path, 
                           const std::map<std::string, std::string>& headers, 
                           const std::string& body = "");
    
    HttpResponse parseResponse(const std::string& response);
    
    // Connection methods (using SocketBase firewall)
    bool connectToHost(const std::string& host, int port);
    std::string sendHttpRequest(const std::string& request);
    std::string receiveHttpResponse();
    
    // Utility methods
    std::string urlEncode(const std::string& str);
    std::string getCurrentTime();
};

} // namespace nob
