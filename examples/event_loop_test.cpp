#include "WebSocket/Socket.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <atomic>

using namespace WebSocket;

int main() {
    std::cout << "WebSocket Event Loop Test" << std::endl;
    std::cout << "=========================" << std::endl;
    
    // Initialize socket system
    // Note: Socket system initialization is now automatic
    
    // Create server socket
    Socket serverSocket;
    Result createResult = serverSocket.Create(SOCKET_FAMILY::IPV4, SOCKET_TYPE::TCP);
    if (!createResult.IsSuccess()) {
        std::cout << "FATAL: Could not create server socket: " << createResult.GetErrorMessage() << std::endl;

        return 1;
    }
    
    // Set reuse address
    Result reuseResult = serverSocket.ReuseAddress(true);
    if (!reuseResult.IsSuccess()) {
        std::cout << "FATAL: Could not set reuse address: " << reuseResult.GetErrorMessage() << std::endl;

        return 1;
    }
    
    // Bind server socket
    Result bindResult = serverSocket.Bind("127.0.0.1", 0); // Let OS choose port
    if (!bindResult.IsSuccess()) {
        std::cout << "FATAL: Could not bind server socket: " << bindResult.GetErrorMessage() << std::endl;

        return 1;
    }
    
    // Start listening
    Result listenResult = serverSocket.Listen(5);
    if (!listenResult.IsSuccess()) {
        std::cout << "FATAL: Could not listen on server socket: " << listenResult.GetErrorMessage() << std::endl;

        return 1;
    }
    
    std::string serverAddress = serverSocket.LocalAddress();
    uint16_t serverPort = serverSocket.LocalPort();
    
    std::cout << "Server listening on " << serverAddress << ":" << serverPort << std::endl;
    
    // Set up event loop callbacks
    std::atomic<int> connectionCount{0};
    std::atomic<int> totalBytesReceived{0};
    std::vector<std::unique_ptr<Socket>> clientSockets; // Keep track of client sockets
    
    serverSocket.AcceptCallback([&](std::unique_ptr<Socket> clientSocket) {
        if (!clientSocket) {
            std::cout << "❌ Accept callback received null socket!" << std::endl;
            return;
        }
        
        connectionCount++;
        std::cout << "🔗 New connection accepted! Total connections: " << connectionCount << std::endl;
        
        // Store the socket BEFORE setting up callbacks to ensure it stays alive
        Socket* clientSocketPtr = clientSocket.get();
        clientSockets.push_back(std::move(clientSocket));
        
        // Set up receive callback for this client using the stored pointer
        clientSocketPtr->ReceiveCallback([clientSocketPtr, &totalBytesReceived](const std::vector<uint8_t>& data) {
            totalBytesReceived += static_cast<int>(data.size());
            std::cout << "📨 Received " << data.size() << " bytes: " 
                      << std::string(data.begin(), data.end()) << std::endl;
            std::cout << "   Total bytes received: " << totalBytesReceived << std::endl;
            
            // Echo the data back
            Result echoResult = clientSocketPtr->Send(data);
            if (!echoResult.IsSuccess()) {
                std::cout << "❌ Failed to echo data: " << echoResult.GetErrorMessage() << std::endl;
            }
        });
        
        clientSocketPtr->ErrorCallback([](const Result& error) {
            std::cout << "❌ Client error: " << error.GetErrorMessage() << std::endl;
        });
        
        // Start event loop for client socket
        Result clientEventLoopResult = clientSocketPtr->StartEventLoop();
        if (!clientEventLoopResult.IsSuccess()) {
            std::cout << "❌ Failed to start client event loop: " << clientEventLoopResult.GetErrorMessage() << std::endl;
        } else {
            std::cout << "✅ Client event loop started!" << std::endl;
        }
    });
    
    serverSocket.ErrorCallback([&](const Result& error) {
        std::cout << "❌ Server error: " << error.GetErrorMessage() << std::endl;
    });
    
    // Start server event loop
    Result eventLoopResult = serverSocket.StartEventLoop();
    if (!eventLoopResult.IsSuccess()) {
        std::cout << "FATAL: Could not start event loop: " << eventLoopResult.GetErrorMessage() << std::endl;

        return 1;
    }
    
    std::cout << "✅ Event loop started successfully!" << std::endl;
    
    // Create client connections to test
    std::vector<std::unique_ptr<Socket>> clients;
    
    for (int i = 0; i < 3; i++) {
        std::cout << "\n🚀 Creating client " << (i + 1) << "..." << std::endl;
        
        auto client = std::make_unique<Socket>();
        Result clientCreateResult = client->Create(SOCKET_FAMILY::IPV4, SOCKET_TYPE::TCP);
        if (!clientCreateResult.IsSuccess()) {
            std::cout << "❌ Failed to create client socket: " << clientCreateResult.GetErrorMessage() << std::endl;
            continue;
        }
        
        Result connectResult = client->Connect(serverAddress, serverPort);
        if (!connectResult.IsSuccess()) {
            std::cout << "❌ Failed to connect: " << connectResult.GetErrorMessage() << std::endl;
            continue;
        }
        
        std::cout << "✅ Client " << (i + 1) << " connected!" << std::endl;
        
        // Send test data
        std::string testData = "Hello from client " + std::to_string(i + 1) + "!";
        Result sendResult = client->Send(std::vector<uint8_t>(testData.begin(), testData.end()));
        if (!sendResult.IsSuccess()) {
            std::cout << "❌ Failed to send data: " << sendResult.GetErrorMessage() << std::endl;
        } else {
            std::cout << "📤 Sent data from client " << (i + 1) << std::endl;
        }
        
        clients.push_back(std::move(client));
        
        // Small delay between connections
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    std::cout << "\n⏱️  Test completed - checking results..." << std::endl;
    
    // Run for a short time to see the immediate results
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    std::cout << "\n📊 Final Statistics:" << std::endl;
    std::cout << "   Total connections: " << connectionCount.load() << std::endl;
    std::cout << "   Total bytes received: " << totalBytesReceived.load() << std::endl;
    
    // Cleanup
    std::cout << "\n🧹 Cleaning up..." << std::endl;
    serverSocket.StopEventLoop();
    for (auto& client : clients) {
        client->Close();
    }
    serverSocket.Close();

    
    std::cout << "✅ Event loop test completed successfully!" << std::endl;
    return 0;
}
