// RAW SOCKET PERFORMANCE TEST
// Tests raw socket transfer rates without HTTP overhead
#include "WebSocket/Socket.h"
#include <iostream>
#include <chrono>
#include <vector>

using namespace WebSocket;
using namespace std::chrono;

int main() {
    std::cout << "🔌 Raw Socket Performance Test" << std::endl;
    std::cout << "===============================" << std::endl;
    
    // Test different data sizes
    std::vector<std::pair<size_t, std::string>> testSizes = {
        {1024, "1 KB"},
        {10240, "10 KB"},
        {102400, "100 KB"},
        {1024000, "1 MB"},
        {5242880, "5 MB"}
    };
    
    for (const auto& [size, name] : testSizes) {
        std::cout << "\n📊 Testing " << name << " transfer..." << std::endl;
        
        // Create test data
        std::vector<uint8_t> testData(size);
        for (size_t i = 0; i < size; i++) {
            testData[i] = static_cast<uint8_t>(i % 256);
        }
        
        // Create server socket
        Socket serverSocket;
        auto createResult = serverSocket.Create(SOCKET_FAMILY::IPV4, SOCKET_TYPE::TCP);
        if (!createResult.IsSuccess()) {
            std::cout << "❌ Server socket creation failed: " << createResult.GetErrorMessage() << std::endl;
            continue;
        }
        
        auto bindResult = serverSocket.Bind("127.0.0.1", 0); // Let OS choose port
        if (!bindResult.IsSuccess()) {
            std::cout << "❌ Server bind failed: " << bindResult.GetErrorMessage() << std::endl;
            serverSocket.Close();
            continue;
        }
        
        auto listenResult = serverSocket.Listen(1);
        if (!listenResult.IsSuccess()) {
            std::cout << "❌ Server listen failed: " << listenResult.GetErrorMessage() << std::endl;
            serverSocket.Close();
            continue;
        }
        
        uint16_t serverPort = serverSocket.LocalPort();
        std::cout << "Server listening on port " << serverPort << std::endl;
        
        // Create client socket
        Socket clientSocket;
        auto clientCreateResult = clientSocket.Create(SOCKET_FAMILY::IPV4, SOCKET_TYPE::TCP);
        if (!clientCreateResult.IsSuccess()) {
            std::cout << "❌ Client socket creation failed: " << clientCreateResult.GetErrorMessage() << std::endl;
            serverSocket.Close();
            continue;
        }
        
        // Connect client to server
        auto connectResult = clientSocket.Connect("127.0.0.1", serverPort);
        if (!connectResult.IsSuccess()) {
            std::cout << "❌ Client connection failed: " << connectResult.GetErrorMessage() << std::endl;
            clientSocket.Close();
            serverSocket.Close();
            continue;
        }
        
        // Accept connection on server
        auto [acceptResult, acceptedSocket] = serverSocket.Accept();
        if (!acceptResult.IsSuccess() || !acceptedSocket) {
            std::cout << "❌ Server accept failed: " << acceptResult.GetErrorMessage() << std::endl;
            clientSocket.Close();
            serverSocket.Close();
            continue;
        }
        
        std::cout << "✅ Connection established" << std::endl;
        
        // Measure send performance
        auto startSend = high_resolution_clock::now();
        auto sendResult = clientSocket.Send(testData);
        auto endSend = high_resolution_clock::now();
        
        if (!sendResult.IsSuccess()) {
            std::cout << "❌ Send failed: " << sendResult.GetErrorMessage() << std::endl;
            clientSocket.Close();
            acceptedSocket->Close();
            serverSocket.Close();
            continue;
        }
        
        // Measure receive performance
        auto startReceive = high_resolution_clock::now();
        std::vector<uint8_t> receivedData;
        size_t totalReceived = 0;
        
        while (totalReceived < size) {
            auto [receiveResult, chunk] = acceptedSocket->Receive(std::min(size_t(65536), size - totalReceived));
            if (receiveResult.IsError() || chunk.empty()) {
                break;
            }
            receivedData.insert(receivedData.end(), chunk.begin(), chunk.end());
            totalReceived += chunk.size();
        }
        
        auto endReceive = high_resolution_clock::now();
        
        // Calculate metrics
        auto sendTime = duration_cast<microseconds>(endSend - startSend).count();
        auto receiveTime = duration_cast<microseconds>(endReceive - startReceive).count();
        auto totalTime = sendTime + receiveTime;
        
        double sendThroughput = (size * 1000000.0) / (sendTime * 1024.0 * 1024.0); // MB/s
        double receiveThroughput = (size * 1000000.0) / (receiveTime * 1024.0 * 1024.0); // MB/s
        double totalThroughput = (size * 1000000.0) / (totalTime * 1024.0 * 1024.0); // MB/s
        
        std::cout << "📤 Send: " << size << " bytes in " << sendTime << "μs (" << sendThroughput << " MB/s)" << std::endl;
        std::cout << "📥 Receive: " << totalReceived << " bytes in " << receiveTime << "μs (" << receiveThroughput << " MB/s)" << std::endl;
        std::cout << "📊 Total: " << size << " bytes in " << totalTime << "μs (" << totalThroughput << " MB/s)" << std::endl;
        
        // Verify data integrity
        bool dataIntegrity = (totalReceived == size) && 
                            std::equal(testData.begin(), testData.end(), receivedData.begin());
        
        std::cout << "✅ Data integrity: " << (dataIntegrity ? "PASSED" : "FAILED") << std::endl;
        
        // Cleanup
        clientSocket.Close();
        acceptedSocket->Close();
        serverSocket.Close();
    }
    
    std::cout << "\n🎯 Raw socket performance test completed!" << std::endl;
    return 0;
}
