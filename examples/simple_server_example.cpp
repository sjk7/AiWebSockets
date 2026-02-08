#include "WebSocket/WebSocketServerLite.h"
#include <iostream>
#include <thread>
#include <chrono>

using namespace WebSocket;

int main() {
    std::cout << "🚀 Simple WebSocket Server Example (Non-Blocking)" << std::endl;
    std::cout << "===================================================" << std::endl;
    
    try {
        // Create server with default settings (port 8080, security enabled)
        WebSocketServerLite server;
        
        // Configure server (optional - all have sensible defaults)
        server.SetPort(8080)
               .EnableSecurity(true)  // User-Agent filtering, rate limiting, etc.
               .SetMaxConnections(50)
               .SetMaxConnectionsPerIP(5);
        
        // Set up event handlers
        server.OnConnect([](const std::string& clientIP) {
            std::cout << "🔗 New connection from: " << clientIP << std::endl;
        });
        
        server.OnMessage([](const std::string& message) {
            std::cout << "📨 Received: " << message << std::endl;
            
            // Echo the message back
            std::cout << "📤 Echoing: " << message << std::endl;
        });
        
        server.OnDisconnect([](const std::string& clientIP) {
            std::cout << "🔌 Client disconnected: " << clientIP << std::endl;
        });
        
        server.OnError([](const Result& error) {
            std::cout << "❌ Server error: " << error.GetErrorMessage() << std::endl;
        });
        
        // Start the server (non-blocking)
        std::cout << "🎯 Starting server..." << std::endl;
        auto startResult = server.StartNonBlocking();
        if (!startResult.IsSuccess()) {
            std::cout << "❌ Failed to start server: " << startResult.GetErrorMessage() << std::endl;
            return 1;
        }
        
        std::cout << "✅ Server started in non-blocking mode" << std::endl;
        std::cout << "🔄 Processing events... (Press Ctrl+C to stop)" << std::endl;
        
        // Main application loop (non-blocking)
        int statusCounter = 0;
        while (server.IsRunning()) {
            // Process server events (non-blocking)
            server.ProcessEvents();
            
            // Your application logic can go here
            if (++statusCounter % 1000 == 0) {  // Every ~10 seconds
                std::cout << "📊 Status: " << server.GetCurrentConnectionCount() << " active connections" << std::endl;
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
