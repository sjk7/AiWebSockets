#include "WebSocket/Socket.h"
#include <iostream>
#include <algorithm>

using namespace WebSocket;

void demonstrateLocalIPEnumeration() {
    std::cout << "=== Local IP Address Enumeration ===" << std::endl;
    
    // Get all local IP addresses
    std::vector<std::string> localIPs = Socket::GetLocalIPAddresses();
    
    if (localIPs.empty()) {
        std::cout << "❌ No local IP addresses found!" << std::endl;
        return;
    }
    
    std::cout << "✅ Found " << localIPs.size() << " local IP address(es):" << std::endl;
    
    // Categorize and display IPs
    std::vector<std::string> loopbackIPs;
    std::vector<std::string> privateIPs;
    std::vector<std::string> publicIPs;
    std::vector<std::string> ipv6IPs;
    
    for (const auto& ip : localIPs) {
        std::cout << "  📍 " << ip << std::endl;
        
        // Categorize the IP
        if (ip.find("::") != std::string::npos) {
            ipv6IPs.push_back(ip);
        }
        else if (ip.find("127.") == 0) {
            loopbackIPs.push_back(ip);
        }
        else if (ip.find("192.168.") == 0 || 
                 ip.find("10.") == 0 || 
                 (ip.find("172.") == 0 && ip.length() > 6)) {
            privateIPs.push_back(ip);
        }
        else {
            publicIPs.push_back(ip);
        }
    }
    
    std::cout << "\n📊 IP Address Categories:" << std::endl;
    
    if (!loopbackIPs.empty()) {
        std::cout << "  🔄 Loopback (localhost): ";
        for (size_t i = 0; i < loopbackIPs.size(); ++i) {
            std::cout << loopbackIPs[i];
            if (i < loopbackIPs.size() - 1) std::cout << ", ";
        }
        std::cout << std::endl;
    }
    
    if (!privateIPs.empty()) {
        std::cout << "  🔒 Private network: ";
        for (size_t i = 0; i < privateIPs.size(); ++i) {
            std::cout << privateIPs[i];
            if (i < privateIPs.size() - 1) std::cout << ", ";
        }
        std::cout << std::endl;
    }
    
    if (!publicIPs.empty()) {
        std::cout << "  🌐 Public network: ";
        for (size_t i = 0; i < publicIPs.size(); ++i) {
            std::cout << publicIPs[i];
            if (i < publicIPs.size() - 1) std::cout << ", ";
        }
        std::cout << std::endl;
    }
    
    if (!ipv6IPs.empty()) {
        std::cout << "  📡 IPv6 addresses: ";
        for (size_t i = 0; i < ipv6IPs.size(); ++i) {
            std::cout << ipv6IPs[i];
            if (i < ipv6IPs.size() - 1) std::cout << ", ";
        }
        std::cout << std::endl;
    }
}

void demonstrateServerBindingOptions() {
    std::cout << "\n=== Server Binding Options ===" << std::endl;
    
    std::vector<std::string> localIPs = Socket::GetLocalIPAddresses();
    
    std::cout << "🔧 Recommended server configurations:" << std::endl;
    
    // Option 1: Bind to all interfaces
    std::cout << "\n  📡 ALL INTERFACES (Recommended for production):" << std::endl;
    std::cout << "     Bind Address: 0.0.0.0" << std::endl;
    std::cout << "     Description: Accept connections from any network interface" << std::endl;
    std::cout << "     Use Case: Public servers, multi-homed hosts" << std::endl;
    
    // Option 2: Bind to localhost
    std::cout << "\n  🔄 LOCALHOST ONLY:" << std::endl;
    std::cout << "     Bind Address: 127.0.0.1" << std::endl;
    std::cout << "     Description: Accept connections only from this machine" << std::endl;
    std::cout << "     Use Case: Development, local services only" << std::endl;
    
    // Option 3: Bind to specific interfaces
    std::cout << "\n  🎯 SPECIFIC INTERFACES:" << std::endl;
    for (const auto& ip : localIPs) {
        if (ip.find("127.") != 0 && ip.find("::") == std::string::npos) { // Skip loopback and IPv6
            std::cout << "     Bind Address: " << ip << std::endl;
            std::cout << "     Description: Bind to specific network interface" << std::endl;
            std::cout << "     Use Case: Multi-NIC servers, specific network access" << std::endl;
            std::cout << std::endl;
        }
    }
}

void demonstratePortScanning() {
    std::cout << "\n=== Port Availability Scanning ===" << std::endl;
    
    std::vector<std::string> localIPs = Socket::GetLocalIPAddresses();
    std::vector<uint16_t> commonPorts = {80, 443, 8080, 8081, 3000, 5000, 8080, 8443};
    
    for (const auto& ip : localIPs) {
        if (ip.find("::") != std::string::npos) continue; // Skip IPv6 for this demo
        
        std::cout << "\n🔍 Scanning ports on " << ip << ":" << std::endl;
        
        for (uint16_t port : commonPorts) {
            bool available = Socket::IsPortAvailable(port, ip);
            std::cout << "  Port " << port << ": " << (available ? "✅ Available" : "❌ In use") << std::endl;
        }
    }
}

void demonstrateWebSocketLiteIntegration() {
    std::cout << "\n=== WebSocket Lite Integration ===" << std::endl;
    
    std::vector<std::string> localIPs = Socket::GetLocalIPAddresses();
    
    std::cout << "🚀 WebSocket Lite server configuration examples:" << std::endl;
    
    // Example configurations
    std::vector<std::pair<std::string, std::string>> configs = {
        {"Development Server", "127.0.0.1"},
        {"Production Server", "0.0.0.0"}
    };
    
    // Add specific IPs if available
    for (const auto& ip : localIPs) {
        if (ip.find("127.") != 0 && ip.find("::") == std::string::npos) {
            configs.push_back({"Interface-Specific Server", ip});
            break; // Just add one example
        }
    }
    
    for (const auto& config : configs) {
        std::cout << "\n  📋 " << config.first << ":" << std::endl;
        std::cout << "     WebSocketServerLite server;" << std::endl;
        std::cout << "     server.SetBindAddress(\"" << config.second << "\")" << std::endl;
        std::cout << "           .SetPort(8080);" << std::endl;
        std::cout << "     server.Start();" << std::endl;
    }
}

int main() {
    std::cout << "🚀 Local IP Address Enumeration Demo" << std::endl;
    std::cout << "=====================================" << std::endl;
    
    try {
        // Demonstrate IP enumeration
        demonstrateLocalIPEnumeration();
        
        // Show server binding options
        demonstrateServerBindingOptions();
        
        // Port availability scanning
        demonstratePortScanning();
        
        // WebSocket Lite integration
        demonstrateWebSocketLiteIntegration();
        
        std::cout << "\n✅ All demonstrations completed!" << std::endl;
        
        std::cout << "\n📋 SUMMARY:" << std::endl;
        std::cout << "✅ Socket::GetLocalIPAddresses() - Enumerate all local IPs" << std::endl;
        std::cout << "✅ Works with IPv4 and IPv6 addresses" << std::endl;
        std::cout << "✅ Cross-platform support (Windows/Linux/macOS)" << std::endl;
        std::cout << "✅ Useful for server interface selection" << std::endl;
        std::cout << "✅ Integrates with WebSocket Lite API" << std::endl;
        
    } catch (const std::exception& e) {
        std::cout << "❌ Exception: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
