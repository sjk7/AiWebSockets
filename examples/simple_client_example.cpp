#include "WebSocket/WebSocketClientLite.h"
#include <iostream>
#include <thread>
#include <chrono>

using namespace WebSocket;

int main() {
    std::cout << "🚀 Simple WebSocket Client Example" << std::endl;
    std::cout << "==================================" << std::endl;
    
    try {
        // Create client
        WebSocketClientLite client("127.0.0.1", 8080);
        
        // Set up event handlers
        client.OnConnect([]() {
            std::cout << "✅ Connected to server!" << std::endl;
        });
        
        client.OnMessage([](const std::string& message) {
            std::cout << "📨 Received: " << message << std::endl;
        });
        
        client.OnDisconnect([]() {
            std::cout << "🔌 Disconnected from server" << std::endl;
        });
        
        client.OnError([](const Result& error) {
            std::cout << "❌ Error: " << error.GetErrorMessage() << std::endl;
        });
        
        // Connect to server
        std::cout << "🔗 Connecting to server..." << std::endl;
        auto connectResult = client.Connect();
        if (!connectResult.IsSuccess()) {
            std::cout << "❌ Failed to connect: " << connectResult.GetErrorMessage() << std::endl;
            return 1;
        }
        
        // Send some messages
        std::cout << "📤 Sending messages..." << std::endl;
        
        client.SendMessage("Hello, Server!");
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        client.SendMessage("This is a test message");
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        client.SendMessage("WebSocket is working!");
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        // Receive messages for a bit
        std::cout << "📨 Listening for messages (5 seconds)..." << std::endl;
        for (int i = 0; i < 50 && client.IsConnected(); ++i) {
            client.ProcessMessages();
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        
        // Disconnect
        std::cout << "🔌 Disconnecting..." << std::endl;
        client.Disconnect();
        
    } catch (const std::exception& e) {
        std::cout << "❌ Exception: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
