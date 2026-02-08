#include "WebSocket/WebSocketClientLite.h"
#include <iostream>
#include <thread>
#include <chrono>

using namespace WebSocket;

int main(int argc, char* argv[]) {
    std::cout << "🚀 WebSocket Test Client" << std::endl;
    std::cout << "========================" << std::endl;
    
    std::string message = "Hello from WebSocket client!";
    if (argc > 1) {
        message = argv[1];
    }
    
    try {
        // Create WebSocket client
        WebSocketClientLite client("127.0.0.1", 8080);
        
        // Set up event handlers
        client.OnConnect([]() {
            std::cout << "✅ Connected to WebSocket server!" << std::endl;
        });
        
        client.OnMessage([](const std::string& msg) {
            std::cout << "📨 Received: " << msg << std::endl;
        });
        
        client.OnDisconnect([]() {
            std::cout << "🔌 Disconnected from server" << std::endl;
        });
        
        client.OnError([](const Result& error) {
            std::cout << "❌ Error: " << error.GetErrorMessage() << std::endl;
        });
        
        // Connect to server (non-blocking)
        std::cout << "🔗 Connecting to WebSocket server..." << std::endl;
        auto connectResult = client.Connect();
        if (!connectResult.IsSuccess()) {
            std::cout << "❌ Failed to connect: " << connectResult.GetErrorMessage() << std::endl;
            return 1;
        }
        
        // Send a message
        std::cout << "📤 Sending: \"" << message << "\"" << std::endl;
        auto sendResult = client.SendMessage(message);
        if (!sendResult.IsSuccess()) {
            std::cout << "❌ Failed to send message: " << sendResult.GetErrorMessage() << std::endl;
            return 1;
        }
        
        std::cout << "✅ Message sent!" << std::endl;
        
        // Listen for responses for a few seconds
        std::cout << "📨 Listening for responses (5 seconds)..." << std::endl;
        for (int i = 0; i < 50 && client.IsConnected(); ++i) {
            client.ProcessMessages();
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        
        // Disconnect
        client.Disconnect();
        
    } catch (const std::exception& e) {
        std::cout << "❌ Exception: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
