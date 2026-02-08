#include <iostream>
#include "WebSocket/Socket.h"

using namespace WebSocket;

int main(int argc, char* argv[]) {
    std::string path = "/";
    if (argc > 1) {
        path = argv[1];
    }
    
    std::cout << "HTTP Client for Hybrid Server" << std::endl;
    std::cout << "===============================" << std::endl;
    
    // Create client socket
    Socket clientSocket;
    if (!clientSocket.Create(SOCKET_FAMILY::IPV4, SOCKET_TYPE::TCP).IsSuccess()) {
        std::cout << "❌ Failed to create client socket" << std::endl;
        return 1;
    }
    
    // Connect to server
    std::cout << "🔗 Connecting to 127.0.0.1:8080..." << std::endl;
    if (!clientSocket.Connect("127.0.0.1", 8080).IsSuccess()) {
        std::cout << "❌ Failed to connect to server" << std::endl;
        return 1;
    }
    
    std::cout << "✅ Connected to server!" << std::endl;
    
    // Send HTTP GET request
    std::string httpRequest = "GET " + path + " HTTP/1.1\r\n";
    httpRequest += "Host: 127.0.0.1:8080\r\n";
    httpRequest += "User-Agent: HybridServerTest/1.0\r\n";
    httpRequest += "Connection: close\r\n";
    httpRequest += "\r\n";
    
    std::cout << "📤 Sending HTTP GET request for: " << path << std::endl;
    auto sendResult = clientSocket.Send(std::vector<uint8_t>(httpRequest.begin(), httpRequest.end()));
    
    if (!sendResult.IsSuccess()) {
        std::cout << "❌ Failed to send HTTP request: " << sendResult.GetErrorMessage() << std::endl;
        clientSocket.Close();
        return 1;
    }
    
    std::cout << "✅ HTTP request sent!" << std::endl;
    
    // Receive HTTP response
    std::cout << "📨 Receiving HTTP response..." << std::endl;
    ReceiveResult receiveResult = clientSocket.Receive(8192);
    
    if (receiveResult.first.IsSuccess() && !receiveResult.second.empty()) {
        std::string response(receiveResult.second.begin(), receiveResult.second.end());
        std::cout << "📄 HTTP Response received:" << std::endl;
        std::cout << "================================" << std::endl;
        std::cout << response << std::endl;
        std::cout << "================================" << std::endl;
    } else {
        std::cout << "❌ Failed to receive HTTP response" << std::endl;
    }
    
    // Close connection
    clientSocket.Close();
    std::cout << "🔌 Disconnected from server" << std::endl;
    
    return 0;
}
