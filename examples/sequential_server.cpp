#include "WebSocket/WebSocketServerLite.h"
#include <iostream>
#include <thread>
#include <chrono>

using namespace WebSocket;

int main() {
    std::cout << "🔄 Sequential WebSocket Server" << std::endl;
    std::cout << "===============================" << std::endl;
    
    try {
        // Create server with sequential (simple) handling
        WebSocketServerLite server(8080, "127.0.0.1");
        
        // Simple configuration
        server.EnableSecurity(true)
               .SetMaxConnections(10);  // Lower limit for sequential processing
        
        // Track connection count
        int connectionCount = 0;
        
        server.OnConnect([&connectionCount](const std::string& clientIP) {
            connectionCount++;
            std::cout << "🔗 Connection #" << connectionCount << " from: " << clientIP << std::endl;
        });
        
        server.OnMessage([&connectionCount](const std::string& message) {
            std::cout << "📨 [Conn #" << connectionCount << "] Received: " << message << std::endl;
            std::cout << "📤 [Conn #" << connectionCount << "] Echo: " << message << std::endl;
        });
        
        server.OnDisconnect([&connectionCount](const std::string& clientIP) {
            std::cout << "🔌 Disconnected: " << clientIP << " (was connection #" << connectionCount << ")" << std::endl;
        });
        
        server.OnError([](const Result& error) {
            std::cout << "❌ Error: " << error.GetErrorMessage() << std::endl;
        });
        
        // Start server
        auto startResult = server.Start();
        if (!startResult.IsSuccess()) {
            std::cout << "❌ Failed to start: " << startResult.GetErrorMessage() << std::endl;
            return 1;
        }
        
        std::cout << "✅ Sequential server started on port 8080" << std::endl;
        std::cout << "🔄 Processing connections sequentially..." << std::endl;
        
        // Simple sequential processing loop
        while (server.IsRunning()) {
            server.ProcessEvents();
            
            // Show simple status
            static int counter = 0;
            if (++counter % 2000 == 0) {  // Every ~20 seconds
                std::cout << "📊 Processed " << connectionCount << " connections so far" << std::endl;
            }
            
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        
    } catch (const std::exception& e) {
        std::cout << "❌ Exception: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
