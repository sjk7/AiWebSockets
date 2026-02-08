#include <iostream>
#include <sstream>
#include <vector>
#include <algorithm>
#include <cctype>
#include <map>
#include <chrono>
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

// Client connection state
enum class ClientState {
    CONNECTED,
    RECEIVING,
    HTTP_PROCESSING,
    WEBSOCKET_HANDSHAKE,
    WEBSOCKET_ESTABLISHED,
    CLOSING
};

struct ClientInfo {
    std::unique_ptr<Socket> socket;
    ClientState state;
    std::string receiveBuffer;
    std::chrono::steady_clock::time_point lastActivity;
    bool isWebSocket;
    
    ClientInfo(std::unique_ptr<Socket> sock) 
        : socket(std::move(sock))
        , state(ClientState::CONNECTED)
        , lastActivity(std::chrono::steady_clock::now())
        , isWebSocket(false) {}
};

int main() {
    std::cout << "Non-Blocking Hybrid HTTP/WebSocket Server" << std::endl;
    std::cout << "=========================================" << std::endl;
    
    // Create server socket
    Socket serverSocket;
    if (!serverSocket.Create(SOCKET_FAMILY::IPV4, SOCKET_TYPE::TCP).IsSuccess()) {
        std::cout << "❌ Failed to create server socket" << std::endl;
        return 1;
    }
    
    // Configure server for non-blocking operation
    serverSocket.ReuseAddress(true);
    
    // Set server to non-blocking mode
    if (!serverSocket.Blocking(false).IsSuccess()) {
        std::cout << "❌ Failed to set server to non-blocking mode" << std::endl;
        return 1;
    }
    
    if (!serverSocket.Bind("127.0.0.1", 8080).IsSuccess()) {
        std::cout << "❌ Failed to bind server socket" << std::endl;
        return 1;
    }
    
    if (!serverSocket.Listen(10).IsSuccess()) {
        std::cout << "❌ Failed to start listening" << std::endl;
        return 1;
    }
    
    std::cout << "🚀 Non-blocking hybrid server listening on 127.0.0.1:8080" << std::endl;
    std::cout << "📝 Single-threaded, non-blocking operation" << std::endl;
    std::cout << "🔄 Press Ctrl+C to stop" << std::endl << std::endl;
    
    // Client management
    std::map<int, ClientInfo> clients;
    int nextClientId = 1;
    
    // Main event loop - completely non-blocking
    while (true) {
        bool hasActivity = false;
        
        // 1. Accept new connections (non-blocking)
        AcceptResult acceptResult = serverSocket.Accept();
        
        if (acceptResult.first.IsSuccess() && acceptResult.second) {
            // Set new client to non-blocking mode
            if (acceptResult.second->Blocking(false).IsSuccess()) {
                int clientId = nextClientId++;
                clients.emplace(clientId, ClientInfo(std::move(acceptResult.second)));
                
                std::cout << "✅ Client " << clientId << " connected (Total: " << clients.size() << ")" << std::endl;
                hasActivity = true;
            } else {
                std::cout << "❌ Failed to set client to non-blocking mode" << std::endl;
                acceptResult.second->Close();
            }
        }
        // If accept failed with "would block" or "no connection pending", that's normal for non-blocking
        
        // 2. Process existing clients (non-blocking)
        auto it = clients.begin();
        while (it != clients.end()) {
            int clientId = it->first;
            ClientInfo& client = it->second;
            bool clientClosed = false;
            
            // Check for client timeout (30 seconds)
            auto now = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - client.lastActivity);
            if (elapsed.count() > 30) {
                std::cout << "⏰ Client " << clientId << " timed out, disconnecting" << std::endl;
                client.socket->Close();
                it = clients.erase(it);
                continue;
            }
            
            // Try to receive data (non-blocking)
            ReceiveResult receiveResult = client.socket->Receive(4096);
            
            if (receiveResult.first.IsSuccess()) {
                if (!receiveResult.second.empty()) {
                    // Data received
                    client.receiveBuffer.append(receiveResult.second.begin(), receiveResult.second.end());
                    client.lastActivity = now;
                    hasActivity = true;
                    
                    std::cout << "📨 Client " << clientId << " sent " << receiveResult.second.size() << " bytes" << std::endl;
                    
                    // Process based on current state
                    if (client.state == ClientState::CONNECTED || client.state == ClientState::RECEIVING) {
                        // Check if we have enough data to determine protocol
                        if (client.receiveBuffer.find("\r\n\r\n") != std::string::npos) {
                            // Complete headers received
                            if (IsWebSocketUpgrade(client.receiveBuffer)) {
                                client.state = ClientState::WEBSOCKET_HANDSHAKE;
                                client.isWebSocket = true;
                                std::cout << "🔌 Client " << clientId << " requesting WebSocket upgrade" << std::endl;
                            } else {
                                client.state = ClientState::HTTP_PROCESSING;
                                std::cout << "🌐 Client " << clientId << " sending HTTP request" << std::endl;
                            }
                        }
                    }
                    
                    // Handle WebSocket handshake
                    if (client.state == ClientState::WEBSOCKET_HANDSHAKE) {
                        HandshakeInfo handshakeInfo;
                        Result handshakeResult = WebSocketProtocol::ValidateHandshakeRequest(client.receiveBuffer, handshakeInfo);
                        
                        if (handshakeResult.IsSuccess()) {
                            std::string response = WebSocketProtocol::GenerateHandshakeResponse(handshakeInfo);
                            auto sendResult = client.socket->Send(std::vector<uint8_t>(response.begin(), response.end()));
                            
                            if (sendResult.IsSuccess()) {
                                client.state = ClientState::WEBSOCKET_ESTABLISHED;
                                std::cout << "🤝 Client " << clientId << " WebSocket handshake completed" << std::endl;
                                
                                // Send welcome message
                                WebSocketFrame welcomeFrame = WebSocketProtocol::CreateTextFrame("Welcome to non-blocking WebSocket server!");
                                std::vector<uint8_t> welcomeData = WebSocketProtocol::GenerateFrame(welcomeFrame);
                                client.socket->Send(welcomeData);
                            }
                        }
                    }
                    
                    // Handle HTTP request
                    else if (client.state == ClientState::HTTP_PROCESSING) {
                        std::string method, path;
                        if (ParseHTTPRequest(client.receiveBuffer, method, path)) {
                            std::string response;
                            if (path == "/") {
                                std::string body = "<html><body><h1>Non-Blocking Hybrid Server</h1><p>This server is completely non-blocking!</p><p>Clients: " + 
                                                std::to_string(clients.size()) + "</p></body></html>";
                                response = GenerateHTTPResponse("200 OK", "text/html", body);
                            } else if (path == "/api") {
                                std::string body = "{\"message\": \"Non-blocking HTTP API!\", \"clients\": " + std::to_string(clients.size()) + "}";
                                response = GenerateHTTPResponse("200 OK", "application/json", body);
                            } else {
                                std::string body = "404 Not Found";
                                response = GenerateHTTPResponse("404 Not Found", "text/plain", body);
                            }
                            
                            auto sendResult = client.socket->Send(std::vector<uint8_t>(response.begin(), response.end()));
                            if (sendResult.IsSuccess()) {
                                std::cout << "📤 Client " << clientId << " HTTP response sent" << std::endl;
                                client.state = ClientState::CLOSING;
                            }
                        }
                    }
                    
                    // Handle WebSocket frames
                    else if (client.state == ClientState::WEBSOCKET_ESTABLISHED) {
                        // Parse WebSocket frames
                        size_t bytesConsumed = 0;
                        WebSocketFrame frame;
                        Result parseResult = WebSocketProtocol::ParseFrame(
                            std::vector<uint8_t>(client.receiveBuffer.begin(), client.receiveBuffer.end()), 
                            frame, bytesConsumed
                        );
                        
                        if (parseResult.IsSuccess() && bytesConsumed > 0) {
                            // Remove processed data
                            client.receiveBuffer.erase(0, bytesConsumed);
                            
                            if (frame.Opcode == WEBSOCKET_OPCODE::TEXT) {
                                std::string message(frame.PayloadData.begin(), frame.PayloadData.end());
                                std::cout << "💬 Client " << clientId << " WebSocket message: \"" << message << "\"" << std::endl;
                                
                                // Echo back with prefix
                                std::string echo = "Server echo: " + message;
                                WebSocketFrame responseFrame = WebSocketProtocol::CreateTextFrame(echo);
                                std::vector<uint8_t> responseData = WebSocketProtocol::GenerateFrame(responseFrame);
                                client.socket->Send(responseData);
                            }
                        }
                    }
                }
            } else {
                // Check if connection was closed
                if (receiveResult.first.GetErrorCode() == ERROR_CODE::SOCKET_RECEIVE_FAILED) {
                    std::cout << "🔌 Client " << clientId << " disconnected" << std::endl;
                    client.socket->Close();
                    clientClosed = true;
                }
                // Other errors are ignored for non-blocking (would block, etc.)
            }
            
            // Close clients that are done
            if (client.state == ClientState::CLOSING || clientClosed) {
                it = clients.erase(it);
            } else {
                ++it;
            }
        }
        
        // 3. Small sleep to prevent CPU spinning (only if no activity)
        if (!hasActivity) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        
        // Periodic status update
        static int statusCounter = 0;
        if (++statusCounter >= 5000) { // Every ~5 seconds
            std::cout << "📊 Status: " << clients.size() << " active clients" << std::endl;
            statusCounter = 0;
        }
    }
    
    // Clean up (never reached in this infinite loop example)
    serverSocket.Close();
    return 0;
}
