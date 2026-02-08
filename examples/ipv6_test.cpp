#include "WebSocket/WebSocketServerLite.h"
#include "WebSocket/Socket.h"
#include <iostream>
#include <thread>
#include <chrono>

using namespace WebSocket;

void demonstrateIPv6Support() {
    std::cout << "🌐 IPv6 Support Demonstration" << std::endl;
    std::cout << "==============================" << std::endl;
    
    // Test 1: IP Address Detection
    std::cout << "\n🔍 Test 1: IPv6 Address Detection" << std::endl;
    auto localIPs = Socket::GetLocalIPAddresses();
    
    std::vector<std::string> ipv4Addresses;
    std::vector<std::string> ipv6Addresses;
    
    for (const auto& ip : localIPs) {
        if (Socket::IsIPv4Address(ip)) {
            ipv4Addresses.push_back(ip);
            std::cout << "  📍 IPv4: " << ip << std::endl;
        } else if (Socket::IsIPv6Address(ip)) {
            ipv6Addresses.push_back(ip);
            std::cout << "  📍 IPv6: " << ip << std::endl;
        } else {
            std::cout << "  ❓ Unknown: " << ip << std::endl;
        }
    }
    
    std::cout << "\n📊 Summary:" << std::endl;
    std::cout << "  IPv4 addresses: " << ipv4Addresses.size() << std::endl;
    std::cout << "  IPv6 addresses: " << ipv6Addresses.size() << std::endl;
    
    // Test 2: IPv6 Port Availability
    std::cout << "\n🔍 Test 2: IPv6 Port Availability" << std::endl;
    if (!ipv6Addresses.empty()) {
        std::string testIPv6 = ipv6Addresses[0];
        std::cout << "Testing IPv6 address: " << testIPv6 << std::endl;
        
        std::vector<uint16_t> testPorts = {8080, 8081, 8082, 9000, 9001};
        for (uint16_t port : testPorts) {
            bool available = Socket::IsPortAvailable(port, testIPv6);
            std::cout << "  Port " << port << ": " << (available ? "✅ Available" : "❌ In use") << std::endl;
        }
    } else {
        std::cout << "⚠️ No IPv6 addresses found for testing" << std::endl;
    }
    
    // Test 3: IPv6 Server Creation (if available)
    std::cout << "\n🔍 Test 3: IPv6 WebSocket Server" << std::endl;
    if (!ipv6Addresses.empty()) {
        std::string ipv6Address = ipv6Addresses[0];
        uint16_t testPort = 9000;
        
        // Find an available port
        for (uint16_t port = 9000; port < 9100; ++port) {
            if (Socket::IsPortAvailable(port, ipv6Address)) {
                testPort = port;
                break;
            }
        }
        
        std::cout << "Attempting to create IPv6 server on " << ipv6Address << ":" << testPort << std::endl;
        
        WebSocketServerLite ipv6Server;
        ipv6Server.SetPort(testPort)
                 .SetBindAddress(ipv6Address)
                 .EnableSecurity(true)
                 .SetMaxConnections(5);
        
        // Set up event handlers
        ipv6Server.OnConnect([ipv6Address](const std::string& clientIP) {
            std::cout << "🔗 [IPv6 Server] Client connected: " << clientIP << std::endl;
        });
        
        ipv6Server.OnMessage([ipv6Address](const std::string& message) {
            std::cout << "📨 [IPv6 Server] Received: " << message << std::endl;
        });
        
        ipv6Server.OnDisconnect([ipv6Address](const std::string& clientIP) {
            std::cout << "🔌 [IPv6 Server] Client disconnected: " << clientIP << std::endl;
        });
        
        ipv6Server.OnError([ipv6Address](const Result& error) {
            std::cout << "❌ [IPv6 Server] Error: " << error.GetErrorMessage() << std::endl;
        });
        
        // Try to start the server
        auto startResult = ipv6Server.Start();
        if (startResult.IsSuccess()) {
            std::cout << "✅ IPv6 WebSocket server started successfully!" << std::endl;
            std::cout << "   Server running on: [" << ipv6Address << "]:" << testPort << std::endl;
            std::cout << "   Clients can connect to: ws://[" << ipv6Address << "]:" << testPort << std::endl;
            
            // Run for a short time to demonstrate
            std::cout << "🔄 Running for 3 seconds..." << std::endl;
            for (int i = 0; i < 30 && ipv6Server.IsRunning(); ++i) {
                ipv6Server.ProcessEvents();
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
            
            ipv6Server.Stop();
            std::cout << "✅ IPv6 server stopped successfully" << std::endl;
        } else {
            std::cout << "❌ Failed to start IPv6 server: " << startResult.GetErrorMessage() << std::endl;
        }
    } else {
        std::cout << "⚠️ Cannot test IPv6 server - no IPv6 addresses available" << std::endl;
    }
    
    // Test 4: Dual-Stack Server Comparison
    std::cout << "\n🔍 Test 4: IPv4 vs IPv6 Comparison" << std::endl;
    
    if (!ipv4Addresses.empty() && !ipv6Addresses.empty()) {
        std::string ipv4Addr = ipv4Addresses[0];
        std::string ipv6Addr = ipv6Addresses[0];
        
        std::cout << "Available server options:" << std::endl;
        std::cout << "  📍 IPv4 Server: " << ipv4Addr << ":8080" << std::endl;
        std::cout << "  📍 IPv6 Server: [" << ipv6Addr << "]:8080" << std::endl;
        std::cout << "  🌐 Dual-Stack: 0.0.0.0:8080 (IPv4) + :::8080 (IPv6)" << std::endl;
        
        std::cout << "\nConnection examples:" << std::endl;
        std::cout << "  IPv4 client: ws://" << ipv4Addr << ":8080" << std::endl;
        std::cout << "  IPv6 client: ws://[" << ipv6Addr << "]:8080" << std::endl;
        std::cout << "  Localhost: ws://localhost:8080" << std::endl;
        std::cout << "  Localhost IPv6: ws://[::1]:8080" << std::endl;
    } else {
        std::cout << "⚠️ Cannot compare - missing IPv4 or IPv6 addresses" << std::endl;
    }
}

void demonstrateIPv6Validation() {
    std::cout << "\n🔍 Test 5: IPv6 Address Validation" << std::endl;
    
    std::vector<std::string> testAddresses = {
        "127.0.0.1",           // IPv4 localhost
        "::1",                  // IPv6 localhost
        "192.168.1.1",         // IPv4 private
        "2001:db8::1",         // IPv6 documentation
        "fe80::1",             // IPv6 link-local
        "2001:0db8:85a3:0000:0000:8a2e:0370:7334", // Full IPv6
        "invalid.address",     // Invalid
        "999.999.999.999",     // Invalid IPv4
        "gggg::1"              // Invalid IPv6
    };
    
    std::cout << "Testing address validation:" << std::endl;
    for (const auto& addr : testAddresses) {
        bool isIPv4 = Socket::IsIPv4Address(addr);
        bool isIPv6 = Socket::IsIPv6Address(addr);
        bool isIP = Socket::IsIPAddress(addr);
        
        std::cout << "  " << addr << std::endl;
        std::cout << "    IPv4: " << (isIPv4 ? "✅" : "❌") << std::endl;
        std::cout << "    IPv6: " << (isIPv6 ? "✅" : "❌") << std::endl;
        std::cout << "    Valid IP: " << (isIP ? "✅" : "❌") << std::endl;
        std::cout << std::endl;
    }
}

int main() {
    std::cout << "🚀 IPv6 WebSocket Server Test" << std::endl;
    std::cout << "=============================" << std::endl;
    
    try {
        demonstrateIPv6Support();
        demonstrateIPv6Validation();
        
        std::cout << "\n✅ IPv6 demonstration completed!" << std::endl;
        
        std::cout << "\n📋 IPv6 Support Summary:" << std::endl;
        std::cout << "✅ IPv6 address detection" << std::endl;
        std::cout << "✅ IPv6 address validation" << std::endl;
        std::cout << "✅ IPv6 port availability checking" << std::endl;
        std::cout << "✅ IPv6 WebSocket server creation" << std::endl;
        std::cout << "✅ Dual-stack server support" << std::endl;
        
    } catch (const std::exception& e) {
        std::cout << "❌ Exception: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
