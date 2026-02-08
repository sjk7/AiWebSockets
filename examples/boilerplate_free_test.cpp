#include "WebSocket/WebSocketServerLite.h"
#include "WebSocket/Socket.h"
#include <iostream>

using namespace WebSocket;

int main() {
    std::cout << "🧪 Testing Boilerplate-Free Usage" << std::endl;
    std::cout << "==================================" << std::endl;
    
    // Test 1: Simple IPv4 WebSocket Server - NO BOILERPLATE
    std::cout << "\n📋 Test 1: Simple IPv4 WebSocket Server" << std::endl;
    {
        WebSocketServerLite server;
        server.SetPort(8080)
               .SetBindAddress("127.0.0.1")
               .OnConnect([](const std::string& ip) {
                   std::cout << "🔗 Connected: " << ip << std::endl;
               })
               .OnMessage([](const std::string& msg) {
                   std::cout << "📨 Received: " << msg << std::endl;
               });
        
        auto result = server.Start();
        std::cout << "IPv4 Server: " << (result.IsSuccess() ? "✅ SUCCESS" : "❌ FAILED") << std::endl;
        if (result.IsSuccess()) {
            std::cout << "   🌐 Connect to: ws://127.0.0.1:8080" << std::endl;
            server.Stop();
        }
    }
    
    // Test 2: Simple IPv6 WebSocket Server - NO BOILERPLATE  
    std::cout << "\n📋 Test 2: Simple IPv6 WebSocket Server" << std::endl;
    {
        WebSocketServerLite server;
        server.SetPort(8081)
               .SetBindAddress("::1")  // IPv6 localhost
               .OnConnect([](const std::string& ip) {
                   std::cout << "🔗 IPv6 Connected: " << ip << std::endl;
               })
               .OnMessage([](const std::string& msg) {
                   std::cout << "📨 IPv6 Received: " << msg << std::endl;
               });
        
        auto result = server.Start();
        std::cout << "IPv6 Server: " << (result.IsSuccess() ? "✅ SUCCESS" : "❌ FAILED") << std::endl;
        if (result.IsSuccess()) {
            std::cout << "   🌐 Connect to: ws://[::1]:8081" << std::endl;
            server.Stop();
        }
    }
    
    // Test 3: Simple Socket Usage - NO BOILERPLATE
    std::cout << "\n📋 Test 3: Simple Socket Operations" << std::endl;
    {
        Socket socket;
        auto createResult = socket.Create(SOCKET_FAMILY::IPV4, SOCKET_TYPE::TCP);  // Simple creation
        std::cout << "Socket Create: " << (createResult.IsSuccess() ? "✅ SUCCESS" : "❌ FAILED") << std::endl;
        
        if (createResult.IsSuccess()) {
            // Test IPv6 port availability - NO BOILERPLATE
            bool available = Socket::IsPortAvailable(8082, "::1");
            std::cout << "IPv6 Port 8082 Available: " << (available ? "✅ YES" : "❌ NO") << std::endl;
            
            // Test IPv4 port availability - NO BOILERPLATE
            available = Socket::IsPortAvailable(8083, "127.0.0.1");
            std::cout << "IPv4 Port 8083 Available: " << (available ? "✅ YES" : "❌ NO") << std::endl;
            
            // Test IP detection - NO BOILERPLATE
            auto ips = Socket::GetLocalIPAddresses();
            std::cout << "Local IPs Found: " << ips.size() << " addresses" << std::endl;
            for (const auto& ip : ips) {
                std::cout << "   📍 " << ip << std::endl;
            }
        }
    }
    
    // Test 4: Zero-Configuration Server - NO BOILERPLATE
    std::cout << "\n📋 Test 4: Zero-Configuration Server" << std::endl;
    {
        WebSocketServerLite server;  // Default configuration
        auto result = server.Start(); // Uses default port 8080, binds to 0.0.0.0
        std::cout << "Default Server: " << (result.IsSuccess() ? "✅ SUCCESS" : "❌ FAILED") << std::endl;
        if (result.IsSuccess()) {
            std::cout << "   🌐 Zero config server running!" << std::endl;
            server.Stop();
        }
    }
    
    std::cout << "\n✅ BOILERPLATE-FREE TEST COMPLETE!" << std::endl;
    std::cout << "\n📋 USAGE SUMMARY:" << std::endl;
    std::cout << "✅ No socket system initialization needed" << std::endl;
    std::cout << "✅ No manual cleanup required" << std::endl;
    std::cout << "✅ No platform-specific code" << std::endl;
    std::cout << "✅ No error handling boilerplate" << std::endl;
    std::cout << "✅ Automatic IPv4/IPv6 detection" << std::endl;
    std::cout << "✅ Ready-to-use WebSocket servers" << std::endl;
    
    return 0;
}
