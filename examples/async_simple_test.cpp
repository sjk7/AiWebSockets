// SIMPLE ASYNC I/O COMPARISON
// Quick test to see if async actually helps
#include "WebSocket/Socket.h"
#include <iostream>
#include <chrono>

using namespace WebSocket;
using namespace std::chrono;

int main() {
    std::cout << "🔍 Simple Async I/O Comparison" << std::endl;
    std::cout << "===============================" << std::endl;
    
    const int numTests = 100; // Fewer tests for cleaner output
    const size_t dataSize = 10240; // 10KB
    std::vector<uint8_t> testData(dataSize, 'A');
    
    // Test 1: Synchronous
    {
        std::cout << "\n📊 Synchronous Operations:" << std::endl;
        
        double totalTime = 0;
        
        for (int i = 0; i < numTests; i++) {
            Socket server;
            server.Create(SOCKET_FAMILY::IPV4, SOCKET_TYPE::TCP);
            server.Bind("127.0.0.1", 0);
            server.Listen(1);
            uint16_t port = server.LocalPort();
            
            Socket client;
            client.Create(SOCKET_FAMILY::IPV4, SOCKET_TYPE::TCP);
            client.Connect("127.0.0.1", port);
            
            auto [acceptResult, accepted] = server.Accept();
            
            if (acceptResult.IsSuccess() && accepted) {
                auto start = high_resolution_clock::now();
                
                client.Send(testData);
                
                std::vector<uint8_t> received;
                size_t total = 0;
                while (total < dataSize) {
                    auto [result, chunk] = accepted->Receive(4096);
                    if (result.IsError() || chunk.empty()) break;
                    received.insert(received.end(), chunk.begin(), chunk.end());
                    total += chunk.size();
                }
                
                auto end = high_resolution_clock::now();
                totalTime += duration_cast<microseconds>(end - start).count();
                
                // Proper cleanup order
                accepted->Close();  // Close connection first
                client.Close();     // Then close client
            }
            
            server.Close();        // Always close server last
        }
        
        double avgTime = totalTime / numTests;
        double throughput = (dataSize * 1000000.0) / (avgTime * 1024.0 * 1024.0);
        
        std::cout << "   Average time: " << avgTime << " μs" << std::endl;
        std::cout << "   Throughput: " << throughput << " MB/s" << std::endl;
        
        double syncThroughput = throughput;
        
        // Test 2: Asynchronous
        std::cout << "\n📊 Asynchronous Operations:" << std::endl;
        
        totalTime = 0;
        
        for (int i = 0; i < numTests; i++) {
            Socket server;
            server.Create(SOCKET_FAMILY::IPV4, SOCKET_TYPE::TCP);
            server.Bind("127.0.0.1", 0);
            server.Listen(1);
            uint16_t port = server.LocalPort();
            
            Socket client;
            client.Create(SOCKET_FAMILY::IPV4, SOCKET_TYPE::TCP);
            client.EnableAsyncIO();
            client.Connect("127.0.0.1", port);
            
            auto [acceptResult, accepted] = server.Accept();
            if (accepted) {
                accepted->EnableAsyncIO();
            }
            
            if (acceptResult.IsSuccess() && accepted) {
                auto start = high_resolution_clock::now();
                
                auto sendResult = client.SendAsync(testData);
                
                std::vector<uint8_t> received;
                size_t total = 0;
                while (total < dataSize) {
                    auto [result, chunk] = accepted->Receive(4096);
                    if (result.IsError() || chunk.empty()) break;
                    received.insert(received.end(), chunk.begin(), chunk.end());
                    total += chunk.size();
                }
                
                auto end = high_resolution_clock::now();
                totalTime += duration_cast<microseconds>(end - start).count();
                
                // Proper cleanup order
                accepted->Close();  // Close connection first
                client.Close();     // Then close client
            }
            
            server.Close();        // Always close server last
        }
        
        avgTime = totalTime / numTests;
        throughput = (dataSize * 1000000.0) / (avgTime * 1024.0 * 1024.0);
        
        std::cout << "   Average time: " << avgTime << " μs" << std::endl;
        std::cout << "   Throughput: " << throughput << " MB/s" << std::endl;
        
        // Comparison
        double improvement = ((throughput - syncThroughput) / syncThroughput) * 100;
        std::cout << "\n🎯 ASYNC I/O IMPACT:" << std::endl;
        std::cout << "   Performance improvement: " << improvement << "%" << std::endl;
        
        if (improvement > 10) {
            std::cout << "   ✅ Async I/O provides significant benefit!" << std::endl;
        } else if (improvement > 0) {
            std::cout << "   📈 Async I/O provides minor benefit" << std::endl;
        } else {
            std::cout << "   ❌ Async I/O provides no benefit (or hurts performance)" << std::endl;
        }
    }
    
    return 0;
}
