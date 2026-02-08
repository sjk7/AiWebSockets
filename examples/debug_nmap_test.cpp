#include <iostream>
#include <string>
#include "WebSocket/Socket.h"

using namespace WebSocket;

int main() {
    std::cout << "🔍 Debug Nmap Scripting Engine Test" << std::endl;
    std::cout << "===================================" << std::endl;
    
    Socket client;
    if (!client.Create(SOCKET_FAMILY::IPV4, SOCKET_TYPE::TCP).IsSuccess()) {
        std::cout << "❌ Failed to create client socket" << std::endl;
        return 1;
    }
    
    if (!client.Connect("127.0.0.1", 8080).IsSuccess()) {
        std::cout << "❌ Failed to connect to server" << std::endl;
        return 1;
    }
    
    // Test the exact User-Agent string that's failing
    std::string request = "GET / HTTP/1.1\r\nHost: 127.0.0.1:8080\r\nUser-Agent: Nmap Scripting Engine\r\n\r\n";
    std::cout << "🔍 Sending request with User-Agent: 'Nmap Scripting Engine'" << std::endl;
    std::cout << "🔍 Request length: " << request.length() << " bytes" << std::endl;
    std::cout << "🔍 Request content: [" << request << "]" << std::endl;
    
    auto sendResult = client.Send(std::vector<uint8_t>(request.begin(), request.end()));
    if (!sendResult.IsSuccess()) {
        std::cout << "❌ Failed to send request" << std::endl;
        return 1;
    }
    
    auto receiveResult = client.Receive(1024);
    if (receiveResult.first.IsSuccess() && !receiveResult.second.empty()) {
        std::string response(receiveResult.second.begin(), receiveResult.second.end());
        std::cout << "🔍 Server response: [" << response << "]" << std::endl;
        
        size_t statusStart = response.find(" ");
        size_t statusEnd = response.find(" ", statusStart + 1);
        
        if (statusStart != std::string::npos && statusEnd != std::string::npos) {
            std::string statusCode = response.substr(statusStart + 1, statusEnd - statusStart - 1);
            std::cout << "🔍 Status code: " << statusCode << std::endl;
            
            if (statusCode == "200") {
                std::cout << "❌ Nmap Scripting Engine was incorrectly allowed" << std::endl;
            } else if (statusCode == "400") {
                std::cout << "✅ Nmap Scripting Engine was correctly blocked" << std::endl;
            } else {
                std::cout << "❓ Unexpected status code: " << statusCode << std::endl;
            }
        }
    } else {
        std::cout << "❌ Failed to receive response or connection closed" << std::endl;
    }
    
    client.Close();
    return 0;
}
