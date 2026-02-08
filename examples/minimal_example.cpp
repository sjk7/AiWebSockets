// ABSOLUTE MINIMAL WEBSOCKET SERVER EXAMPLE
// No includes beyond WebSocket, no boilerplate, no cleanup needed
#include "WebSocket/WebSocketServerLite.h"
#include <iostream>

int main() {
    using namespace WebSocket;
    
    // Create and start server in ONE LINE
    WebSocketServerLite server;
    server.SetPort(8080)
           .OnMessage([](const std::string& msg) {
               std::cout << "Got: " << msg << std::endl;
           })
           .Start();  // That's it! Server is running
    
    std::cout << "🚀 Server running on ws://localhost:8080" << std::endl;
    std::cout << "Press Enter to stop..." << std::endl;
    std::cin.get();
    
    return 0;  // Automatic cleanup happens here
}
