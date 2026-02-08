// SIMPLE WEBSOCKET SERVER EXAMPLE
// WebSocketServerLite automatically handles HTTP upgrades to WebSocket
#include "WebSocket/WebSocketServerLite.h"
#include <iostream>

int main() {
    using namespace WebSocket;
    
    // Create WebSocket server
    WebSocketServerLite server;
    server.SetPort(8080)
           .SetBindAddress("127.0.0.1")
           
           // Handle WebSocket connections
           .OnConnect([](const std::string& clientIP) {
               std::cout << "🔗 WebSocket client connected: " << clientIP << std::endl;
           })
           
           .OnMessage([](const std::string& message) -> std::string {
               std::cout << "📨 WebSocket message: " << message << std::endl;
               return "Echo: " + message;
           })
           
           .OnDisconnect([](const std::string& clientIP) {
               std::cout << "🔌 WebSocket client disconnected: " << clientIP << std::endl;
           })
           
           .Start();  // Start the server
    
    std::cout << "🚀 WebSocket Server started!" << std::endl;
    std::cout << " WebSocket: ws://localhost:8080" << std::endl;
    std::cout << "\n📋 Connect with browser JavaScript:" << std::endl;
    std::cout << "const ws = new WebSocket('ws://localhost:8080');" << std::endl;
    std::cout << "ws.onmessage = (e) => console.log(e.data);" << std::endl;
    std::cout << "ws.send('Hello Server!');" << std::endl;
    std::cout << "\nPress Enter to stop..." << std::endl;
    
    std::cin.get();
    return 0;  // Automatic cleanup
}
