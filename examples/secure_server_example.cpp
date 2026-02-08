// HTTP + WEBSOCKET SERVER EXAMPLE
// Built-in anti-DDoS and protection features
#include "WebSocket/HttpWsServer.h"
#include <iostream>

using namespace WebSocket;

int main() {
    std::cout << "🛡️  HTTP + WebSocket Server" << std::endl;
    std::cout << "===================================" << std::endl;
    
    // Configure security settings
    SecurityConfig security;
    security.maxConnectionsPerIP = 3;           // Max 3 connections per IP
    security.maxConnectionsTotal = 50;           // Max 50 total connections
    security.maxRequestsPerIP = 10;              // Max 10 requests per IP per reset period
    security.requestResetPeriodSeconds = 60;      // Reset period of 1 minute
    security.maxRequestSize = 4096;              // Max 4KB HTTP requests
    security.maxMessageSize = 32768;             // Max 32KB WebSocket messages
    security.connectionTimeoutSeconds = 300;     // 5 minute timeout
    security.enableRequestSizeLimit = true;      // Enable request size limits
    security.enableMessageSizeLimit = true;      // Enable message size limits
    
    // Create protected server
    HttpWsServer server(8081, "127.0.0.1", security);
    
    // Configure HTTP request handling
    server.OnHttpRequest([&server](const HTTPRequest& request) -> std::string {
        std::cout << "🌐 HTTP " << request.method << " " << request.path << " from " << request.clientIP << std::endl;
        
        if (request.path == "/") {
            return "<!DOCTYPE html><html><head><title>Protected Server</title>"
                   "<meta charset='UTF-8'>"
                   "<meta http-equiv='Content-Type' content='text/html; charset=UTF-8'>"
                   "</head>"
                   "<body><h1>🛡️ HTTP + WebSocket Server</h1>"
                   "<p>This server has built-in anti-DDoS protection!</p>"
                   "<h2>Protection Features:</h2>"
                   "<ul>"
                   "<li>✅ Connection rate limiting</li>"
                   "<li>✅ IP-based connection limits</li>"
                   "<li>✅ Request size validation</li>"
                   "<li>✅ Message size limits</li>"
                   "<li>✅ Connection timeout</li>"
                   "<li>✅ IP blocking capability</li>"
                   "</ul>"
                   "<h2>Test WebSocket:</h2>"
                   "<button onclick='testWebSocket()'>Test WebSocket</button>"
                   "<div id='output'></div>"
                   "<script>"
                   "function testWebSocket() {"
                   "  const ws = new WebSocket('ws://localhost:8081');"
                   "  ws.onopen = () => ws.send('Hello from protected client!');"
                   "  ws.onmessage = (e) => {"
                   "    document.getElementById('output').innerHTML = 'Received: ' + e.data;"
                   "    ws.close();"
                   "  };"
                   "}"
                   "</script>"
                   "</body></html>";
        }
        
        if (request.path == "/api/status") {
            return "{\"status\":\"protected\",\"protection\":\"active\",\"clients\":" + 
                   std::to_string(server.GetCurrentConnectionCount()) + "}";
        }
        
        if (request.path == "/api/block") {
            // Example of blocking an IP (in real usage, this would be authenticated)
            return "{\"message\":\"IP blocking endpoint (requires authentication)\"}";
        }
        
        return "404 Not Found";
    });
    
    // Configure WebSocket message handling
    server.OnWebSocketMessage([](const WebSocketMessageWithIP& message) -> std::string {
        std::cout << "🔌 WebSocket message from " << message.clientIP << ": " << message.message.AsText() << std::endl;
        
        // Echo back with security info
        return "🛡️ Protected Echo: " + message.message.AsText();
    });
    
    // Handle new connections
    server.OnConnect([](const std::string& clientIP) {
        std::cout << "🔗 New connection: " << clientIP << std::endl;
    });
    
    // Handle disconnections
    server.OnDisconnect([](const std::string& clientIP) {
        std::cout << "🔌 Disconnection: " << clientIP << std::endl;
    });
    
    // Handle security violations
    server.OnSecurityViolation([](const std::string& clientIP, const std::string& reason) {
        std::cout << "🚨 SECURITY VIOLATION from " << clientIP << ": " << reason << std::endl;
    });
    
    // Handle errors
    server.OnError([](const std::string& error) {
        std::cout << "❌ Server error: " << error << std::endl;
    });
    
    // Start the server
    auto result = server.Start();
    if (!result.IsSuccess()) {
        std::cout << "❌ Failed to start server: " << result.GetErrorMessage() << std::endl;
        return 1;
    }
    
    std::cout << "✅ Protected server started!" << std::endl;
    std::cout << "🌐 HTTP: http://localhost:" << server.GetPort() << std::endl;
    std::cout << "🔌 WebSocket: ws://localhost:" << server.GetPort() << std::endl;
    std::cout << "\n🛡️ Protection Features Active:" << std::endl;
    std::cout << "   • Request limiting: " << security.maxRequestsPerIP << " requests per IP per " << security.requestResetPeriodSeconds << " seconds" << std::endl;
    std::cout << "   • Max connections per IP: " << security.maxConnectionsPerIP << std::endl;
    std::cout << "   • Max total connections: " << security.maxConnectionsTotal << std::endl;
    std::cout << "   • Max request size: " << security.maxRequestSize << " bytes" << std::endl;
    std::cout << "   • Max message size: " << security.maxMessageSize << " bytes" << std::endl;
    std::cout << "   • Connection timeout: " << security.connectionTimeoutSeconds << " seconds" << std::endl;
    std::cout << "   • Local addresses: Exempt from all limits" << std::endl;
    
    std::cout << "\n📋 Server Commands:" << std::endl;
    std::cout << "   Type 'status' to see connection info" << std::endl;
    std::cout << "   Type 'block <ip>' to block an IP" << std::endl;
    std::cout << "   Type 'unblock <ip>' to unblock an IP" << std::endl;
    std::cout << "   Type 'list' to see connected IPs" << std::endl;
    std::cout << "   Type 'quit' to stop server" << std::endl;
    
    // Interactive command loop
    std::string command;
    while (std::getline(std::cin, command)) {
        if (command == "quit" || command == "exit") {
            break;
        } else if (command == "status") {
            std::cout << "📊 Server Status:" << std::endl;
            std::cout << "   Connections: " << server.GetCurrentConnectionCount() << "/" << security.maxConnectionsTotal << std::endl;
            std::cout << "   Running: " << (server.IsRunning() ? "Yes" : "No") << std::endl;
        } else if (command.substr(0, 6) == "block ") {
            std::string ip = command.substr(6);
            server.BlockIP(ip);
            std::cout << "🚫 Blocked IP: " << ip << std::endl;
        } else if (command.substr(0, 8) == "unblock ") {
            std::string ip = command.substr(8);
            server.UnblockIP(ip);
            std::cout << "✅ Unblocked IP: " << ip << std::endl;
        } else if (command == "list") {
            auto ips = server.GetConnectedIPs();
            std::cout << "📋 Connected IPs (" << ips.size() << "):" << std::endl;
            for (const auto& ip : ips) {
                std::cout << "   • " << ip << std::endl;
            }
        } else if (command == "help") {
            std::cout << "📋 Available commands:" << std::endl;
            std::cout << "   status - Show server status" << std::endl;
            std::cout << "   block <ip> - Block an IP address" << std::endl;
            std::cout << "   unblock <ip> - Unblock an IP address" << std::endl;
            std::cout << "   list - List connected IPs" << std::endl;
            std::cout << "   quit - Stop server" << std::endl;
        } else if (!command.empty()) {
            std::cout << "❓ Unknown command. Type 'help' for available commands." << std::endl;
        }
    }
    
    std::cout << "\n🛑 Stopping protected server..." << std::endl;
    server.Stop();
    std::cout << "✅ Server stopped safely!" << std::endl;
    
    return 0;
}
