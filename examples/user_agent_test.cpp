#include <iostream>
#include <vector>
#include <string>
#include <thread>
#include <chrono>
#include "WebSocket/Socket.h"

using namespace WebSocket;

void TestUserAgentFiltering() {
    std::cout << "🧪 Testing User-Agent Filtering" << std::endl;
    std::cout << "===============================" << std::endl;
    
    std::vector<std::pair<std::string, bool>> testCases = {
        {"Mozilla/5.0", true},      // Should be allowed
        {"curl/7.68.0", true},      // Should be allowed
        {"sqlmap/1.0", false},      // Should be blocked
        {"nikto/2.1.6", false},     // Should be blocked
        {"Nmap Scripting Engine", false}, // Should be blocked
        {"masscan/1.0.3", false},   // Should be blocked
        {"MyBot sqlmap Scanner", false}, // Should be blocked (contains sqlmap)
        {"", true}                  // No User-Agent should be allowed
    };
    
    for (const auto& testCase : testCases) {
        std::cout << "\n🔍 Testing: '" << testCase.first << "' (should " << (testCase.second ? "allow" : "block") << ")" << std::endl;
        
        Socket client;
        client.Create(SOCKET_FAMILY::IPV4, SOCKET_TYPE::TCP);
        
        if (client.Connect("127.0.0.1", 8080).IsSuccess()) {
            std::string request = "GET / HTTP/1.1\r\nHost: 127.0.0.1:8080\r\n";
            
            // Only add User-Agent header if it's not empty
            if (!testCase.first.empty()) {
                request += "User-Agent: " + testCase.first + "\r\n";
            }
            
            request += "\r\n";
            
            auto sendResult = client.Send(std::vector<uint8_t>(request.begin(), request.end()));
            
            if (sendResult.IsSuccess()) {
                auto receiveResult = client.Receive(1024);
                
                if (receiveResult.first.IsSuccess() && !receiveResult.second.empty()) {
                    std::string response(receiveResult.second.begin(), receiveResult.second.end());
                    
                    size_t statusStart = response.find(" ");
                    size_t statusEnd = response.find(" ", statusStart + 1);
                    
                    if (statusStart != std::string::npos && statusEnd != std::string::npos) {
                        std::string statusCode = response.substr(statusStart + 1, statusEnd - statusStart - 1);
                        
                        bool allowed = (statusCode == "200");
                        bool blocked = (statusCode == "400");
                        
                        if (testCase.second && allowed) {
                            std::cout << "   ✅ Correctly allowed (200 OK)" << std::endl;
                        } else if (!testCase.second && blocked) {
                            std::cout << "   🚫 Correctly blocked (400 Bad Request)" << std::endl;
                        } else if (testCase.second && blocked) {
                            std::cout << "   ❌ Incorrectly blocked (got 400, expected 200)" << std::endl;
                        } else if (!testCase.second && allowed) {
                            std::cout << "   ❌ Incorrectly allowed (got 200, expected 400)" << std::endl;
                        } else {
                            std::cout << "   ❓ Unexpected response: " << statusCode << std::endl;
                        }
                    } else {
                        std::cout << "   ❓ Could not parse status code" << std::endl;
                    }
                } else {
                    std::cout << "   🚫 Connection closed (likely blocked)" << std::endl;
                }
            } else {
                std::cout << "   ❌ Send failed" << std::endl;
            }
            
            client.Close();
        } else {
            std::cout << "   ❌ Failed to connect - server is down" << std::endl;
            return;
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

void TestManualUserAgent() {
    std::cout << "\n\n🧪 Manual User-Agent Test" << std::endl;
    std::cout << "=========================" << std::endl;
    
    Socket client;
    client.Create(SOCKET_FAMILY::IPV4, SOCKET_TYPE::TCP);
    
    if (client.Connect("127.0.0.1", 8080).IsSuccess()) {
        // Test with exactly what should trigger the block
        std::string request = 
            "GET / HTTP/1.1\r\n"
            "Host: 127.0.0.1:8080\r\n"
            "User-Agent: sqlmap/1.0\r\n"
            "\r\n";
        
        std::cout << "📤 Sending exact sqlmap User-Agent:" << std::endl;
        std::cout << "   " << request << std::endl;
        
        auto sendResult = client.Send(std::vector<uint8_t>(request.begin(), request.end()));
        
        if (sendResult.IsSuccess()) {
            auto receiveResult = client.Receive(1024);
            
            if (receiveResult.first.IsSuccess() && !receiveResult.second.empty()) {
                std::string response(receiveResult.second.begin(), receiveResult.second.end());
                std::cout << "📄 Received response: " << response << std::endl;
            } else {
                std::cout << "🚫 No response received (connection likely closed)" << std::endl;
            }
        }
        
        client.Close();
    }
}

int main() {
    std::cout << "🔍 User-Agent Checking Analysis Tool" << std::endl;
    std::cout << "===================================" << std::endl;
    std::cout << "💡 Make sure the server is running: ./build-release/aiWebSocketsServer.exe" << std::endl;
    std::cout << std::endl;
    
    TestUserAgentFiltering();
    
    std::cout << "\n\n🎯 User-Agent Filtering Test Complete" << std::endl;
    std::cout << "=====================================" << std::endl;
    
    return 0;
}
