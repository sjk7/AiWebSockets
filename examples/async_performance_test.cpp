#include <iostream>
#include <chrono>
#include <vector>
#include <thread>
#include "WebSocket/Socket.h"

using namespace WebSocket;

int main() {
    std::cout << "WebSocket Async I/O Performance Test" << std::endl;
    std::cout << "====================================" << std::endl;
    
    // Create server socket
    Socket serverSocket;
    if (!serverSocket.Create(SOCKET_FAMILY::IPV4, SOCKET_TYPE::TCP).IsSuccess()) {
        std::cout << "❌ Failed to create server socket" << std::endl;
        return 1;
    }
    
    // Enable async I/O on server
    auto asyncResult = serverSocket.EnableAsyncIO();
    if (!asyncResult.IsSuccess()) {
        std::cout << "❌ Failed to enable async I/O on server: " << asyncResult.GetErrorMessage() << std::endl;
        return 1;
    }
    std::cout << "✅ Server async I/O enabled" << std::endl;
    
    // Configure server
    serverSocket.ReuseAddress(true);
    serverSocket.SendBufferSize(1024 * 1024);
    serverSocket.ReceiveBufferSize(1024 * 1024);
    
    if (!serverSocket.Bind("127.0.0.1", 0).IsSuccess()) {
        std::cout << "❌ Failed to bind server socket" << std::endl;
        return 1;
    }
    
    if (!serverSocket.Listen(5).IsSuccess()) {
        std::cout << "❌ Failed to listen on server socket" << std::endl;
        return 1;
    }
    
    std::string serverAddress = serverSocket.LocalAddress();
    uint16_t serverPort = serverSocket.LocalPort();
    
    std::cout << "🚀 Server listening on " << serverAddress << ":" << serverPort << std::endl;
    
    // Create client socket
    Socket clientSocket;
    if (!clientSocket.Create(SOCKET_FAMILY::IPV4, SOCKET_TYPE::TCP).IsSuccess()) {
        std::cout << "❌ Failed to create client socket" << std::endl;
        return 1;
    }
    
    // Enable async I/O on client
    asyncResult = clientSocket.EnableAsyncIO();
    if (!asyncResult.IsSuccess()) {
        std::cout << "❌ Failed to enable async I/O on client: " << asyncResult.GetErrorMessage() << std::endl;
        return 1;
    }
    std::cout << "✅ Client async I/O enabled" << std::endl;
    
    // Configure client
    clientSocket.SendBufferSize(1024 * 1024);
    clientSocket.ReceiveBufferSize(1024 * 1024);
    
    // Connect to server
    if (!clientSocket.Connect(serverAddress, serverPort).IsSuccess()) {
        std::cout << "❌ Failed to connect client to server" << std::endl;
        return 1;
    }
    std::cout << "✅ Client connected to server" << std::endl;
    
    // Accept connection
    auto [acceptResult, acceptedSocket] = serverSocket.Accept();
    if (!acceptResult.IsSuccess() || !acceptedSocket) {
        std::cout << "❌ Failed to accept client connection" << std::endl;
        return 1;
    }
    std::cout << "✅ Server accepted client connection" << std::endl;
    
    // Enable async I/O on accepted socket
    asyncResult = acceptedSocket->EnableAsyncIO();
    if (!asyncResult.IsSuccess()) {
        std::cout << "❌ Failed to enable async I/O on accepted socket: " << asyncResult.GetErrorMessage() << std::endl;
        return 1;
    }
    std::cout << "✅ Accepted socket async I/O enabled" << std::endl;
    
    // Test data
    const size_t testDataSize = 1024 * 1024; // 1MB
    std::vector<uint8_t> testData(testDataSize, 0x42);
    
    std::cout << "📤 Testing async send of " << (testDataSize / 1024) << " KB..." << std::endl;
    
    // Measure async send performance
    auto startTime = std::chrono::high_resolution_clock::now();
    
    // Use async send
    auto sendResult = clientSocket.SendAsync(testData);
    if (!sendResult.IsSuccess()) {
        std::cout << "❌ Async send failed: " << sendResult.GetErrorMessage() << std::endl;
        return 1;
    }
    
    auto endTime = std::chrono::high_resolution_clock::now();
    auto sendDuration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);
    
    std::cout << "✅ Async send initiated in " << sendDuration.count() << " microseconds" << std::endl;
    
    // Use async receive
    std::cout << "📨 Testing async receive..." << std::endl;
    startTime = std::chrono::high_resolution_clock::now();
    
    auto receiveResult = acceptedSocket->ReceiveAsync(testDataSize);
    if (!receiveResult.IsSuccess()) {
        std::cout << "❌ Async receive failed: " << receiveResult.GetErrorMessage() << std::endl;
        return 1;
    }
    
    endTime = std::chrono::high_resolution_clock::now();
    auto receiveDuration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);
    
    std::cout << "✅ Async receive initiated in " << receiveDuration.count() << " microseconds" << std::endl;
    
    // Give some time for async operations to complete
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // Test regular send/receive for comparison
    std::cout << "📊 Comparing with synchronous operations..." << std::endl;
    
    startTime = std::chrono::high_resolution_clock::now();
    auto syncSendResult = clientSocket.Send(testData);
    endTime = std::chrono::high_resolution_clock::now();
    auto syncSendDuration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);
    
    startTime = std::chrono::high_resolution_clock::now();
    auto [syncReceiveResult, receivedData] = acceptedSocket->Receive(testDataSize);
    endTime = std::chrono::high_resolution_clock::now();
    auto syncReceiveDuration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);
    
    std::cout << "📈 Performance Comparison:" << std::endl;
    std::cout << "  Async Send: " << sendDuration.count() << " μs" << std::endl;
    std::cout << "  Sync Send: " << syncSendDuration.count() << " μs" << std::endl;
    std::cout << "  Async Receive: " << receiveDuration.count() << " μs" << std::endl;
    std::cout << "  Sync Receive: " << syncReceiveDuration.count() << " μs" << std::endl;
    
    if (sendDuration.count() < syncSendDuration.count()) {
        double improvement = (double)(syncSendDuration.count() - sendDuration.count()) / syncSendDuration.count() * 100.0;
        std::cout << "🚀 Async send is " << improvement << "% faster!" << std::endl;
    }
    
    if (receiveDuration.count() < syncReceiveDuration.count()) {
        double improvement = (double)(syncReceiveDuration.count() - receiveDuration.count()) / syncReceiveDuration.count() * 100.0;
        std::cout << "🚀 Async receive is " << improvement << "% faster!" << std::endl;
    }
    
    std::cout << "🎉 Async I/O performance test completed!" << std::endl;
    
    return 0;
}
