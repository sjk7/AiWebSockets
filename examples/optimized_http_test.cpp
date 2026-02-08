// OPTIMIZED HTTP PERFORMANCE TEST
// Tests optimized HTTP response generation
#include "WebSocket/Socket.h"
#include <iostream>
#include <chrono>
#include <vector>
#include <sstream>

using namespace WebSocket;
using namespace std::chrono;

// Optimized HTTP response generation (no stringstream)
std::vector<uint8_t> GenerateOptimizedHTTPResponse(const std::string& status, 
                                                  const std::string& contentType, 
                                                  const std::string& body) {
    std::vector<uint8_t> response;
    response.reserve(256 + body.size()); // Pre-allocate
    
    // Add status line
    const char* statusLine = "HTTP/1.1 ";
    response.insert(response.end(), statusLine, statusLine + 9);
    response.insert(response.end(), status.begin(), status.end());
    response.push_back('\r'); response.push_back('\n');
    
    // Add content type
    const char* contentTypeHeader = "Content-Type: ";
    response.insert(response.end(), contentTypeHeader, contentTypeHeader + 14);
    response.insert(response.end(), contentType.begin(), contentType.end());
    const char* charset = "; charset=UTF-8\r\n";
    response.insert(response.end(), charset, charset + 17);
    
    // Add content length
    const char* contentLengthHeader = "Content-Length: ";
    response.insert(response.end(), contentLengthHeader, contentLengthHeader + 16);
    std::string lengthStr = std::to_string(body.length());
    response.insert(response.end(), lengthStr.begin(), lengthStr.end());
    response.push_back('\r'); response.push_back('\n');
    
    // Add connection close
    const char* connectionHeader = "Connection: close\r\n";
    response.insert(response.end(), connectionHeader, connectionHeader + 19);
    
    // Add blank line
    response.push_back('\r'); response.push_back('\n');
    
    // Add body
    response.insert(response.end(), body.begin(), body.end());
    
    return response;
}

// Original HTTP response generation (stringstream)
std::vector<uint8_t> GenerateOriginalHTTPResponse(const std::string& status, 
                                                  const std::string& contentType, 
                                                  const std::string& body) {
    std::stringstream response;
    response << "HTTP/1.1 " << status << "\r\n";
    response << "Content-Type: " << contentType << "; charset=UTF-8\r\n";
    response << "Content-Length: " << body.length() << "\r\n";
    response << "Connection: close\r\n";
    response << "\r\n";
    response << body;
    
    std::string responseStr = response.str();
    return std::vector<uint8_t>(responseStr.begin(), responseStr.end());
}

int main() {
    std::cout << "🚀 Optimized HTTP Performance Test" << std::endl;
    std::cout << "===================================" << std::endl;
    
    // Test data
    std::string testBody(10240, 'A'); // 10KB
    const int iterations = 1000;
    
    std::cout << "Testing HTTP response generation with " << testBody.size() 
              << " bytes body, " << iterations << " iterations" << std::endl;
    
    // Test original method
    {
        auto start = high_resolution_clock::now();
        
        for (int i = 0; i < iterations; i++) {
            auto response = GenerateOriginalHTTPResponse("200 OK", "text/html", testBody);
        }
        
        auto end = high_resolution_clock::now();
        auto duration = duration_cast<microseconds>(end - start).count();
        
        double avgTime = static_cast<double>(duration) / iterations;
        double throughput = (testBody.size() * 1000000.0) / (avgTime * 1024.0 * 1024.0);
        
        std::cout << "\n📊 Original Method (stringstream):" << std::endl;
        std::cout << "   Total time: " << duration << " μs" << std::endl;
        std::cout << "   Avg per response: " << avgTime << " μs" << std::endl;
        std::cout << "   Throughput: " << throughput << " MB/s" << std::endl;
    }
    
    // Test optimized method
    {
        auto start = high_resolution_clock::now();
        
        for (int i = 0; i < iterations; i++) {
            auto response = GenerateOptimizedHTTPResponse("200 OK", "text/html", testBody);
        }
        
        auto end = high_resolution_clock::now();
        auto duration = duration_cast<microseconds>(end - start).count();
        
        double avgTime = static_cast<double>(duration) / iterations;
        double throughput = (testBody.size() * 1000000.0) / (avgTime * 1024.0 * 1024.0);
        
        std::cout << "\n🚀 Optimized Method (vector operations):" << std::endl;
        std::cout << "   Total time: " << duration << " μs" << std::endl;
        std::cout << "   Avg per response: " << avgTime << " μs" << std::endl;
        std::cout << "   Throughput: " << throughput << " MB/s" << std::endl;
    }
    
    // Test with actual socket transfer
    std::cout << "\n🔌 Testing with actual socket transfer..." << std::endl;
    
    // Create server
    Socket serverSocket;
    serverSocket.Create(SOCKET_FAMILY::IPV4, SOCKET_TYPE::TCP);
    serverSocket.Bind("127.0.0.1", 0);
    serverSocket.Listen(1);
    uint16_t serverPort = serverSocket.LocalPort();
    
    // Create client
    Socket clientSocket;
    clientSocket.Create(SOCKET_FAMILY::IPV4, SOCKET_TYPE::TCP);
    clientSocket.Connect("127.0.0.1", serverPort);
    
    // Accept connection
    auto [acceptResult, acceptedSocket] = serverSocket.Accept();
    
    if (acceptResult.IsSuccess() && acceptedSocket) {
        // Test optimized HTTP response over socket
        auto start = high_resolution_clock::now();
        
        for (int i = 0; i < 100; i++) {
            auto response = GenerateOptimizedHTTPResponse("200 OK", "text/html", testBody);
            clientSocket.Send(response);
            
            // Receive response
            std::vector<uint8_t> received;
            size_t total = 0;
            while (total < response.size()) {
                auto [recvResult, chunk] = acceptedSocket->Receive(8192);
                if (recvResult.IsError() || chunk.empty()) break;
                received.insert(received.end(), chunk.begin(), chunk.end());
                total += chunk.size();
            }
        }
        
        auto end = high_resolution_clock::now();
        auto duration = duration_cast<microseconds>(end - start).count();
        
        double avgTime = static_cast<double>(duration) / 100;
        double throughput = (testBody.size() * 1000000.0) / (avgTime * 1024.0 * 1024.0);
        
        std::cout << "📊 Socket Transfer (Optimized HTTP):" << std::endl;
        std::cout << "   Avg per request: " << avgTime << " μs" << std::endl;
        std::cout << "   Throughput: " << throughput << " MB/s" << std::endl;
    }
    
    clientSocket.Close();
    acceptedSocket->Close();
    serverSocket.Close();
    
    std::cout << "\n✅ Performance test completed!" << std::endl;
    return 0;
}
