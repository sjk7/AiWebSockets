/**
 * @file server_to_client_test.cpp
 * @brief Example demonstrating server-to-client data transfer using TestUtilities
 * 
 * This example shows how to use the TestUtilities header for server-initiated
 * data transfers and verification.
 */

#include "WebSocket/Socket.h"
#include "WebSocket/TestUtilities.h"
#include <iostream>
#include <thread>
#include <chrono>

using namespace WebSocket;

int main() {
    std::cout << "WebSocket Server-to-Client Test" << std::endl;
    std::cout << "===============================" << std::endl;
    
    // Initialize socket system
    // Note: Socket system is now automatically initialized when first socket is created
    
    // Create server socket
    Socket serverSocket;
    if (!serverSocket.Create(SOCKET_FAMILY::IPV4, SOCKET_TYPE::TCP).IsSuccess()) {
        std::cout << "❌ Failed to create server socket" << std::endl;
        return 1;
    }
    
    if (!serverSocket.ReuseAddress(true).IsSuccess()) {
        std::cout << "❌ Failed to set reuse address" << std::endl;
        return 1;
    }
    
    if (!serverSocket.Bind("127.0.0.1", 0).IsSuccess()) {
        std::cout << "❌ Failed to bind server socket" << std::endl;
        return 1;
    }
    
    if (!serverSocket.Listen(1).IsSuccess()) {
        std::cout << "❌ Failed to listen on server socket" << std::endl;
        return 1;
    }
    
    std::string serverAddress = serverSocket.LocalAddress();
    uint16_t serverPort = serverSocket.LocalPort();
    
    std::cout << "Server listening on " << serverAddress << ":" << serverPort << std::endl;
    
    // Start client in separate thread
    std::thread clientThread([&]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        Socket clientSocket;
        if (!clientSocket.Create(SOCKET_FAMILY::IPV4, SOCKET_TYPE::TCP).IsSuccess()) {
            std::cout << "❌ Client failed to create socket" << std::endl;
            return;
        }
        
        if (!clientSocket.Connect(serverAddress, serverPort).IsSuccess()) {
            std::cout << "❌ Client failed to connect" << std::endl;
            return;
        }
        
        std::cout << "✅ Client connected" << std::endl;
        
        // Receive data from server
        const size_t expectedSize = 1024;
        auto receiveResult = clientSocket.Receive(expectedSize);
        
        if (!receiveResult.first.IsSuccess()) {
            std::cout << "❌ Client receive failed: " << receiveResult.first.GetErrorMessage() << std::endl;
            return;
        }
        
        std::vector<uint8_t> receivedData = receiveResult.second;
        std::cout << "📨 Client received " << receivedData.size() << " bytes" << std::endl;
        
        // Verify data integrity using TestUtilities
        if (WebSocket::VerifyDataIntegrity(receivedData, expectedSize)) {
            std::cout << "✅ Data integrity verified - Server-to-client transfer successful!" << std::endl;
        } else {
            std::cout << "❌ Data integrity check failed" << std::endl;
        }
        
        clientSocket.Close();
    });
    
    // Accept client connection
    auto acceptResult = serverSocket.Accept();
    if (!acceptResult.first.IsSuccess() || !acceptResult.second) {
        std::cout << "❌ Server failed to accept client" << std::endl;
        return 1;
    }
    
    std::cout << "✅ Server accepted client connection" << std::endl;
    
    // Create test data using TestUtilities
    const size_t testDataSize = 1024;
    std::vector<uint8_t> testData = WebSocket::CreateTestData(testDataSize);
    
    std::cout << "📤 Server sending " << testData.size() << " bytes to client..." << std::endl;
    
    // Send data to client
    Result sendResult = acceptResult.second->Send(testData);
    if (!sendResult.IsSuccess()) {
        std::cout << "❌ Server send failed: " << sendResult.GetErrorMessage() << std::endl;
        return 1;
    }
    
    std::cout << "✅ Server sent data successfully" << std::endl;
    
    // Wait for client to complete
    clientThread.join();
    
    // Cleanup
    acceptResult.second->Close();
    serverSocket.Close();
    
    // Cleanup socket system

    
    std::cout << "🎉 Server-to-client test completed!" << std::endl;
    return 0;
}
