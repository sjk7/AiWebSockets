// Socket Tests - Additional socket-specific tests
// These are currently included in main.cpp but can be moved here for modularity

#include "WebSocket/ErrorCodes.h"
#include "WebSocket/Socket.h"
#include <iostream>

int main() {
    std::cout << "Running Socket Tests..." << std::endl;
    
    // Basic socket creation test
    WebSocket::Socket socket;
    auto result = socket.create(WebSocket::socketFamily::IPV4, WebSocket::socketType::TCP);
    
    if (result.isSuccess()) {
        std::cout << "✅ Socket creation test passed" << std::endl;
        socket.close();
    } else {
        std::cout << "❌ Socket creation test failed: " << result.getErrorMessage() << std::endl;
        return 1;
    }
    
    std::cout << "Socket tests completed successfully!" << std::endl;
    return 0;
}
