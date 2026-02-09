/**
 * @file large_data_test.cpp
 * @brief Large data bidirectional transfer test with integrity verification
 * 
 * This test demonstrates:
 * 1. Client sends large amount of data to server
 * 2. Server verifies received data integrity
 * 3. Server sends 250MB reply back to client
 * 4. Client verifies reply data integrity
 * 5. Performance metrics for both directions
 */

#include "WebSocket/Socket.h"
#include "WebSocket/TestUtilities.h"
#include <iostream>
#include <vector>
#include <string>
#include <chrono>
#include <thread>
#include <iomanip>

using namespace WebSocket;

// Configuration constants
constexpr size_t CLIENT_SEND_SIZE = 50 * 1024 * 1024;  // 50MB from client
constexpr size_t SERVER_REPLY_SIZE = 250 * 1024 * 1024; // 250MB from server
constexpr size_t CHUNK_SIZE = 64 * 1024;              // 64KB chunks

struct TransferMetrics {
    size_t bytesTransferred;
    double transferTimeMs;
    double throughputMBps;
    
    TransferMetrics(size_t bytes, double timeMs) 
        : bytesTransferred(bytes), transferTimeMs(timeMs) {
        throughputMBps = (bytes / (1024.0 * 1024.0)) / (timeMs / 1000.0);
    }
    
    void Print(const std::string& direction) const {
        std::cout << "   " << direction << " Transfer:" << std::endl;
        std::cout << "     Data Size: " << std::fixed << std::setprecision(2) 
                  << (bytesTransferred / (1024.0 * 1024.0)) << " MB" << std::endl;
        std::cout << "     Transfer Time: " << std::setprecision(1) 
                  << transferTimeMs << " ms" << std::endl;
        std::cout << "     Throughput: " << std::setprecision(2) 
                  << throughputMBps << " MB/s" << std::endl;
    }
};

bool SendDataInChunks(Socket& socket, const std::vector<uint8_t>& data, const std::string& name) {
    std::cout << "📤 " << name << " sending " << data.size() << " bytes..." << std::endl;
    
    auto startTime = std::chrono::high_resolution_clock::now();
    size_t bytesSent = 0;
    
    while (bytesSent < data.size()) {
        size_t chunkSize = std::min(CHUNK_SIZE, data.size() - bytesSent);
        std::vector<uint8_t> chunk(data.begin() + bytesSent, data.begin() + bytesSent + chunkSize);
        
        Result sendResult = socket.Send(chunk);
        if (!sendResult.IsSuccess()) {
            std::cout << "❌ " << name << " send failed: " << sendResult.GetErrorMessage() << std::endl;
            return false;
        }
        
        bytesSent += chunkSize;
        
        // Progress indicator for large transfers
        if (bytesSent % (10 * 1024 * 1024) == 0) { // Every 10MB
            double progress = (double)bytesSent / data.size() * 100.0;
            std::cout << "   Progress: " << std::fixed << std::setprecision(1) 
                      << progress << "% (" << bytesSent / (1024 * 1024) << " MB)" << std::endl;
        }
    }
    
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    
    TransferMetrics metrics(data.size(), duration.count());
    metrics.Print(name);
    
    return true;
}

bool ReceiveDataInChunks(Socket& socket, size_t expectedSize, std::vector<uint8_t>& receivedData, const std::string& name) {
    std::cout << "📨 " << name << " receiving " << expectedSize << " bytes..." << std::endl;
    
    auto startTime = std::chrono::high_resolution_clock::now();
    receivedData.clear();
    receivedData.reserve(expectedSize);
    
    while (receivedData.size() < expectedSize) {
        size_t remainingBytes = expectedSize - receivedData.size();
        size_t receiveChunkSize = std::min(CHUNK_SIZE, remainingBytes);
        
        auto receiveResult = socket.Receive(receiveChunkSize);
        if (!receiveResult.first.IsSuccess()) {
            std::cout << "❌ " << name << " receive failed: " << receiveResult.first.GetErrorMessage() << std::endl;
            return false;
        }
        
        const auto& chunk = receiveResult.second;
        if (chunk.empty()) {
            std::cout << "❌ " << name << " connection closed unexpectedly" << std::endl;
            return false;
        }
        
        receivedData.insert(receivedData.end(), chunk.begin(), chunk.end());
        
        // Progress indicator for large transfers
        if (receivedData.size() % (10 * 1024 * 1024) == 0) { // Every 10MB
            double progress = (double)receivedData.size() / expectedSize * 100.0;
            std::cout << "   Progress: " << std::fixed << std::setprecision(1) 
                      << progress << "% (" << receivedData.size() / (1024 * 1024) << " MB)" << std::endl;
        }
    }
    
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    
    TransferMetrics metrics(receivedData.size(), duration.count());
    metrics.Print(name);
    
    return true;
}

int main() {
    std::cout << "WebSocket Large Data Bidirectional Test" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "Client Send: " << (CLIENT_SEND_SIZE / (1024 * 1024)) << " MB" << std::endl;
    std::cout << "Server Reply: " << (SERVER_REPLY_SIZE / (1024 * 1024)) << " MB" << std::endl;
    std::cout << std::endl;
    
    // Initialize socket system
    // Note: Socket system initialization is now automatic
    
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
        
        std::cout << "✅ Client connected to server" << std::endl;
        std::cout << std::endl;
        
        // Phase 1: Client sends large data to server
        std::cout << "=== Phase 1: Client → Server (" << (CLIENT_SEND_SIZE / (1024 * 1024)) << " MB) ===" << std::endl;
        auto clientSendData = WebSocket::createTestData(CLIENT_SEND_SIZE);
        
        if (!SendDataInChunks(clientSocket, clientSendData, "Client")) {
            clientSocket.Close();
            return;
        }
        
        std::cout << "✅ Client completed sending data to server" << std::endl;
        std::cout << std::endl;
        
        // Phase 2: Client receives large reply from server
        std::cout << "=== Phase 2: Server → Client (" << (SERVER_REPLY_SIZE / (1024 * 1024)) << " MB) ===" << std::endl;
        std::vector<uint8_t> clientReceivedData;
        
        if (!ReceiveDataInChunks(clientSocket, SERVER_REPLY_SIZE, clientReceivedData, "Client")) {
            clientSocket.Close();
            return;
        }
        
        std::cout << "✅ Client completed receiving data from server" << std::endl;
        std::cout << std::endl;
        
        // Verify server reply integrity
        std::cout << "=== Phase 3: Client Verification ===" << std::endl;
        if (WebSocket::verifyDataIntegrity(clientReceivedData, SERVER_REPLY_SIZE)) {
            std::cout << "✅ Client verified server reply data integrity - PASSED" << std::endl;
        } else {
            std::cout << "❌ Client verified server reply data integrity - FAILED" << std::endl;
        }
        
        clientSocket.Close();
    });
    
    // Server accepts client connection
    auto acceptResult = serverSocket.Accept();
    if (!acceptResult.first.IsSuccess() || !acceptResult.second) {
        std::cout << "❌ Server failed to accept client" << std::endl;

        return 1;
    }
    
    std::cout << "✅ Server accepted client connection" << std::endl;
    std::cout << std::endl;
    
    // Phase 1: Server receives data from client
    std::cout << "=== Phase 1: Server Receiving from Client ===" << std::endl;
    std::vector<uint8_t> serverReceivedData;
    
    if (!ReceiveDataInChunks(*acceptResult.second, CLIENT_SEND_SIZE, serverReceivedData, "Server")) {
        clientThread.join();
        acceptResult.second->Close();
        serverSocket.Close();

        return 1;
    }
    
    std::cout << "✅ Server completed receiving data from client" << std::endl;
    std::cout << std::endl;
    
    // Phase 2: Server verifies client data integrity
    std::cout << "=== Phase 2: Server Verification ===" << std::endl;
    bool clientDataIntegrity = WebSocket::verifyDataIntegrity(serverReceivedData, CLIENT_SEND_SIZE);
    
    if (clientDataIntegrity) {
        std::cout << "✅ Server verified client data integrity - PASSED" << std::endl;
    } else {
        std::cout << "❌ Server verified client data integrity - FAILED" << std::endl;
    }
    std::cout << std::endl;
    
    // Phase 3: Server sends large reply to client
    std::cout << "=== Phase 3: Server Sending Reply ===" << std::endl;
    auto serverReplyData = WebSocket::createTestData(SERVER_REPLY_SIZE);
    
    if (!SendDataInChunks(*acceptResult.second, serverReplyData, "Server")) {
        clientThread.join();
        acceptResult.second->Close();
        serverSocket.Close();

        return 1;
    }
    
    std::cout << "✅ Server completed sending reply to client" << std::endl;
    std::cout << std::endl;
    
    // Wait for client to complete
    clientThread.join();
    
    // Final summary
    std::cout << "=== FINAL RESULTS ===" << std::endl;
    std::cout << "Total Data Transferred: " << (CLIENT_SEND_SIZE + SERVER_REPLY_SIZE) / (1024 * 1024) << " MB" << std::endl;
    std::cout << "Client → Server: " << (clientDataIntegrity ? "✅ PASSED" : "❌ FAILED") << std::endl;
    std::cout << "Server → Client: Integrity verified by client" << std::endl;
    std::cout << "🎉 Large data bidirectional test completed!" << std::endl;
    
    // Cleanup
    acceptResult.second->Close();
    serverSocket.Close();

    
    return clientDataIntegrity ? 0 : 1;
}
