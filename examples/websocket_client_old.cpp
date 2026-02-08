#include <iostream>
#include <sstream>
#include "WebSocket/Socket.h"
#include "WebSocket/WebSocketProtocol.h"

using namespace WebSocket;

int main() {
    std::cout << "WebSocket Client for Hybrid Server" << std::endl;
    std::cout << "===================================" << std::endl;
    
    // Create client socket
    Socket clientSocket;
    if (!clientSocket.Create(SOCKET_FAMILY::IPV4, SOCKET_TYPE::TCP).IsSuccess()) {
        std::cout << "❌ Failed to create client socket" << std::endl;
        return 1;
    }
    
    // Connect to server
    std::cout << "🔗 Connecting to 127.0.0.1:8080..." << std::endl;
    if (!clientSocket.Connect("127.0.0.1", 8080).IsSuccess()) {
        std::cout << "❌ Failed to connect to server" << std::endl;
        return 1;
    }
    
    std::cout << "✅ Connected to server!" << std::endl;
    
    // Send WebSocket handshake request
    std::string handshakeRequest = 
        "GET / HTTP/1.1\r\n"
        "Host: 127.0.0.1:8080\r\n"
        "Upgrade: websocket\r\n"
        "Connection: Upgrade\r\n"
        "Sec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\n"
        "Sec-WebSocket-Version: 13\r\n"
        "\r\n";
    
    std::cout << "🤝 Sending WebSocket handshake..." << std::endl;
    auto sendResult = clientSocket.Send(std::vector<uint8_t>(handshakeRequest.begin(), handshakeRequest.end()));
    
    if (!sendResult.IsSuccess()) {
        std::cout << "❌ Failed to send handshake: " << sendResult.ErrorMessage << std::endl;
        clientSocket.Close();
        return 1;
    }
    
    // Receive handshake response
    std::cout << "📨 Receiving handshake response..." << std::endl;
    ReceiveResult receiveResult = clientSocket.Receive(4096);
    
    if (!receiveResult.first.IsSuccess() || receiveResult.second.empty()) {
        std::cout << "❌ Failed to receive handshake response" << std::endl;
        clientSocket.Close();
        return 1;
    }
    
    std::string response(receiveResult.second.begin(), receiveResult.second.end());
    std::cout << "📄 Handshake Response:" << std::endl;
    std::cout << "========================" << std::endl;
    std::cout << response << std::endl;
    std::cout << "========================" << std::endl;
    
    // Check if handshake was successful
    if (response.find("101 Switching Protocols") != std::string::npos) {
        std::cout << "✅ WebSocket handshake successful!" << std::endl;
        
        // Receive welcome message from server
        std::cout << "📨 Waiting for welcome message..." << std::endl;
        ReceiveResult welcomeResult = clientSocket.Receive(4096);
        
        if (welcomeResult.first.IsSuccess() && !welcomeResult.second.empty()) {
            // Parse WebSocket frame
            WebSocketFrame frame;
            size_t bytesConsumed = 0;
            Result parseResult = WebSocketProtocol::ParseFrame(welcomeResult.second, frame, bytesConsumed);
            
            if (parseResult.IsSuccess() && frame.Opcode == WEBSOCKET_OPCODE::TEXT) { // Text frame
                std::string message(frame.PayloadData.begin(), frame.PayloadData.end());
                std::cout << "💬 Welcome message: \"" << message << "\"" << std::endl;
                
                // Send a message back to server
                std::string testMessage = "Hello from WebSocket client!";
                WebSocketFrame sendFrame = WebSocketProtocol::CreateTextFrame(testMessage);
                std::vector<uint8_t> frameData = WebSocketProtocol::GenerateFrame(sendFrame);
                
                std::cout << "📤 Sending message: \"" << testMessage << "\"" << std::endl;
                auto frameSendResult = clientSocket.Send(frameData);
                
                if (frameSendResult.IsSuccess()) {
                    std::cout << "✅ Message sent successfully!" << std::endl;
                } else {
                    std::cout << "❌ Failed to send message: " << frameSendResult.ErrorMessage << std::endl;
                }
            } else {
                std::cout << "❌ Failed to parse welcome frame" << std::endl;
            }
        } else {
            std::cout << "❌ Failed to receive welcome message" << std::endl;
        }
        
    } else {
        std::cout << "❌ WebSocket handshake failed!" << std::endl;
    }
    
    // Close connection
    clientSocket.Close();
    std::cout << "🔌 Disconnected from server" << std::endl;
    
    return 0;
}
