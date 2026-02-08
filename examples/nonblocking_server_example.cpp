#include "WebSocket/WebSocketServerLite.h"
#include <iostream>
#include <thread>
#include <chrono>

using namespace WebSocket;

int main() {
    std::cout << "🚀 Non-blocking WebSocket Server Example" << std::endl;
    std::cout << "=======================================" << std::endl;
    
    try {
        // Create server
        WebSocketServerLite server(8080, "127.0.0.1");
        
        // Configure with security enabled
        server.EnableSecurity(true)
               .SetMaxConnections(100);
        
        // Set up event handlers
        server.OnConnect([](const std::string& clientIP) {
            std::cout << "🔗 " << clientIP << " connected" << std::endl;
        });
        
        server.OnMessage([](const std::string& message) {
            std::cout << "📨 Got message: " << message << std::endl;
        });
        
        server.OnDisconnect([](const std::string& clientIP) {
            std::cout << "🔌 " << clientIP << " disconnected" << std::endl;
        });
        
        server.OnError([](const Result& error) {
            std::cout << "❌ Error: " << error.GetErrorMessage() << std::endl;
        });
        
        // Start non-blocking server
        auto startResult = server.StartNonBlocking();
        if (!startResult.IsSuccess()) {
            std::cout << "❌ Failed to start server: " << startResult.GetErrorMessage() << std::endl;
            return 1;
        }
        
        std::cout << "✅ Server started in non-blocking mode" << std::endl;
        std::cout << "📊 Current connections: " << server.GetCurrentConnectionCount() << std::endl;
        std::cout << "🔄 Processing events... (Press Ctrl+C to stop)" << std::endl;
        
        // Main application loop
        while (server.IsRunning()) {
            // Process server events
            server.ProcessEvents();
            
            // Your application logic here
            static int counter = 0;
            if (++counter % 1000 == 0) {  // Every ~10 seconds
                std::cout << "📊 Status: " << server.GetCurrentConnectionCount() << " connections" << std::endl;
            }
            
            // Small delay to prevent 100% CPU usage
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        
    } catch (const std::exception& e) {
        std::cout << "❌ Exception: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
