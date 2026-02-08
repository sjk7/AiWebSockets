#include "WebSocket/WebSocketClientLite.h"
#include <iostream>
#include <thread>
#include <chrono>

using namespace WebSocket;

int main() {
    std::cout << "🚀 WebSocket Client Example" << std::endl;
    std::cout << "===========================" << std::endl;
    
    try {
        // Create WebSocket client
        WebSocketClientLite client("127.0.0.1", 8080);
        
        // Set up event handlers
        client.OnConnect([]() {
            std::cout << "✅ Connected to WebSocket server!" << std::endl;
        });
        
        client.OnMessage([](const std::string& message) {
            std::cout << "📨 Server message: " << message << std::endl;
        });
        
        client.OnDisconnect([]() {
            std::cout << "🔌 Disconnected from server" << std::endl;
        });
        
        client.OnError([](const Result& error) {
            std::cout << "❌ Client error: " << error.GetErrorMessage() << std::endl;
        });
        
        // Connect to server
        std::cout << "🔗 Connecting to WebSocket server..." << std::endl;
        auto connectResult = client.Connect();
        if (!connectResult.IsSuccess()) {
            std::cout << "❌ Failed to connect: " << connectResult.GetErrorMessage() << std::endl;
            return 1;
        }
        
        // Send multiple test messages
        std::vector<std::string> messages = {
            "Hello, WebSocket Server!",
            "This is a test message",
            "WebSocket is working!",
            "Final test message"
        };
        
        for (size_t i = 0; i < messages.size(); ++i) {
            std::cout << "📤 Sending message " << (i+1) << ": " << messages[i] << std::endl;
            auto sendResult = client.SendMessage(messages[i]);
            if (!sendResult.IsSuccess()) {
                std::cout << "❌ Failed to send message: " << sendResult.GetErrorMessage() << std::endl;
            }
            
            // Wait a bit between messages
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
        
        // Listen for responses for a few more seconds
        std::cout << "📨 Listening for server responses..." << std::endl;
        for (int i = 0; i < 30 && client.IsConnected(); ++i) {
            client.ProcessMessages();
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        
        // Disconnect gracefully
        std::cout << "🔌 Disconnecting..." << std::endl;
        client.Disconnect();
        
    } catch (const std::exception& e) {
        std::cout << "❌ Exception: " << e.what() << std::endl;
        return 1;
    }
    
    std::cout << "✅ WebSocket client example complete!" << std::endl;
    return 0;
}
