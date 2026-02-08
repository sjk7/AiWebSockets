// SIMPLE HTTP + WEBSOCKET HYBRID SERVER
// Serves HTTP pages and upgrades to WebSocket when requested
#include "WebSocket/Socket.h"
#include "WebSocket/WebSocketProtocol.h"
#include <iostream>
#include <sstream>

using namespace WebSocket;

// Simple HTTP response generator
std::string HTTPResponse(const std::string& status, const std::string& contentType, const std::string& body) {
    std::stringstream response;
    response << "HTTP/1.1 " << status << "\r\n";
    response << "Content-Type: " << contentType << "\r\n";
    response << "Content-Length: " << body.length() << "\r\n";
    response << "Connection: close\r\n";
    response << "\r\n";
    response << body;
    return response.str();
}

// Check if request wants WebSocket upgrade
bool IsWebSocketRequest(const std::string& request) {
    return request.find("Upgrade: websocket") != std::string::npos &&
           request.find("Connection: Upgrade") != std::string::npos &&
           request.find("Sec-WebSocket-Key:") != std::string::npos;
}

// Get HTTP path from request
std::string GetPath(const std::string& request) {
    size_t end = request.find("\r\n");
    if (end == std::string::npos) return "/";
    
    std::string firstLine = request.substr(0, end);
    std::stringstream ss(firstLine);
    std::string method, path;
    ss >> method >> path;
    return path.empty() ? "/" : path;
}

int main() {
    std::cout << "🚀 Simple HTTP + WebSocket Server" << std::endl;
    std::cout << "===================================" << std::endl;
    
    // Create server socket
    Socket serverSocket;
    auto createResult = serverSocket.Create(SOCKET_FAMILY::IPV4, SOCKET_TYPE::TCP);
    if (!createResult.IsSuccess()) {
        std::cout << "❌ Failed to create socket: " << createResult.GetErrorMessage() << std::endl;
        return 1;
    }
    
    // Bind and listen
    auto bindResult = serverSocket.Bind("127.0.0.1", 8080);
    if (!bindResult.IsSuccess()) {
        std::cout << "❌ Failed to bind: " << bindResult.GetErrorMessage() << std::endl;
        return 1;
    }
    
    auto listenResult = serverSocket.Listen();
    if (!listenResult.IsSuccess()) {
        std::cout << "❌ Failed to listen: " << listenResult.GetErrorMessage() << std::endl;
        return 1;
    }
    
    std::cout << "✅ Server started on http://localhost:8080" << std::endl;
    std::cout << "🔌 WebSocket available at: ws://localhost:8080" << std::endl;
    std::cout << "\n📋 Try these:" << std::endl;
    std::cout << "1. Browser: http://localhost:8080" << std::endl;
    std::cout << "2. WebSocket: ws://localhost:8080" << std::endl;
    std::cout << "\nPress Ctrl+C to stop" << std::endl;
    
    // Main server loop
    while (true) {
        // Accept connection
        auto [acceptResult, clientSocket] = serverSocket.Accept();
        if (!acceptResult.IsSuccess() || !clientSocket) {
            continue; // Try again
        }
        
        // Receive HTTP request
        auto [receiveResult, requestData] = clientSocket->Receive(4096);
        if (!receiveResult.IsSuccess() || requestData.empty()) {
            clientSocket->Close();
            continue;
        }
        
        std::string request(requestData.begin(), requestData.end());
        std::string path = GetPath(request);
        
        if (IsWebSocketRequest(request)) {
            // WebSocket upgrade path
            std::cout << "🔌 WebSocket upgrade request: " << path << std::endl;
            
            // Perform WebSocket handshake
            HandshakeInfo info;
            auto handshakeResult = WebSocketProtocol::ValidateHandshakeRequest(request, info);
            
            if (handshakeResult.IsSuccess()) {
                // Send WebSocket handshake response
                std::string response = WebSocketProtocol::GenerateHandshakeResponse(info);
                clientSocket->Send(std::vector<uint8_t>(response.begin(), response.end()));
                
                std::cout << "✅ WebSocket handshake successful" << std::endl;
                
                // Handle WebSocket messages
                while (true) {
                    auto [msgResult, msgData] = clientSocket->Receive(4096);
                    if (!msgResult.IsSuccess() || msgData.empty()) {
                        break; // Client disconnected
                    }
                    
                    // Parse WebSocket frame
                    WebSocketFrame frame;
                    size_t bytesConsumed = 0;
                    auto parseResult = WebSocketProtocol::ParseFrame(msgData, frame, bytesConsumed);
                    
                    if (parseResult.IsSuccess() && frame.Opcode == WEBSOCKET_OPCODE::TEXT) {
                        std::string message(frame.PayloadData.begin(), frame.PayloadData.end());
                        std::cout << "📨 WebSocket message: " << message << std::endl;
                        
                        // Echo back
                        std::string echo = "Echo: " + message;
                        WebSocketFrame responseFrame = WebSocketProtocol::CreateTextFrame(echo);
                        std::vector<uint8_t> responseData = WebSocketProtocol::GenerateFrame(responseFrame);
                        clientSocket->Send(responseData);
                    }
                }
                
                std::cout << "🔌 WebSocket client disconnected" << std::endl;
            } else {
                std::cout << "❌ Invalid WebSocket handshake: " << handshakeResult.GetErrorMessage() << std::endl;
                clientSocket->Close();
            }
        } else {
            // HTTP response path
            std::cout << "🌐 HTTP request: " << path << std::endl;
            
            std::string response;
            if (path == "/") {
                response = HTTPResponse("200 OK", "text/html", 
                    "<!DOCTYPE html><html><head><title>HTTP + WebSocket Server</title></head>"
                    "<body><h1>HTTP + WebSocket Server</h1>"
                    "<p>This server handles both HTTP and WebSocket!</p>"
                    "<p>Connect via WebSocket:</p>"
                    "<script>const ws = new WebSocket('ws://localhost:8080');"
                    "ws.onopen = () => ws.send('Hello from browser!');"
                    "ws.onmessage = (e) => console.log('Received:', e.data);</script>"
                    "</body></html>");
            } else if (path == "/api/status") {
                response = HTTPResponse("200 OK", "application/json", 
                    "{\"status\":\"running\",\"websocket\":\"available\"}");
            } else {
                response = HTTPResponse("404 Not Found", "text/plain", "Not Found");
            }
            
            clientSocket->Send(std::vector<uint8_t>(response.begin(), response.end()));
            clientSocket->Close();
        }
    }
    
    return 0;
}
