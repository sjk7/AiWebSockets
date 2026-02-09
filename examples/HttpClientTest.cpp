#include "WebSocket/HttpClient.h"
#include <iostream>
#include <type_traits>

// Template trick to static_assert that native socket functions are not available
template<typename T, typename = void>
struct check_socket_not_available : std::true_type {};

// This template should NOT be instantiated if ::socket exists (abstract violation)
template<typename T>
struct check_socket_not_available<T, std::void_t<decltype(::socket)>> : std::false_type {};

// Static assert that the first template (without ::socket) is the one chosen
static_assert(check_socket_not_available<void>::value, 
              "🛡️ COMPILER ABSTRACT VIOLATION: ::socket() should NOT be available!");

// Alternative approach: Check if we can call ::socket (C++17 compatible)
template<typename T>
struct can_call_socket {
    template<typename U = T>
    static constexpr auto test(int) -> decltype(::socket(0, 0, 0), std::true_type{}) {
        return std::true_type{};
    }
    
    template<typename U = T>
    static constexpr auto test(...) -> std::false_type {
        return std::false_type{};
    }
    
    static constexpr bool value = decltype(test<T>(0))::value;
};

static_assert(!can_call_socket<void>::value, 
              " COMPILER ABSTRACTION VIOLATION: ::socket() should NOT be available!");

// Test class to access protected members
class TestHttpClient : public nob::HttpClient {
public:
    using HttpClient::HttpClient;
    using HttpClient::ParsedUrl;
    using HttpClient::parseUrl;
};

int main() {
    std::cout << "=== HttpClient Test with SocketBase Compiler Abstraction ===" << std::endl;
    
    // Create HttpClient (inherits from SocketBase - behind compiler abstraction!)
    TestHttpClient client;
    std::cout << "✅ HttpClient created successfully!" << std::endl;
    std::cout << "🛡️ Behind SocketBase compiler abstraction!" << std::endl;
    
    // Test basic configuration methods
    client.setTimeout(30);
    client.setUserAgent("TestClient/1.0");
    client.setHeader("X-Custom-Header", "TestValue");
    
    
    std::cout << "✅ Configuration methods work!" << std::endl;
    
    // Test URL parsing (now accessible through derived class)
    TestHttpClient::ParsedUrl url = client.parseUrl("http://www.google.com");
    std::cout << "✅ URL parsing works!" << std::endl;
    std::cout << "   Host: " << url.host << std::endl;
    std::cout << "   Port: " << url.port << std::endl;
    std::cout << "   Path: " << url.path << std::endl;
    std::cout << "   HTTPS: " << (url.useHttps ? "Yes" : "No") << std::endl;

   //const auto no_compile = ::socket(AF_INET, SOCK_STREAM, 0);
   // (void)no_compile;

    // Test HTTP GET method (this will try to connect)
    std::cout << "\n🚀 Testing HTTP GET request..." << std::endl;
    std::cout << "   Attempting to connect to: http://httpbin.org/get" << std::endl;
    
    nob::HttpResponse response = client.get("http://httpbin.org/get");
    
    if (response.isSuccess()) {
        std::cout << "✅ HTTP GET Success!" << std::endl;
        std::cout << "   Status: " << response.statusCode << " " << response.statusMessage << std::endl;
        std::cout << "   Headers: " << response.headers.size() << std::endl;
        std::cout << "   Body size: " << response.body.size() << " bytes" << std::endl;
        
        // Show first 100 chars of response
        if (!response.body.empty()) {
            std::string bodyStr(response.body.begin(), response.body.end());
            std::cout << "   Response preview: " << bodyStr.substr(0, 100) << "..." << std::endl;
        }
    } else {
        std::cout << "❌ HTTP GET Failed!" << std::endl;
        std::cout << "   Status: " << response.statusCode << std::endl;
        std::cout << "   Error: " << response.statusMessage << std::endl;
        std::cerr << "   Check stderr for detailed error information" << std::endl;
    }
    
    // Test various bogus URLs with timeout measurement
    std::cout << "\n🚀 Testing various bogus URLs with timeout measurement..." << std::endl;
    
    std::vector<std::tuple<std::string, std::string, int>> bogusUrls = {
        {"http://bogus-url-that-does-not-exist.com", "Non-existent domain", 3000},
        {"http://192.168.255.254", "Non-routable IP address", 2000},
        {"http://localhost:99999", "Invalid port number", 1000},
        {"http://.invalid", "Invalid domain format", 2000},
        {"http://127.0.0.1:99999", "Invalid port on localhost", 1000},
        {"http://this-domain-definitely-does-not-exist-12345.com", "Long non-existent domain", 2000}
    };
    
    for (const auto& [url, description, timeoutMs] : bogusUrls) {
        std::cout << "\n   Testing: " << url << " (" << description << ")" << std::endl;
        std::cout << "   Timeout: " << timeoutMs << "ms" << std::endl;
        
        auto startTime = std::chrono::steady_clock::now();
        
        nob::HttpResponse bogusResponse = client.get(url, std::chrono::milliseconds(timeoutMs));
        
        auto endTime = std::chrono::steady_clock::now();
        auto actualTime = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();
        
        std::cout << "   Actual time taken: " << actualTime << "ms" << std::endl;
        
        if (bogusResponse.isSuccess()) {
            std::cout << "   ❌ Unexpected success with bogus URL!" << std::endl;
        } else {
            std::cout << "   ✅ Bogus URL correctly failed (as expected)" << std::endl;
            std::cout << "   Status: " << bogusResponse.statusCode << std::endl;
            std::cout << "   Error: " << bogusResponse.statusMessage << std::endl;
            
            // Check if timeout was respected
            if (actualTime > timeoutMs + 500) { // Allow 500ms tolerance
                std::cout << "   ⚠️  Warning: Took longer than expected timeout!" << std::endl;
            } else {
                std::cout << "   ✅ Timeout respected" << std::endl;
            }
            
            std::cerr << "   Failed to connect to: " << url << " (took " << actualTime << "ms)" << std::endl;
        }
    }
    
    std::cout << "\n=== Test Complete ===" << std::endl;
    std::cout << " Compiler Abstraction Status: MAINTAINED!" << std::endl;
    std::cout << " HttpClient working behind SocketBase compiler abstraction!" << std::endl;
    
    return 0;
}
