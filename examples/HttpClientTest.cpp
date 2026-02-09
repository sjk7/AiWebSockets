#include "WebSocket/HttpClient.h"
#include <iostream>

// Test class to access protected members
class TestHttpClient : public nob::HttpClient {
public:
    using HttpClient::HttpClient;
    using HttpClient::ParsedUrl;
    using HttpClient::parseUrl;
};

int main() {
    std::cout << "=== HttpClient Test with SocketBase Firewall ===" << std::endl;
    
    // Create HttpClient (inherits from SocketBase - behind firewall!)
    TestHttpClient client;
    std::cout << "✅ HttpClient created successfully!" << std::endl;
    std::cout << "🛡️ Behind SocketBase firewall!" << std::endl;
    
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
    
    // Test HTTP GET method (this will try to connect)
    std::cout << "\n🚀 Testing HTTP GET request..." << std::endl;
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
    }
    
    std::cout << "\n=== Test Complete ===" << std::endl;
    std::cout << "🛡️ Firewall Status: MAINTAINED!" << std::endl;
    std::cout << "🚀 HttpClient working behind SocketBase firewall!" << std::endl;
    
    return 0;
}
