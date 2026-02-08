#include <iostream>
#include <vector>
#include <string>
#include <thread>
#include <chrono>
#include <memory>
#include "WebSocket/Socket.h"

using namespace WebSocket;

void TestBasicConnection() {
    std::cout << "🧪 Testing Basic Connection" << std::endl;
    std::cout << "============================" << std::endl;
    
    Socket client;
    auto result = client.Create(SOCKET_FAMILY::IPV4, SOCKET_TYPE::TCP);
    if (!result.IsSuccess()) {
        std::cout << "❌ Failed to create client socket: " << result.GetErrorMessage() << std::endl;
        return;
    }
    
    result = client.Connect("127.0.0.1", 8080);
    if (!result.IsSuccess()) {
        std::cout << "❌ Failed to connect to server: " << result.GetErrorMessage() << std::endl;
        return;
    }
    
    std::cout << "✅ Connected successfully" << std::endl;
    
    // Send a simple HTTP request
    std::string request = "GET / HTTP/1.1\r\nHost: 127.0.0.1:8080\r\n\r\n";
    auto sendResult = client.Send(std::vector<uint8_t>(request.begin(), request.end()));
    
    if (sendResult.IsSuccess()) {
        std::cout << "✅ Request sent successfully" << std::endl;
        
        // Try to receive response
        auto receiveResult = client.Receive(4096);
        
        if (receiveResult.first.IsSuccess()) {
            if (!receiveResult.second.empty()) {
                std::string response(receiveResult.second.begin(), receiveResult.second.end());
                std::cout << "✅ Response received: " << response.substr(0, std::min(response.length(), size_t(50))) << "..." << std::endl;
            } else {
                std::cout << "❌ No response received" << std::endl;
            }
        } else {
            std::cout << "❌ Receive failed: " << receiveResult.first.GetErrorMessage() << std::endl;
        }
    } else {
        std::cout << "❌ Send failed: " << sendResult.GetErrorMessage() << std::endl;
    }
    
    client.Close();
    std::cout << "✅ Connection closed" << std::endl;
    std::cout << std::endl;
}

void TestMultipleConnections() {
    std::cout << "🧪 Testing Multiple Connections" << std::endl;
    std::cout << "===============================" << std::endl;
    
    std::vector<std::unique_ptr<Socket>> clients;
    int successfulConnections = 0;
    
    for (int i = 0; i < 10; i++) {
        auto client = std::make_unique<Socket>();
        if (client->Create(SOCKET_FAMILY::IPV4, SOCKET_TYPE::TCP).IsSuccess()) {
            if (client->Connect("127.0.0.1", 8080).IsSuccess()) {
                clients.push_back(std::move(client));
                successfulConnections++;
                std::cout << "✅ Connection " << i << " successful" << std::endl;
            } else {
                std::cout << "❌ Connection " << i << " failed" << std::endl;
            }
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    std::cout << "📊 Results: " << successfulConnections << "/10 connections successful" << std::endl;
    
    // Clean up
    for (auto& client : clients) {
        client->Close();
    }
    
    std::cout << "✅ All connections closed" << std::endl;
    std::cout << std::endl;
}

int main() {
    std::cout << "🔧 Security Debug Test" << std::endl;
    std::cout << "======================" << std::endl;
    std::cout << "Testing basic connectivity with security improvements" << std::endl;
    std::cout << "💡 Make sure the enhanced server is running: ./build-release/Release/aiWebSocketsServer.exe" << std::endl;
    std::cout << std::endl;
    
    TestBasicConnection();
    TestMultipleConnections();
    
    std::cout << "🎯 Debug Test Complete" << std::endl;
    
    return 0;
}
