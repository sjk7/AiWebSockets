#include "WebSocket/WebSocketServerLite.h"
#include <iostream>
#include <thread>
#include <chrono>

using namespace WebSocket;

int main() {
    std::cout << "🚀 WebSocket Server - Simple & Secure" << std::endl;
    std::cout << "====================================" << std::endl;
    
    try {
        // Create server with built-in security
        WebSocketServerLite server;
        
        // Configure (optional - all have sensible defaults)
        server.SetPort(8080)
               .EnableSecurity(true)  // User-Agent filtering, rate limiting, etc.
               .SetMaxConnections(50)
               .SetMaxConnectionsPerIP(5);
        
        // Set up event handlers
        server.OnConnect([](const std::string& clientIP) {
            std::cout << "🔗 Client connected: " << clientIP << std::endl;
        });
        
        server.OnMessage([](const std::string& message) {
            std::cout << "📨 Received: " << message << std::endl;
            
            // Simple echo response
            std::cout << "📤 Echoing: " << message << std::endl;
            // In a real server, you would process the message and send responses
        });
        
        server.OnDisconnect([](const std::string& clientIP) {
            std::cout << "🔌 Client disconnected: " << clientIP << std::endl;
        });
        
        server.OnError([](const Result& error) {
            std::cout << "❌ Server error: " << error.GetErrorMessage() << std::endl;
        });
        
        // Start server (non-blocking)
        auto startResult = server.Start();
        if (!startResult.IsSuccess()) {
            std::cout << "❌ Failed to start server: " << startResult.GetErrorMessage() << std::endl;
            return 1;
        }
        
        std::cout << "✅ Server started successfully!" << std::endl;
        std::cout << "🔒 Security: ENABLED (User-Agent filtering, rate limiting)" << std::endl;
        std::cout << "📊 Listening on port 8080" << std::endl;
        std::cout << "🔄 Processing events... (Press Ctrl+C to stop)" << std::endl;
        
        // Main event loop (non-blocking)
        int statusCounter = 0;
        while (server.IsRunning()) {
            // Process WebSocket events
            server.ProcessEvents();
            
            // Show status periodically
            if (++statusCounter % 1000 == 0) {  // Every ~10 seconds
                int connections = server.GetCurrentConnectionCount();
                std::cout << "📊 Status: " << connections << " active connections" << std::endl;
            }
            
            // Small delay to prevent 100% CPU usage
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        
    } catch (const std::exception& e) {
        std::cout << "❌ Exception: " << e.what() << std::endl;
        return 1;
    }
    
    std::cout << "🛑 Server shutdown complete" << std::endl;
    return 0;
}
