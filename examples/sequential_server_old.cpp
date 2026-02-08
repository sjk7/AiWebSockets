#include <iostream>
#include <vector>
#include "WebSocket/Socket.h"

using namespace WebSocket;

int main() {
    std::cout << "Sequential Server - Accept in Loop" << std::endl;
    std::cout << "===================================" << std::endl;
    
    // Create server socket
    Socket serverSocket;
    if (!serverSocket.Create(SOCKET_FAMILY::IPV4, SOCKET_TYPE::TCP).IsSuccess()) {
        std::cout << "❌ Failed to create server socket" << std::endl;
        return 1;
    }
    
    // Configure server
    serverSocket.ReuseAddress(true);
    
    if (!serverSocket.Bind("127.0.0.1", 8080).IsSuccess()) {
        std::cout << "❌ Failed to bind server socket" << std::endl;
        return 1;
    }
    
    if (!serverSocket.Listen(10).IsSuccess()) {
        std::cout << "❌ Failed to start listening" << std::endl;
        return 1;
    }
    
    std::cout << "🚀 Server listening on 127.0.0.1:8080" << std::endl;
    std::cout << "📝 Accepting clients sequentially (single-threaded)" << std::endl;
    std::cout << "🔄 Press Ctrl+C to stop" << std::endl << std::endl;
    
    int clientCount = 0;
    
    // Main server loop - Accept clients sequentially
    while (true) {
        std::cout << "⏳ Waiting for client connection..." << std::endl;
        
        // Accept() blocks here until a client connects
        AcceptResult acceptResult = serverSocket.Accept();
        
        if (!acceptResult.first.IsSuccess()) {
            std::cout << "❌ Failed to accept client: " << acceptResult.first.ErrorMessage << std::endl;
            continue; // Continue waiting for next client
        }
        
        if (!acceptResult.second) {
            std::cout << "❌ Accepted socket is null" << std::endl;
            continue;
        }
        
        clientCount++;
        std::cout << "✅ Client " << clientCount << " connected!" << std::endl;
        
        // Handle this client completely before accepting next one
        std::cout << "📨 Receiving data from client " << clientCount << "..." << std::endl;
        
        // Receive data from client
        ReceiveResult receiveResult = acceptResult.second->Receive(4096);
        
        if (receiveResult.first.IsSuccess() && !receiveResult.second.empty()) {
            std::string receivedString(receiveResult.second.begin(), receiveResult.second.end());
            std::cout << "📄 Received: \"" << receivedString << "\" (" << receiveResult.second.size() << " bytes)" << std::endl;
            
            // Send response back to client
            std::string response = "Server received your message: " + receivedString;
            auto sendResult = acceptResult.second->Send(std::vector<uint8_t>(response.begin(), response.end()));
            
            if (sendResult.IsSuccess()) {
                std::cout << "📤 Sent response to client " << clientCount << std::endl;
            } else {
                std::cout << "❌ Failed to send response: " << sendResult.ErrorMessage << std::endl;
            }
        } else {
            std::cout << "❌ Failed to receive data or client disconnected" << std::endl;
        }
        
        // Close this client connection
        acceptResult.second->Close();
        std::cout << "🔌 Client " << clientCount << " disconnected" << std::endl;
        std::cout << "--------------------------------------------------" << std::endl;
        
        // Loop continues - wait for next client
    }
    
    // Clean up (never reached in this infinite loop example)
    serverSocket.Close();
    return 0;
}
