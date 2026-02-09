// DIRECT ASYNC I/O TEST
// Isolates async I/O performance impact
#include "WebSocket/Socket.h"
#include <iostream>
#include <chrono>

using namespace WebSocket;
using namespace std::chrono;

int main() {
    std::cout << "🔍 Direct Async I/O Impact Test" << std::endl;
    std::cout << "===============================" << std::endl;
    
    const int numTests = 1000;
    const size_t dataSize = 10240; // 10KB
    std::vector<uint8_t> testData(dataSize, 'A');
    
    // Test 1: Synchronous send/receive
    {
        std::cout << "\n📊 Testing Synchronous Operations..." << std::endl;
        
        double totalTime = 0;
        
        for (int i = 0; i < numTests; i++) {
            // Create server
            Socket server;
            server.create(socketFamily::IPV4, socketType::TCP);
            server.bind("127.0.0.1", 0);
            server.listen(1);
            uint16_t port = server.localPort();
            
            // Create client
            Socket client;
            client.create(socketFamily::IPV4, socketType::TCP);
            client.connect("127.0.0.1", port);
            
            auto [acceptResult, accepted] = server.accept();
            
            if (acceptResult.isSuccess() && accepted) {
                auto start = high_resolution_clock::now();
                
                // Synchronous send
                client.send(testData);
                
                // Synchronous receive
                std::vector<uint8_t> received;
                size_t total = 0;
                while (total < dataSize) {
                    auto [result, chunk] = accepted->receive(4096);
                    if (result.isError() || chunk.empty()) break;
                    received.insert(received.end(), chunk.begin(), chunk.end());
                    total += chunk.size();
                }
                
                auto end = high_resolution_clock::now();
                totalTime += duration_cast<microseconds>(end - start).count();
            }
            
            client.close();
            accepted->close();
            server.close();
        }
        
        double avgTime = totalTime / numTests;
        double throughput = (dataSize * 1000000.0) / (avgTime * 1024.0 * 1024.0);
        
        std::cout << "   Average time: " << avgTime << " μs" << std::endl;
        std::cout << "   Throughput: " << throughput << " MB/s" << std::endl;
    }
    
    // Test 2: Asynchronous send/receive
    {
        std::cout << "\n📊 Testing Asynchronous Operations..." << std::endl;
        
        double totalTime = 0;
        
        for (int i = 0; i < numTests; i++) {
            // Create server
            Socket server;
            server.create(socketFamily::IPV4, socketType::TCP);
            server.bind("127.0.0.1", 0);
            server.listen(1);
            uint16_t port = server.localPort();
            
            // Create client
            Socket client;
            client.create(socketFamily::IPV4, socketType::TCP);
            client.enableAsyncIO(); // Enable async I/O
            client.connect("127.0.0.1", port);
            
            auto [acceptResult, accepted] = server.accept();
            if (accepted) {
                accepted->enableAsyncIO(); // Enable async I/O
            }
            
            if (acceptResult.isSuccess() && accepted) {
                auto start = high_resolution_clock::now();
                
                // Asynchronous send
                auto sendResult = client.sendAsync(testData);
                
                // Asynchronous receive
                std::vector<uint8_t> received;
                size_t total = 0;
                while (total < dataSize) {
                    auto [result, chunk] = accepted->receive(4096);
                    if (result.isError() || chunk.empty()) break;
                    received.insert(received.end(), chunk.begin(), chunk.end());
                    total += chunk.size();
                }
                
                auto end = high_resolution_clock::now();
                totalTime += duration_cast<microseconds>(end - start).count();
            }
            
            client.close();
            accepted->close();
            server.close();
        }
        
        double avgTime = totalTime / numTests;
        double throughput = (dataSize * 1000000.0) / (avgTime * 1024.0 * 1024.0);
        
        std::cout << "   Average time: " << avgTime << " μs" << std::endl;
        std::cout << "   Throughput: " << throughput << " MB/s" << std::endl;
    }
    
    std::cout << "\n🎯 CONCLUSION:" << std::endl;
    std::cout << "Test the difference between sync and async operations!" << std::endl;
    
    return 0;
}
