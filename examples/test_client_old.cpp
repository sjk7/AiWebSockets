#include <iostream>
#include <thread>
#include <chrono>
#include "WebSocket/Socket.h"

using namespace WebSocket;

int main(int argc, char* argv[]) {
    std::cout << "Test Client for Sequential Server" << std::endl;
    std::cout << "==================================" << std::endl;
    
    std::string message = "Hello from client!";
    if (argc > 1) {
        message = argv[1];
    }
    
    // Create client socket
    Socket clientSocket;
    if (!clientSocket.Create(SOCKET_FAMILY::IPV4, SOCKET_TYPE::TCP).IsSuccess()) {
        std::cout << "❌ Failed to create client socket" << std::endl;
        return 1;
    }
    
    // Connect to server
    std::cout << "🔗 Connecting to 127.0.0.1:8080..." << std::endl;
    if (!clientSocket.Connect("127.0.0.1", 8080).IsSuccess()) {
        std::cout << "❌ Failed to connect to server" << std::endl;
        return 1;
    }
    
    std::cout << "✅ Connected to server!" << std::endl;
    
    // Send message to server
    std::cout << "📤 Sending: \"" << message << "\"" << std::endl;
    auto sendResult = clientSocket.Send(std::vector<uint8_t>(message.begin(), message.end()));
    
    if (!sendResult.IsSuccess()) {
        std::cout << "❌ Failed to send message: " << sendResult.ErrorMessage << std::endl;
        clientSocket.Close();
        return 1;
    }
    
    std::cout << "✅ Message sent!" << std::endl;
    
    // Receive response from server
    std::cout << "📨 Waiting for response..." << std::endl;
    ReceiveResult receiveResult = clientSocket.Receive(4096);
    
    if (receiveResult.first.IsSuccess() && !receiveResult.second.empty()) {
        std::string response(receiveResult.second.begin(), receiveResult.second.end());
        std::cout << "📄 Received: \"" << response << "\"" << std::endl;
    } else {
        std::cout << "❌ Failed to receive response or no data received" << std::endl;
    }
    
    // Close connection
    clientSocket.Close();
    std::cout << "🔌 Disconnected from server" << std::endl;
    
    return 0;
}
