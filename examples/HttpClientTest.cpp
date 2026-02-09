#include "WebSocket/HttpClient.h"
#include <iostream>

int main() {
    nob::HttpClient client;
    
    std::cout << "Testing HttpClient with SocketBase firewall..." << std::endl;
    
    // Test GET request to Google
    std::cout << "\nDownloading Google homepage..." << std::endl;
    nob::HttpResponse response = client.get("http://www.google.com");
    
    if (response.isSuccess()) {
        std::cout << "✅ Success! Status: " << response.statusCode << " " << response.statusMessage << std::endl;
        std::cout << "Headers received: " << response.headers.size() << std::endl;
        std::cout << "Body size: " << response.body.size() << " bytes" << std::endl;
        
        // Save to file
        if (client.downloadToFile("http://www.google.com", "google_homepage.html")) {
            std::cout << "✅ Saved to google_homepage.html" << std::endl;
        } else {
            std::cout << "❌ Failed to save to file" << std::endl;
        }
    } else {
        std::cout << "❌ Failed! Status: " << response.statusCode << std::endl;
    }
    
    // Test POST request
    std::cout << "\nTesting POST request..." << std::endl;
    nob::HttpResponse postResponse = client.post("http://httpbin.org/post", "Hello from HttpClient!");
    
    if (postResponse.isSuccess()) {
        std::cout << "✅ POST Success! Status: " << postResponse.statusCode << std::endl;
        std::string body(postResponse.body.begin(), postResponse.body.end());
        std::cout << "Response body: " << body.substr(0, 200) << "..." << std::endl;
    } else {
        std::cout << "❌ POST Failed! Status: " << postResponse.statusCode << std::endl;
    }
    
    std::cout << "\nHttpClient test completed!" << std::endl;
    std::cout << "🛡️ Firewall maintained - no native socket headers exposed!" << std::endl;
    
    return 0;
}
