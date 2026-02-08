#include <iostream>
#include <sstream>
#include <vector>
#include <algorithm>
#include <cctype>
#include "WebSocket/Socket.h"
#include "WebSocket/WebSocketProtocol.h"

using namespace WebSocket;

// HTTP Response helper
std::string GenerateHTTPResponse(const std::string& status, const std::string& contentType, const std::string& body) {
    std::stringstream response;
    response << "HTTP/1.1 " << status << "\r\n";
    response << "Content-Type: " << contentType << "\r\n";
    response << "Content-Length: " << body.length() << "\r\n";
    response << "Connection: close\r\n";
    response << "\r\n";
    response << body;
    return response.str();
}

// Check if request is WebSocket upgrade
bool IsWebSocketUpgrade(const std::string& request) {
    // Convert to lowercase for case-insensitive comparison
    std::string lowerRequest = request;
    std::transform(lowerRequest.begin(), lowerRequest.end(), lowerRequest.begin(), ::tolower);
    
    return lowerRequest.find("upgrade: websocket") != std::string::npos &&
           lowerRequest.find("connection: upgrade") != std::string::npos &&
           lowerRequest.find("sec-websocket-key:") != std::string::npos;
}

// Parse HTTP request to get path and method
bool ParseHTTPRequest(const std::string& request, std::string& method, std::string& path) {
    size_t lineEnd = request.find("\r\n");
    if (lineEnd == std::string::npos) return false;
    
    std::string firstLine = request.substr(0, lineEnd);
    std::stringstream ss(firstLine);
    
    if (!(ss >> method >> path)) return false;
    return true;
}

int main() {
    std::cout << "Hybrid HTTP/WebSocket Server" << std::endl;
    std::cout << "=============================" << std::endl;
    
    // Create server socket
    Socket serverSocket;
    if (!serverSocket.Create(SOCKET_FAMILY::IPV4, SOCKET_TYPE::TCP).IsSuccess()) {
        std::cout << "❌ Failed to create server socket" << std::endl;
        return 1;
    }
    
    // Configure server
    serverSocket.ReuseAddress(true);
    
    if (!serverSocket.Bind("127.0.0.1", 8080).IsSuccess()) {
        std::cout << "❌ Failed to bind server socket" << std::endl;
        return 1;
    }
    
    if (!serverSocket.Listen(10).IsSuccess()) {
        std::cout << "❌ Failed to start listening" << std::endl;
        return 1;
    }
    
    std::cout << "🚀 Hybrid server listening on 127.0.0.1:8080" << std::endl;
    std::cout << "📝 Supports both HTTP and WebSocket connections" << std::endl;
    std::cout << "🔄 Press Ctrl+C to stop" << std::endl << std::endl;
    
    int clientCount = 0;
    
    // Main server loop
    while (true) {
        std::cout << "⏳ Waiting for client connection..." << std::endl;
        
        // Accept client connection
        AcceptResult acceptResult = serverSocket.Accept();
        
        if (!acceptResult.first.IsSuccess()) {
            std::cout << "❌ Failed to accept client: " << acceptResult.first.GetErrorMessage() << std::endl;
            continue;
        }
        
        if (!acceptResult.second) {
            std::cout << "❌ Accepted socket is null" << std::endl;
            continue;
        }
        
        clientCount++;
        std::cout << "✅ Client " << clientCount << " connected!" << std::endl;
        
        // Receive initial request to determine protocol
        std::cout << "📨 Receiving initial request..." << std::endl;
        ReceiveResult receiveResult = acceptResult.second->Receive(4096);
        
        if (!receiveResult.first.IsSuccess() || receiveResult.second.empty()) {
            std::cout << "❌ Failed to receive request or client disconnected" << std::endl;
            acceptResult.second->Close();
            continue;
        }
        
        std::string request(receiveResult.second.begin(), receiveResult.second.end());
        std::cout << "📄 Request received (" << request.length() << " bytes)" << std::endl;
        
        // Check if this is a WebSocket upgrade request
        if (IsWebSocketUpgrade(request)) {
            std::cout << "🔌 WebSocket connection requested" << std::endl;
            
            // Process WebSocket handshake
            HandshakeInfo handshakeInfo;
            Result handshakeResult = WebSocketProtocol::ValidateHandshakeRequest(request, handshakeInfo);
            
            if (handshakeResult.IsSuccess()) {
                std::cout << "✅ WebSocket handshake validated" << std::endl;
                
                // Generate handshake response
                std::string response = WebSocketProtocol::GenerateHandshakeResponse(handshakeInfo);
                auto sendResult = acceptResult.second->Send(std::vector<uint8_t>(response.begin(), response.end()));
                
                if (sendResult.IsSuccess()) {
                    std::cout << "🤝 WebSocket handshake completed" << std::endl;
                    std::cout << "💬 WebSocket connection established - ready for frames!" << std::endl;
                    
                    // Send a welcome message as WebSocket frame
                    WebSocketFrame welcomeFrame = WebSocketProtocol::CreateTextFrame("Welcome to WebSocket server!");
                    std::vector<uint8_t> welcomeData = WebSocketProtocol::GenerateFrame(welcomeFrame);
                    acceptResult.second->Send(welcomeData);
                    
                    std::cout << "📤 Sent welcome message as WebSocket frame" << std::endl;
                } else {
                    std::cout << "❌ Failed to send WebSocket handshake response" << std::endl;
                }
            } else {
                std::cout << "❌ Invalid WebSocket handshake: " << handshakeResult.GetErrorMessage() << std::endl;
                
                // Send bad request response
                std::string errorResponse = GenerateHTTPResponse("400 Bad Request", "text/plain", "Invalid WebSocket handshake");
                acceptResult.second->Send(std::vector<uint8_t>(errorResponse.begin(), errorResponse.end()));
            }
            
        } else {
            // Handle as HTTP request
            std::cout << "🌐 HTTP connection requested" << std::endl;
            
            std::string method, path;
            if (ParseHTTPRequest(request, method, path)) {
                std::cout << "📋 " << method << " " << path << std::endl;
                
                // Generate HTTP response based on path
                std::string response;
                if (path == "/") {
                    std::string body = "<html><body><h1>Hybrid Server</h1><p>This server supports both HTTP and WebSocket!</p><p>Try connecting with WebSocket client to ws://127.0.0.1:8080</p></body></html>";
                    response = GenerateHTTPResponse("200 OK", "text/html", body);
                } else if (path == "/api") {
                    std::string body = "{\"message\": \"Hello from HTTP API!\", \"server\": \"Hybrid HTTP/WebSocket Server\"}";
                    response = GenerateHTTPResponse("200 OK", "application/json", body);
                } else {
                    std::string body = "404 Not Found";
                    response = GenerateHTTPResponse("404 Not Found", "text/plain", body);
                }
                
                auto sendResult = acceptResult.second->Send(std::vector<uint8_t>(response.begin(), response.end()));
                
                if (sendResult.IsSuccess()) {
                    std::cout << "📤 HTTP response sent successfully" << std::endl;
                } else {
                    std::cout << "❌ Failed to send HTTP response" << std::endl;
                }
            } else {
                std::cout << "❌ Invalid HTTP request" << std::endl;
                
                // Send bad request response
                std::string errorResponse = GenerateHTTPResponse("400 Bad Request", "text/plain", "Invalid HTTP request");
                acceptResult.second->Send(std::vector<uint8_t>(errorResponse.begin(), errorResponse.end()));
            }
        }
        
        // Close connection
        acceptResult.second->Close();
        std::cout << "🔌 Client " << clientCount << " disconnected" << std::endl;
        std::cout << "--------------------------------------------------" << std::endl;
        
        // Loop continues - wait for next client
    }
    
    // Clean up (never reached in this infinite loop example)
    serverSocket.Close();
    return 0;
}
