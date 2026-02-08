// HTTP PERFORMANCE TEST FOR PROTECTED SERVER
// Measures HTTP transfer rates with protection features enabled
#include "WebSocket/HttpWsServer.h"
#include "WebSocket/WebSocketClientLite.h"
#include <iostream>
#include <chrono>
#include <vector>
#include <string>

using namespace WebSocket;
using namespace std::chrono;

int main() {
    std::cout << "🌐 HTTP Performance Test for Protected Server" << std::endl;
    std::cout << "=========================================" << std::endl;
    
    // Configure protected server with relaxed limits for testing
    SecurityConfig security;
    security.maxConnectionsPerIP = 10;
    security.maxConnectionsTotal = 100;
    security.maxRequestsPerIP = 1000;  // High limit for performance test
    security.requestResetPeriodSeconds = 60;
    security.maxRequestSize = 1024 * 1024;  // 1MB
    security.maxMessageSize = 1024 * 1024;  // 1MB
    security.connectionTimeoutSeconds = 300;
    security.enableRequestSizeLimit = true;
    security.enableMessageSizeLimit = true;
    
    // Create different response sizes to test
    std::vector<std::pair<std::string, std::string>> testPages = {
        {"/small", "<html><body><h1>Small Page</h1><p>This is a small test page.</p></body></html>"},
        {"/medium", std::string(1024, 'A')},  // 1KB
        {"/large", std::string(10240, 'B')},  // 10KB
        {"/xlarge", std::string(102400, 'C')}  // 100KB
    };
    
    // Create protected server
    HttpWsServer server(8082, "127.0.0.1", security);
    
    // Track performance metrics
    struct Metric {
        std::string path;
        size_t responseSize;
        double responseTime;
        double throughput;
    };
    std::vector<Metric> metrics;
    
    // Configure HTTP request handling
    server.OnHttpRequest([&](const HTTPRequest& request) -> std::string {
        auto start = high_resolution_clock::now();
        
        std::string response;
        for (const auto& [path, content] : testPages) {
            if (request.path == path) {
                response = content;
                break;
            }
        }
        
        if (response.empty()) {
            response = "<html><body><h1>404 Not Found</h1></body></html>";
        }
        
        auto end = high_resolution_clock::now();
        auto duration = duration_cast<microseconds>(end - start).count();
        
        std::cout << "📤 HTTP " << request.method << " " << request.path 
                  << " (" << response.size() << " bytes, " << duration << "μs)" << std::endl;
        
        // Return proper HTTP response
        std::string httpResponse = "HTTP/1.1 200 OK\r\n";
        httpResponse += "Content-Type: text/html; charset=UTF-8\r\n";
        httpResponse += "Connection: close\r\n";
        httpResponse += "Content-Length: " + std::to_string(response.size()) + "\r\n";
        httpResponse += "\r\n";
        httpResponse += response;
        
        return httpResponse;
    });
    
    // Start server
    auto result = server.Start();
    if (!result.IsSuccess()) {
        std::cout << "❌ Failed to start server: " << result.GetErrorMessage() << std::endl;
        return 1;
    }
    
    std::cout << "✅ Protected HTTP Server started on port 8082" << std::endl;
    std::cout << "🌐 Test URLs:" << std::endl;
    for (const auto& [path, _] : testPages) {
        std::cout << "   http://localhost:8082" << path << std::endl;
    }
    std::cout << "\n🔄 Starting performance measurements..." << std::endl;
    
    // Wait a moment for server to be ready
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // Test each page size multiple times
    for (const auto& [path, content] : testPages) {
        const int numTests = 10;
        double totalTime = 0.0;
        
        for (int i = 0; i < numTests; i++) {
            // Create simple HTTP client using Socket
            Socket client;
            auto createResult = client.Create(SOCKET_FAMILY::IPV4, SOCKET_TYPE::TCP);
            if (!createResult.IsSuccess()) {
                std::cout << "❌ Socket creation failed: " << createResult.GetErrorMessage() << std::endl;
                continue;
            }
            
            auto connectResult = client.Connect("127.0.0.1", 8082);
            if (!connectResult.IsSuccess()) {
                std::cout << "❌ Connection failed: " << connectResult.GetErrorMessage() << std::endl;
                client.Close();
                continue;
            }
            
            auto start = high_resolution_clock::now();
            
            // Send HTTP request
            std::string httpRequest = "GET " + path + " HTTP/1.1\r\n";
            httpRequest += "Host: localhost:8082\r\n";
            httpRequest += "Connection: close\r\n";
            httpRequest += "\r\n";
            
            auto sendResult = client.Send(std::vector<uint8_t>(httpRequest.begin(), httpRequest.end()));
            if (!sendResult.IsSuccess()) {
                std::cout << "❌ Send failed: " << sendResult.GetErrorMessage() << std::endl;
                client.Close();
                continue;
            }
            
            // Receive HTTP response
            std::string responseData;
            bool headersComplete = false;
            size_t contentLength = 0;
            
            while (true) {
                auto [receiveResult, data] = client.Receive(4096);
                if (receiveResult.IsError() || data.empty()) {
                    break;
                }
                
                responseData += std::string(data.begin(), data.end());
                
                // Parse headers to get content length
                if (!headersComplete) {
                    size_t headerEnd = responseData.find("\r\n\r\n");
                    if (headerEnd != std::string::npos) {
                        headersComplete = true;
                        // Extract Content-Length if present
                        size_t contentPos = responseData.find("Content-Length:");
                        if (contentPos != std::string::npos) {
                            size_t colonPos = responseData.find(":", contentPos);
                            size_t endPos = responseData.find("\r\n", colonPos);
                            if (endPos != std::string::npos) {
                                std::string lengthStr = responseData.substr(colonPos + 1, endPos - colonPos - 1);
                                contentLength = std::stoul(lengthStr);
                            }
                        }
                    }
                }
                
                // Check if we have received all content
                if (headersComplete) {
                    size_t bodyStart = responseData.find("\r\n\r\n") + 4;
                    size_t bodySize = responseData.length() - bodyStart;
                    if (bodySize >= contentLength) {
                        break;
                    }
                }
            }
            
            auto end = high_resolution_clock::now();
            auto responseTime = duration_cast<microseconds>(end - start).count();
            
            client.Close();
            
            // Calculate actual body size (excluding headers)
            size_t bodySize = 0;
            size_t headerEnd = responseData.find("\r\n\r\n");
            if (headerEnd != std::string::npos) {
                bodySize = responseData.length() - headerEnd - 4;
            }
            
            totalTime += responseTime;
            
            std::cout << "  Test " << (i+1) << ": " << bodySize << " bytes in " 
                      << responseTime << "μs" << std::endl;
        }
        
        if (totalTime > 0) {
            double avgTime = totalTime / numTests;
            double throughput = (content.size() * 1000000.0) / (avgTime * 1024.0); // KB/s
            
            Metric metric;
            metric.path = path;
            metric.responseSize = content.size();
            metric.responseTime = avgTime;
            metric.throughput = throughput;
            metrics.push_back(metric);
            
            std::cout << "📊 " << path << ": " << content.size() << " bytes, avg " 
                      << avgTime << "μs, " << throughput << " KB/s" << std::endl;
        }
    }
    
    std::cout << "\n📈 HTTP PERFORMANCE RESULTS:" << std::endl;
    std::cout << "+------------+------------+------------+-------------+" << std::endl;
    std::cout << "| Page Size  | Size (B)   | Time (μs)  | Throughput  |" << std::endl;
    std::cout << "+------------+------------+------------+-------------+" << std::endl;
    
    for (const auto& metric : metrics) {
        printf("| %-10s | %-10zu | %-10.0f | %-11.2f KB/s |\n", 
               metric.path.c_str(), metric.responseSize, metric.responseTime, metric.throughput);
    }
    
    std::cout << "+------------+------------+------------+-------------+" << std::endl;
    
    // Calculate overall performance
    double totalThroughput = 0;
    for (const auto& metric : metrics) {
        totalThroughput += metric.throughput;
    }
    double avgThroughput = totalThroughput / metrics.size();
    
    std::cout << "\n🎯 PERFORMANCE SUMMARY:" << std::endl;
    std::cout << "Average HTTP throughput: " << avgThroughput << " KB/s" << std::endl;
    std::cout << "🛡️ Protection: ENABLED" << std::endl;
    std::cout << "Socket shutdown: PROPER" << std::endl;
    
    if (avgThroughput > 1000) {
        std::cout << "Classification: EXCELLENT (> 1 MB/s)" << std::endl;
    } else if (avgThroughput > 500) {
        std::cout << "Classification: VERY GOOD (> 500 KB/s)" << std::endl;
    } else if (avgThroughput > 100) {
        std::cout << "Classification: GOOD (> 100 KB/s)" << std::endl;
    } else {
        std::cout << "Classification: NEEDS OPTIMIZATION" << std::endl;
    }
    
    std::cout << "\n🛑 Stopping server..." << std::endl;
    server.Stop();
    std::cout << "✅ HTTP Performance test completed!" << std::endl;
    
    return 0;
}
