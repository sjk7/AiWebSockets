// WebSocket Protocol Tests
// These tests will be added as we implement the protocol

#include "WebSocket/ErrorCodes.h"
#include "WebSocket/WebSocketProtocol.h"
#include <iostream>

int main() {
    std::cout << "Running WebSocket Protocol Tests..." << std::endl;
    
    // Basic frame creation test
    std::string testString = "Hello WebSocket";
    auto textFrame = WebSocket::WebSocketProtocol::createTextFrame(testString);
    
    if (textFrame.PayloadLength == testString.length()) {
        std::cout << "✅ Text frame creation test passed - Payload length: " << textFrame.PayloadLength << std::endl;
    } else {
        std::cout << "❌ Text frame creation test failed - Expected " << testString.length() << ", got " << textFrame.PayloadLength << std::endl;
        return 1;
    }
    
    // Test frame generation
    auto frameData = WebSocket::WebSocketProtocol::generateFrame(textFrame);
    
    if (!frameData.empty()) {
        std::cout << "✅ Frame generation test passed" << std::endl;
    } else {
        std::cout << "❌ Frame generation test failed" << std::endl;
        return 1;
    }
    
    std::cout << "WebSocket Protocol tests completed successfully!" << std::endl;
    return 0;
}
