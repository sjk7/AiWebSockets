// SIMPLE HTTP + WEBSOCKET SERVER EXAMPLE
// Handles HTTP requests and upgrades to WebSocket automatically
#include "WebSocket/WebSocketServerLite.h"
#include <iostream>

int main() {
    using namespace WebSocket;
    
    // Create server that handles both HTTP and WebSocket
    WebSocketServerLite server;
    server.SetPort(8080)
           .SetBindAddress("127.0.0.1")
           
           // Handle WebSocket connections (upgraded from HTTP)
           .OnConnect([](const std::string& clientIP) {
               std::cout << "🔗 WebSocket connected: " << clientIP << std::endl;
           })
           
           .OnMessage([](const std::string& message) {
               std::cout << "📨 WebSocket message: " << message << std::endl;
               
               // Echo back with a prefix
               return "Echo: " + message;
           })
           
           .OnDisconnect([](const std::string& clientIP) {
               std::cout << "🔌 WebSocket disconnected: " << clientIP << std::endl;
           })
           
           .OnError([](const Result& error) {
               std::cout << "❌ Server error: " << error.GetErrorMessage() << std::endl;
           })
           
           .Start();  // Start the server
    
    std::cout << "🚀 HTTP + WebSocket Server started!" << std::endl;
    std::cout << "📱 HTTP requests are handled automatically for WebSocket upgrade" << std::endl;
    std::cout << "🔌 WebSocket: ws://localhost:8080" << std::endl;
    std::cout << "\n📋 How it works:" << std::endl;
    std::cout << "1. HTTP requests → WebSocket handshake (automatic)" << std::endl;
    std::cout << "2. WebSocket upgrade → Real-time messaging" << std::endl;
    std::cout << "3. Non-WebSocket HTTP → 400 Bad Request (WebSocket only)" << std::endl;
    std::cout << "\n🌐 Test with browser JavaScript:" << std::endl;
    std::cout << "const ws = new WebSocket('ws://localhost:8080');" << std::endl;
    std::cout << "ws.onopen = () => ws.send('Hello Server!');" << std::endl;
    std::cout << "ws.onmessage = (e) => console.log('Received:', e.data);" << std::endl;
    std::cout << "\nPress Enter to stop..." << std::endl;
    
    std::cin.get();
    return 0;  // Automatic cleanup
}
