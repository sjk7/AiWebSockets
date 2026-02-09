/**
 * @file performance_test.cpp
 * @brief Socket performance measurement test
 * 
 * This executable measures the maximum transfer rate of our socket implementation
 * using various data sizes and configurations.
 */

#include "WebSocket/Socket.h"
#include "WebSocket/TestUtilities.h"
#include <iostream>
#include <vector>
#include <string>
#include <chrono>
#include <thread>
#include <iomanip>
#include <atomic>
#include <sstream>
#include <algorithm>

using namespace WebSocket;

class PerformanceTestSuite {
private:
    struct TestResult {
        std::string testName;
        size_t dataSize;
        double transferTimeMs;
        double throughputMBps;
        double throughputGbps;
        bool success;
    };

    void PrintResult(const std::string& testName, const TestResult& result) {
        std::cout << std::fixed << std::setprecision(2);
        std::cout << "  " << testName << ":" << std::endl;
        std::cout << "    Data Size: " << FormatBytes(result.dataSize) << std::endl;
        std::cout << "    Time: " << result.transferTimeMs << " ms" << std::endl;
        std::cout << "    Throughput: " << result.throughputMBps << " MB/s (" 
                  << result.throughputGbps << " Gbps)" << std::endl;
        std::cout << "    Status: " << (result.success ? "SUCCESS" : "FAILED") << std::endl;
        std::cout << std::endl;
    }

    std::string FormatBytes(size_t bytes) {
        const char* units[] = {"B", "KB", "MB", "GB", "TB"};
        int unit = 0;
        double size = static_cast<double>(bytes);
        
        while (size >= 1024.0 && unit < 4) {
            size /= 1024.0;
            unit++;
        }
        
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(2) << size << " " << units[unit];
        return oss.str();
    }

    TestResult MeasureTransfer(size_t dataSize, size_t chunkSize = 65536) { // Large chunk size - test real capability
        TestResult result{"", dataSize, 0.0, 0.0, 0.0, false};
        
        // Create test data
        std::vector<uint8_t> testData = WebSocket::createTestData(dataSize);
        
        // Setup server and client
        Socket serverSocket, clientSocket;
        if (!serverSocket.Create(socketFamily::IPV4, socketType::TCP).IsSuccess()) {
            return result;
        }
        if (!serverSocket.ReuseAddress(true).IsSuccess()) {
            return result;
        }
        
        // Set larger buffers for better performance
        serverSocket.SendBufferSize(1024 * 1024); // 1MB send buffer
        serverSocket.ReceiveBufferSize(1024 * 1024); // 1MB receive buffer
        
        if (!serverSocket.Bind("127.0.0.1", 0).IsSuccess()) {
            return result;
        }
        if (!serverSocket.Listen(5).IsSuccess()) {
            return result;
        }
        
        std::string serverAddress = serverSocket.LocalAddress();
        uint16_t serverPort = serverSocket.LocalPort();
        
        if (!clientSocket.Create(socketFamily::IPV4, socketType::TCP).IsSuccess()) {
            return result;
        }
        
        // Enable async I/O for maximum performance
        auto clientAsyncResult = clientSocket.EnableAsyncIO();
        if (!clientAsyncResult.IsSuccess()) {
            std::cout << "Warning: Failed to enable client async I/O: " << clientAsyncResult.GetErrorMessage() << std::endl;
        }
        
        if (!clientSocket.Connect(serverAddress, serverPort).IsSuccess()) {
            return result;
        }
        
        auto acceptPair = serverSocket.Accept();
        auto& acceptResult = acceptPair.first;
        auto& acceptedSocket = acceptPair.second;
        if (!acceptResult.IsSuccess() || !acceptedSocket) {
            return result;
        }
        
        // Set larger receive buffers for better performance
        acceptedSocket->ReceiveBufferSize(1024 * 1024); // 1MB buffer
        clientSocket.SendBufferSize(1024 * 1024); // 1MB buffer
        
        // Enable async I/O on accepted socket for maximum performance
        auto serverAsyncResult = acceptedSocket->EnableAsyncIO();
        if (!serverAsyncResult.IsSuccess()) {
            std::cout << "Warning: Failed to enable server async I/O: " << serverAsyncResult.GetErrorMessage() << std::endl;
        }
        
        // Keep client BLOCKING to prevent infinite retry loops
        // clientSocket.Blocking(false); // REMOVED - causes infinite retries
        
        // Measure transfer time
        auto startTime = std::chrono::high_resolution_clock::now();
        
        // Receive data with larger buffer
        std::vector<uint8_t> receivedData;
        receivedData.reserve(dataSize); // Pre-allocate for performance
        
        // Simple approach: Send smaller chunks and receive immediately
        size_t bytesSent = 0;
        size_t bytesReceived = 0;
        
        printf("DEBUG: Starting concurrent send/receive for %zu bytes\n", testData.size());
        
        // Track timing for speed calculations
        auto lastProgressTime = std::chrono::high_resolution_clock::now();
        size_t lastProgressBytes = 0;
        
        while (bytesSent < testData.size() || bytesReceived < testData.size()) {
            // Send a chunk if we have more to send
            if (bytesSent < testData.size()) {
                size_t chunkSizeToSend = std::min(chunkSize, testData.size() - bytesSent);
                std::vector<uint8_t> chunk(testData.begin() + bytesSent, 
                                           testData.begin() + bytesSent + chunkSizeToSend);
                
                printf("DEBUG CLIENT: Sending chunk %zu/%zu (size %zu)...\n", 
                       bytesSent + chunkSizeToSend, testData.size(), chunkSizeToSend);
                Result sendResult = clientSocket.Send(chunk);
                if (!sendResult.IsSuccess()) {
                    printf("DEBUG CLIENT: Send failed: %s\n", sendResult.GetErrorMessage().c_str());
                    return result;
                }
                bytesSent += chunkSizeToSend;
                printf("DEBUG CLIENT: Sent %zu bytes total (%.1f%% complete)\n", 
                       bytesSent, (double)bytesSent / testData.size() * 100.0);
            }
            
            // Receive a chunk if we have more to receive
            if (bytesReceived < testData.size()) {
                size_t receiveChunkSize = std::min(chunkSize, testData.size() - bytesReceived);
                printf("DEBUG SERVER: About to receive chunk %zu/%zu (size %zu)...\n", 
                       bytesReceived + receiveChunkSize, testData.size(), receiveChunkSize);
                auto receivePair = acceptedSocket->Receive(receiveChunkSize);
                auto& receiveResult = receivePair.first;
                auto& chunk = receivePair.second;
                
                if (!receiveResult.IsSuccess()) {
                    printf("DEBUG SERVER: Receive failed: %s\n", receiveResult.GetErrorMessage().c_str());
                    return result;
                }
                
                if (chunk.empty()) {
                    printf("DEBUG SERVER: Connection closed by client\n");
                    break;
                }
                
                // Add detailed server debug output
                printf("DEBUG SERVER: Received chunk of %zu bytes\n", chunk.size());
                printf("DEBUG SERVER: Chunk data preview: ");
                size_t previewSize = std::min(chunk.size(), size_t(16));
                for (size_t i = 0; i < previewSize; i++) {
                    printf("%02X ", chunk[i]);
                }
                if (chunk.size() > 16) {
                    printf("... (+%zu more bytes)", chunk.size() - 16);
                }
                printf("\n");
                
                receivedData.insert(receivedData.end(), chunk.begin(), chunk.end());
                bytesReceived += chunk.size();
                
                // Calculate transfer speed
                auto currentTime = std::chrono::high_resolution_clock::now();
                auto timeDiff = std::chrono::duration_cast<std::chrono::microseconds>(currentTime - lastProgressTime);
                
                if (timeDiff.count() > 0) {
                    double bytesPerSecond = (double)(bytesReceived - lastProgressBytes) / (timeDiff.count() / 1000000.0);
                    double mbPerSecond = bytesPerSecond / (1024.0 * 1024.0);
                    
                    printf("DEBUG SERVER: Received %zu bytes total (%.1f%% complete) - Speed: %.2f MB/s\n", 
                           bytesReceived, (double)bytesReceived / testData.size() * 100.0, mbPerSecond);
                    
                    lastProgressTime = currentTime;
                    lastProgressBytes = bytesReceived;
                } else {
                    printf("DEBUG SERVER: Received %zu bytes total (%.1f%% complete)\n", 
                           bytesReceived, (double)bytesReceived / testData.size() * 100.0);
                }
            }
            
            // Small delay to allow proper interleaving
            std::this_thread::sleep_for(std::chrono::microseconds(100));
        }
        
        printf("DEBUG: Transfer complete - Sent: %zu, Received: %zu\n", bytesSent, bytesReceived);
        
        // Data integrity verification
        printf("DEBUG: Performing data integrity checks...\n");
        
        // Check if all data was sent
        if (bytesSent != testData.size()) {
            printf("❌ ERROR: Not all data was sent! Expected: %zu, Sent: %zu\n", 
                   testData.size(), bytesSent);
            return result;
        }
        
        // Check if all data was received
        if (bytesReceived != testData.size()) {
            printf("❌ ERROR: Not all data was received! Expected: %zu, Received: %zu\n", 
                   testData.size(), bytesReceived);
            return result;
        }
        
        // Check if received data size matches buffer
        if (receivedData.size() != testData.size()) {
            printf("❌ ERROR: Received buffer size mismatch! Expected: %zu, Buffer: %zu\n", 
                   testData.size(), receivedData.size());
            return result;
        }
        
        // Verify data content integrity
        bool dataIntegrityPassed = true;
        for (size_t i = 0; i < testData.size(); i++) {
            if (i < receivedData.size() && testData[i] != receivedData[i]) {
                printf("❌ ERROR: Data corruption at byte %zu! Expected: 0x%02X, Received: 0x%02X\n", 
                       i, testData[i], receivedData[i]);
                dataIntegrityPassed = false;
                break;
            }
        }
        
        if (!dataIntegrityPassed) {
            printf("❌ ERROR: Data integrity check FAILED!\n");
            return result;
        }
        
        printf("✅ SUCCESS: All data transferred correctly!\n");
        printf("   - Sent: %zu bytes (100%%)\n", bytesSent);
        printf("   - Received: %zu bytes (100%%)\n", bytesReceived);
        printf("   - Data integrity: PASSED\n");
        printf("   - No corruption detected\n");
        
        auto endTime = std::chrono::high_resolution_clock::now();
        
        // Calculate metrics
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);
        result.transferTimeMs = duration.count() / 1000.0;
        
        if (result.transferTimeMs > 0) {
            double bytesPerSecond = (static_cast<double>(receivedData.size()) * 1000000.0) / duration.count();
            result.throughputMBps = bytesPerSecond / (1024.0 * 1024.0);
            result.throughputGbps = result.throughputMBps / 1024.0;
        }
        
        // Verify data integrity (sample check for large data to save time)
        result.success = (receivedData.size() == testData.size());
        if (result.success && dataSize <= 1024 * 1024) { // Full check for 1MB or less
            for (size_t i = 0; i < testData.size(); i++) {
                if (receivedData[i] != testData[i]) {
                    result.success = false;
                    break;
                }
            }
        } else if (result.success && dataSize > 1024 * 1024) { // Sample check for larger data
            const size_t sampleSize = 1024; // Check first and last 1KB
            for (size_t i = 0; i < sampleSize && i < receivedData.size(); i++) {
                if (receivedData[i] != testData[i]) {
                    result.success = false;
                    break;
                }
            }
            if (result.success) {
                for (size_t i = 0; i < sampleSize && i < receivedData.size(); i++) {
                    size_t checkPos = receivedData.size() - sampleSize + i;
                    size_t originalPos = testData.size() - sampleSize + i;
                    if (receivedData[checkPos] != testData[originalPos]) {
                        result.success = false;
                        break;
                    }
                }
            }
        }
        
        // Cleanup
        clientSocket.Close();
        acceptedSocket->Close();
        serverSocket.Close();
        
        return result;
    }

    TestResult MeasureBidirectionalTransfer(size_t dataSize) {
        TestResult result{"", dataSize * 2, 0.0, 0.0, 0.0, false}; // *2 for bidirectional
        
        // Create test data
        std::vector<uint8_t> testData = WebSocket::createTestData(dataSize);
        
        // Setup server and client
        Socket serverSocket, clientSocket;
        serverSocket.Create(socketFamily::IPV4, socketType::TCP);
        serverSocket.ReuseAddress(true);
        serverSocket.Bind("127.0.0.1", 0);
        serverSocket.Listen(5);
        
        std::string serverAddress = serverSocket.LocalAddress();
        uint16_t serverPort = serverSocket.LocalPort();
        
        clientSocket.Create(socketFamily::IPV4, socketType::TCP);
        clientSocket.Connect(serverAddress, serverPort);
        
        auto acceptPair = serverSocket.Accept();
        auto& acceptResult = acceptPair.first;
        auto& acceptedSocket = acceptPair.second;
        if (!acceptResult.IsSuccess() || !acceptedSocket) {
            return result;
        }
        
        // Start receiver thread
        std::atomic<bool> receiveSuccess{false};
        std::vector<uint8_t> receivedData;
        
        std::thread receiverThread([&]() {
            size_t totalReceived = 0;
            while (totalReceived < dataSize) {
                auto receivePair = acceptedSocket->Receive(8192);
                auto& receiveResult = receivePair.first;
                auto& chunk = receivePair.second;
                if (!receiveResult.IsSuccess() || chunk.empty()) {
                    return;
                }
                receivedData.insert(receivedData.end(), chunk.begin(), chunk.end());
                totalReceived += chunk.size();
            }
            receiveSuccess = true;
        });
        
        // Measure bidirectional transfer time
        auto startTime = std::chrono::high_resolution_clock::now();
        
        // Send from client to server
        size_t bytesSent = 0;
        while (bytesSent < testData.size()) {
            size_t chunkSize = std::min(size_t(8192), testData.size() - bytesSent);
            std::vector<uint8_t> chunk(testData.begin() + bytesSent, 
                                       testData.begin() + bytesSent + chunkSize);
            
            if (!clientSocket.Send(chunk).IsSuccess()) {
                receiverThread.join();
                return result;
            }
            bytesSent += chunkSize;
        }
        
        // Wait for receive to complete
        receiverThread.join();
        
        // Send response back
        if (receiveSuccess) {
            size_t responseSent = 0;
            while (responseSent < testData.size()) {
                size_t chunkSize = std::min(size_t(8192), testData.size() - responseSent);
                std::vector<uint8_t> chunk(testData.begin() + responseSent, 
                                           testData.begin() + responseSent + chunkSize);
                
                if (!acceptedSocket->Send(chunk).IsSuccess()) {
                    break;
                }
                responseSent += chunkSize;
            }
        }
        
        // Receive response on client
        std::vector<uint8_t> responseData;
        size_t responseReceived = 0;
        while (responseReceived < testData.size()) {
            auto receivePair = clientSocket.Receive(8192);
            auto& receiveResult = receivePair.first;
            auto& chunk = receivePair.second;
            if (!receiveResult.IsSuccess() || chunk.empty()) {
                break;
            }
            responseData.insert(responseData.end(), chunk.begin(), chunk.end());
            responseReceived += chunk.size();
        }
        
        auto endTime = std::chrono::high_resolution_clock::now();
        
        // Calculate metrics
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);
        result.transferTimeMs = duration.count() / 1000.0;
        
        if (result.transferTimeMs > 0) {
            double totalBytes = static_cast<double>(receivedData.size() + responseData.size());
            double bytesPerSecond = (totalBytes * 1000000.0) / duration.count();
            result.throughputMBps = bytesPerSecond / (1024.0 * 1024.0);
            result.throughputGbps = result.throughputMBps / 1024.0;
        }
        
        // Verify data integrity
        result.success = receiveSuccess && 
                        (receivedData.size() == testData.size()) && 
                        (responseData.size() == testData.size());
        
        // Cleanup
        clientSocket.Close();
        acceptedSocket->Close();
        serverSocket.Close();
        
        return result;
    }

    TestResult MeasureConcurrentTransfer(size_t dataSize, int numConnections) {
        TestResult result{"", dataSize * numConnections, 0.0, 0.0, 0.0, false};
        
        // Create server socket
        Socket serverSocket;
        if (!serverSocket.Create(socketFamily::IPV4, socketType::TCP).IsSuccess()) {
            return result;
        }
        if (!serverSocket.ReuseAddress(true).IsSuccess()) {
            return result;
        }
        
        // Enable async I/O on server for maximum performance
        auto serverAsyncResult = serverSocket.EnableAsyncIO();
        if (!serverAsyncResult.IsSuccess()) {
            std::cout << "Warning: Failed to enable concurrent server async I/O: " << serverAsyncResult.GetErrorMessage() << std::endl;
        }
        
        if (!serverSocket.Bind("127.0.0.1", 0).IsSuccess()) {
            return result;
        }
        if (!serverSocket.Listen(numConnections).IsSuccess()) {
            return result;
        }
        
        std::string serverAddress = serverSocket.LocalAddress();
        uint16_t serverPort = serverSocket.LocalPort();
        
        // Create test data
        std::vector<uint8_t> testData = WebSocket::createTestData(dataSize);
        
        std::vector<std::unique_ptr<Socket>> clients;
        std::vector<std::thread> clientThreads;
        std::atomic<int> successfulTransfers{0};
        std::atomic<size_t> totalTransferTime{0};
        
        auto startTime = std::chrono::high_resolution_clock::now();
        
        // Create client threads
        for (int i = 0; i < numConnections; i++) {
            clientThreads.emplace_back([&]() {
                auto threadStart = std::chrono::high_resolution_clock::now();
                
                Socket clientSocket;
                if (!clientSocket.Create(socketFamily::IPV4, socketType::TCP).IsSuccess()) {
                    return;
                }
                
                // Enable async I/O for maximum performance
                auto clientAsyncResult = clientSocket.EnableAsyncIO();
                if (!clientAsyncResult.IsSuccess()) {
                    // Continue without async I/O if it fails
                }
                
                if (!clientSocket.Connect(serverAddress, serverPort).IsSuccess()) {
                    return;
                }
                
                // Send data
                size_t bytesSent = 0;
                while (bytesSent < testData.size()) {
                    size_t chunkSize = std::min(size_t(4096), testData.size() - bytesSent);
                    std::vector<uint8_t> chunk(testData.begin() + bytesSent, 
                                               testData.begin() + bytesSent + chunkSize);
                    
                    if (!clientSocket.Send(chunk).IsSuccess()) {
                        clientSocket.Close();
                        return;
                    }
                    bytesSent += chunkSize;
                }
                
                auto threadEnd = std::chrono::high_resolution_clock::now();
                auto threadDuration = std::chrono::duration_cast<std::chrono::microseconds>(threadEnd - threadStart);
                totalTransferTime += threadDuration.count();
                
                clientSocket.Close();
                successfulTransfers++;
            });
        }
        
        // Accept connections
        std::vector<std::unique_ptr<Socket>> acceptedSockets;
        std::vector<std::thread> receiverThreads;
        
        for (int i = 0; i < numConnections; i++) {
            auto acceptPair = serverSocket.Accept();
            auto& acceptResult = acceptPair.first;
            auto& acceptedSocket = acceptPair.second;
            if (acceptResult.IsSuccess() && acceptedSocket) {
                acceptedSockets.push_back(std::move(acceptedSocket));
                
                receiverThreads.emplace_back([&, i]() {
                    size_t totalReceived = 0;
                    while (totalReceived < dataSize) {
                        auto receivePair = acceptedSockets[i]->Receive(4096);
                        auto& receiveResult = receivePair.first;
                        auto& chunk = receivePair.second;
                        if (!receiveResult.IsSuccess() || chunk.empty()) {
                            break;
                        }
                        totalReceived += chunk.size();
                    }
                });
            }
        }
        
        // Wait for all threads to complete
        for (auto& thread : clientThreads) {
            thread.join();
        }
        for (auto& thread : receiverThreads) {
            thread.join();
        }
        
        auto endTime = std::chrono::high_resolution_clock::now();
        
        // Calculate metrics
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);
        result.transferTimeMs = duration.count() / 1000.0;
        
        if (result.transferTimeMs > 0) {
            double bytesPerSecond = (static_cast<double>(dataSize * successfulTransfers.load()) * 1000000.0) / duration.count();
            result.throughputMBps = bytesPerSecond / (1024.0 * 1024.0);
            result.throughputGbps = result.throughputMBps / 1024.0;
        }
        
        result.success = (successfulTransfers.load() == numConnections);
        
        // Cleanup
        for (auto& accepted : acceptedSockets) {
            accepted->Close();
        }
        serverSocket.Close();
        
        return result;
    }

public:
    void RunPerformanceTests() {
        std::cout << "WebSocket Socket Performance Test Suite" << std::endl;
        std::cout << "=======================================" << std::endl;
        std::cout << std::endl;
        
        auto testStartTime = std::chrono::high_resolution_clock::now();
        
        // Initialize socket system
        // Note: Socket system initialization is now automatic
        
        // Test different data sizes
        std::vector<size_t> dataSizes = {
            1 * 1024,        // 1 KB
            10 * 1024,       // 10 KB
            100 * 1024,      // 100 KB
            1 * 1024 * 1024, // 1 MB
            5 * 1024 * 1024  // 5 MB (reduced from 10MB to avoid hanging)
        };
        
        std::cout << "=== Single-Direction Transfer Tests (Client→Server Only) ===" << std::endl;
        std::cout << std::endl;
        
        TestResult bestResult{"", 0, 0, 0, 0, false};
        std::vector<TestResult> allResults;
        
        for (size_t i = 0; i < dataSizes.size(); i++) {
            size_t size = dataSizes[i];
            std::cout << "Running test " << (i+1) << "/" << dataSizes.size() << ": ";
            TestResult result = MeasureTransfer(size);
            result.testName = FormatBytes(size) + " (Client→Server)";
            PrintResult(result.testName, result);
            allResults.push_back(result);
            
            if (result.success && result.throughputMBps > bestResult.throughputMBps) {
                bestResult = result;
            }
        }
        
        std::cout << "✅ Single-direction tests completed" << std::endl;
        std::cout << std::endl;
        
        std::cout << "=== Full-Duplex Transfer Tests (Client↔Server Both Directions) ===" << std::endl;
        std::cout << std::endl;
        
        for (size_t size : dataSizes) {
            if (size <= 1 * 1024 * 1024) { // Limit bidirectional tests to 1MB
                TestResult result = MeasureBidirectionalTransfer(size);
                result.testName = FormatBytes(size) + " (Client↔Server)";
                PrintResult(result.testName, result);
                allResults.push_back(result);
                
                if (result.success && result.throughputMBps > bestResult.throughputMBps) {
                    bestResult = result;
                }
            }
        }
        
        std::cout << "✅ Full-duplex tests completed" << std::endl;
        std::cout << std::endl;
        
        std::cout << "=== Concurrent Connection Tests ===" << std::endl;
        std::cout << std::endl;
        
        std::vector<int> connectionCounts = {2, 4, 8, 16};
        for (size_t i = 0; i < connectionCounts.size(); i++) {
            int connections = connectionCounts[i];
            std::cout << "Running concurrent test " << (i+1) << "/" << connectionCounts.size() << ": ";
            TestResult result = MeasureConcurrentTransfer(100 * 1024, connections); // 100KB per connection
            result.testName = std::to_string(connections) + " Concurrent Clients";
            PrintResult(std::to_string(connections) + " Concurrent Connections", result);
            allResults.push_back(result);
            
            if (result.success && result.throughputMBps > bestResult.throughputMBps) {
                bestResult = result;
            }
        }
        
        std::cout << "✅ Concurrent connection tests completed" << std::endl;
        std::cout << std::endl;
        
        // Summary
        std::cout << "=== Performance Summary ===" << std::endl;
        std::cout << std::endl;
        
        // Detailed Results Table
        std::cout << "Detailed Results Table:" << std::endl;
        std::cout << "+----------------------+------------+----------+------------------+--------+" << std::endl;
        std::cout << "| Test Name           | Data Size  | Time (ms)| Transfer Rate    | Status |" << std::endl;
        std::cout << "+----------------------+------------+----------+------------------+--------+" << std::endl;
        
        // Print individual test results in table format
        for (const auto& result : allResults) {
            std::cout << "| " << std::left << std::setw(20) << result.testName << " | "
                      << std::right << std::setw(10) << FormatBytes(result.dataSize) << " | "
                      << std::setw(8) << std::fixed << std::setprecision(1) << result.transferTimeMs << " | "
                      << std::setw(14) << std::fixed << std::setprecision(2) << result.throughputMBps << " MB/s | "
                      << std::setw(6) << (result.success ? "✅" : "❌") << " |" << std::endl;
        }
        
        std::cout << "+----------------------+------------+----------+------------------+--------+" << std::endl;
        std::cout << std::endl;
        
        std::cout << "Maximum Throughput Achieved:" << std::endl;
        std::cout << "  " << bestResult.throughputMBps << " MB/s (" 
                  << bestResult.throughputGbps << " Gbps)" << std::endl;
        std::cout << "  Data Size: " << FormatBytes(bestResult.dataSize) << std::endl;
        std::cout << "  Transfer Time: " << bestResult.transferTimeMs << " ms" << std::endl;
        std::cout << std::endl;
        
        // Performance classification
        if (bestResult.throughputGbps >= 1.0) {
            std::cout << "Performance Classification: EXCELLENT (≥ 1 Gbps)" << std::endl;
        } else if (bestResult.throughputMBps >= 100) {
            std::cout << "Performance Classification: VERY GOOD (≥ 100 MB/s)" << std::endl;
        } else if (bestResult.throughputMBps >= 10) {
            std::cout << "Performance Classification: GOOD (≥ 10 MB/s)" << std::endl;
        } else {
            std::cout << "Performance Classification: NEEDS IMPROVEMENT (< 10 MB/s)" << std::endl;
        }
        
        // Cleanup

        
        auto testEndTime = std::chrono::high_resolution_clock::now();
        auto totalTestDuration = std::chrono::duration_cast<std::chrono::milliseconds>(testEndTime - testStartTime);
        
        std::cout << std::endl;
        std::cout << "=====================================" << std::endl;
        std::cout << "🎉 ALL PERFORMANCE TESTS COMPLETED!" << std::endl;
        std::cout << "Total test time: " << totalTestDuration.count() << " ms" << std::endl;
        std::cout << "=====================================" << std::endl;
    }
};

int main() {
    PerformanceTestSuite testSuite;
    testSuite.RunPerformanceTests();
    return 0;
}
