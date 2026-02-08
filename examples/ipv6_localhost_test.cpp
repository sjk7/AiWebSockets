#include "WebSocket/WebSocketServerLite.h"
#include "WebSocket/Socket.h"
#include <iostream>
#include <thread>
#include <chrono>

using namespace WebSocket;

int main() {
    std::cout << "🚀 IPv6 Localhost Server Test" << std::endl;
    std::cout << "=============================" << std::endl;
    
    // Test IPv6 localhost server
    std::cout << "\n🔍 Testing IPv6 Localhost Server..." << std::endl;
    
    WebSocketServerLite ipv6Server;
    ipv6Server.SetPort(8080)
             .SetBindAddress("::1")  // IPv6 localhost
             .EnableSecurity(true)
             .SetMaxConnections(5);
    
    // Set up event handlers
    ipv6Server.OnConnect([](const std::string& clientIP) {
        std::cout << "🔗 Client connected: " << clientIP << std::endl;
    });
    
    ipv6Server.OnMessage([](const std::string& message) {
        std::cout << "📨 Received: " << message << std::endl;
    });
    
    ipv6Server.OnDisconnect([](const std::string& clientIP) {
        std::cout << "🔌 Client disconnected: " << clientIP << std::endl;
    });
    
    ipv6Server.OnError([](const Result& error) {
        std::cout << "❌ Error: " << error.GetErrorMessage() << std::endl;
    });
    
    // Start the server
    auto startResult = ipv6Server.Start();
    if (startResult.IsSuccess()) {
        std::cout << "✅ IPv6 localhost server started!" << std::endl;
        std::cout << "   Connect to: ws://[::1]:8080" << std::endl;
        std::cout << "   Or: ws://localhost:8080" << std::endl;
        
        // Run for 5 seconds
        std::cout << "🔄 Running for 5 seconds..." << std::endl;
        for (int i = 0; i < 50 && ipv6Server.IsRunning(); ++i) {
            ipv6Server.ProcessEvents();
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            
            if (i % 10 == 0) {
                std::cout << "   Server running... (" << (50-i)/10 << "s remaining)" << std::endl;
            }
        }
        
        ipv6Server.Stop();
        std::cout << "✅ IPv6 localhost server stopped" << std::endl;
    } else {
        std::cout << "❌ Failed to start IPv6 localhost server: " << startResult.GetErrorMessage() << std::endl;
    }
    
    // Test IPv4 localhost for comparison
    std::cout << "\n🔍 Testing IPv4 Localhost Server..." << std::endl;
    
    WebSocketServerLite ipv4Server;
    ipv4Server.SetPort(8081)
             .SetBindAddress("127.0.0.1")  // IPv4 localhost
             .EnableSecurity(true)
             .SetMaxConnections(5);
    
    ipv4Server.OnError([](const Result& error) {
        std::cout << "❌ IPv4 Error: " << error.GetErrorMessage() << std::endl;
    });
    
    auto ipv4StartResult = ipv4Server.Start();
    if (ipv4StartResult.IsSuccess()) {
        std::cout << "✅ IPv4 localhost server started!" << std::endl;
        std::cout << "   Connect to: ws://127.0.0.1:8081" << std::endl;
        
        // Run for 3 seconds
        for (int i = 0; i < 30 && ipv4Server.IsRunning(); ++i) {
            ipv4Server.ProcessEvents();
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        
        ipv4Server.Stop();
        std::cout << "✅ IPv4 localhost server stopped" << std::endl;
    } else {
        std::cout << "❌ Failed to start IPv4 localhost server: " << ipv4StartResult.GetErrorMessage() << std::endl;
    }
    
    std::cout << "\n✅ IPv6 Localhost Test Complete!" << std::endl;
    std::cout << "\n📋 Connection Instructions:" << std::endl;
    std::cout << "IPv6 Server: ws://[::1]:8080" << std::endl;
    std::cout << "IPv4 Server: ws://127.0.0.1:8081" << std::endl;
    std::cout << "Or use: ws://localhost:8080 (should resolve to IPv6 if available)" << std::endl;
    
    return 0;
}
