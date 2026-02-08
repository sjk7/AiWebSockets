#include "WebSocket/Socket.h"
#include <iostream>
#include <thread>
#include <chrono>

using namespace WebSocket;

void demonstrateServerSocketAddresses() {
    std::cout << "=== SERVER: Socket Address Information ===" << std::endl;
    
    // Create server socket
    Socket serverSocket;
    auto createResult = serverSocket.Create(SOCKET_FAMILY::IPV4, SOCKET_TYPE::TCP);
    if (!createResult.IsSuccess()) {
        std::cout << "❌ Failed to create server socket: " << createResult.GetErrorMessage() << std::endl;
        return;
    }
    
    // Bind to specific interface (you can change this)
    std::string bindAddress = "127.0.0.1";  // Change to "0.0.0.0" for all interfaces
    uint16_t bindPort = 8080;
    
    std::cout << "🔗 Binding server to: " << bindAddress << ":" << bindPort << std::endl;
    auto bindResult = serverSocket.Bind(bindAddress, bindPort);
    if (!bindResult.IsSuccess()) {
        std::cout << "❌ Failed to bind: " << bindResult.GetErrorMessage() << std::endl;
        return;
    }
    
    // Get server's local address info
    std::cout << "📍 Server Local Address: " << serverSocket.LocalAddress() << std::endl;
    std::cout << "📍 Server Local Port: " << serverSocket.LocalPort() << std::endl;
    
    // Start listening
    auto listenResult = serverSocket.Listen(5);
    if (!listenResult.IsSuccess()) {
        std::cout << "❌ Failed to listen: " << listenResult.GetErrorMessage() << std::endl;
        return;
    }
    
    std::cout << "👂 Server listening for connections..." << std::endl;
    
    // Accept a connection (non-blocking attempt)
    auto acceptResult = serverSocket.Accept();
    if (acceptResult.first.IsSuccess() && acceptResult.second) {
        std::cout << "✅ Client connected!" << std::endl;
        
        // Get client's remote address info (foreign IP)
        std::cout << "🌍 Client Remote Address: " << acceptResult.second->RemoteAddress() << std::endl;
        std::cout << "🌍 Client Remote Port: " << acceptResult.second->RemotePort() << std::endl;
        
        // Get client's local address info (from server's perspective)
        std::cout << "📍 Client Local Address: " << acceptResult.second->LocalAddress() << std::endl;
        std::cout << "📍 Client Local Port: " << acceptResult.second->LocalPort() << std::endl;
        
        acceptResult.second->Close();
    } else {
        std::cout << "⏳ No client connection available (this is expected)" << std::endl;
    }
    
    serverSocket.Close();
}

void demonstrateClientSocketAddresses() {
    std::cout << "\n=== CLIENT: Socket Address Information ===" << std::endl;
    
    // Create client socket
    Socket clientSocket;
    auto createResult = clientSocket.Create(SOCKET_FAMILY::IPV4, SOCKET_TYPE::TCP);
    if (!createResult.IsSuccess()) {
        std::cout << "❌ Failed to create client socket: " << createResult.GetErrorMessage() << std::endl;
        return;
    }
    
    std::cout << "🔗 Created client socket" << std::endl;
    
    // Before connection - local info should be available
    std::cout << "📍 Client Local Address (pre-connect): " << clientSocket.LocalAddress() << std::endl;
    std::cout << "📍 Client Local Port (pre-connect): " << clientSocket.LocalPort() << std::endl;
    
    // Try to connect to server
    std::cout << "🔗 Connecting to 127.0.0.1:8080..." << std::endl;
    auto connectResult = clientSocket.Connect("127.0.0.1", 8080);
    if (!connectResult.IsSuccess()) {
        std::cout << "⚠️ Connection failed (expected if server not running): " << connectResult.GetErrorMessage() << std::endl;
    } else {
        std::cout << "✅ Connected successfully!" << std::endl;
        
        // After connection - all address info should be available
        std::cout << "📍 Client Local Address: " << clientSocket.LocalAddress() << std::endl;
        std::cout << "📍 Client Local Port: " << clientSocket.LocalPort() << std::endl;
        std::cout << "🌍 Client Remote Address: " << clientSocket.RemoteAddress() << std::endl;
        std::cout << "🌍 Client Remote Port: " << clientSocket.RemotePort() << std::endl;
    }
    
    clientSocket.Close();
}

void demonstrateInterfaceBinding() {
    std::cout << "\n=== INTERFACE BINDING OPTIONS ===" << std::endl;
    
    struct InterfaceOption {
        std::string name;
        std::string address;
        std::string description;
    };
    
    std::vector<InterfaceOption> interfaces = {
        {"Loopback", "127.0.0.1", "Local connections only"},
        {"All Interfaces", "0.0.0.0", "Accept connections on all network interfaces"},
        {"Local Network", "192.168.1.100", "Specific network interface (example)"},
        {"Any Available", "", "Let system choose (binds to 0.0.0.0)"}
    };
    
    std::cout << "Available server binding options:" << std::endl;
    for (const auto& iface : interfaces) {
        std::cout << "  📡 " << iface.name << ": " << iface.address << " (" << iface.description << ")" << std::endl;
        
        // Demonstrate binding to each interface
        Socket testSocket;
        auto createResult = testSocket.Create(SOCKET_FAMILY::IPV4, SOCKET_TYPE::TCP);
        if (createResult.IsSuccess()) {
            auto bindResult = testSocket.Bind(iface.address, 0); // Port 0 = system assigns
            if (bindResult.IsSuccess()) {
                std::cout << "    ✅ Successfully bound to " << iface.address 
                         << " on port " << testSocket.LocalPort() << std::endl;
            } else {
                std::cout << "    ❌ Failed to bind to " << iface.address 
                         << ": " << bindResult.GetErrorMessage() << std::endl;
            }
            testSocket.Close();
        }
    }
}

void demonstratePortAvailability() {
    std::cout << "\n=== PORT AVAILABILITY CHECKING ===" << std::endl;
    
    std::vector<uint16_t> testPorts = {8080, 8081, 8082, 443, 80, 22};
    std::vector<std::string> testAddresses = {"127.0.0.1", "0.0.0.0"};
    
    for (const auto& address : testAddresses) {
        std::cout << "🔍 Checking ports on " << address << ":" << std::endl;
        for (uint16_t port : testPorts) {
            bool available = Socket::IsPortAvailable(port, address);
            std::cout << "  Port " << port << ": " << (available ? "✅ Available" : "❌ In use") << std::endl;
        }
    }
}

int main() {
    std::cout << "🚀 Socket Address Information Demonstration" << std::endl;
    std::cout << "=============================================" << std::endl;
    
    try {
        // Demonstrate server socket addresses
        demonstrateServerSocketAddresses();
        
        // Demonstrate client socket addresses  
        demonstrateClientSocketAddresses();
        
        // Demonstrate interface binding options
        demonstrateInterfaceBinding();
        
        // Demonstrate port availability checking
        demonstratePortAvailability();
        
        std::cout << "\n✅ All demonstrations completed!" << std::endl;
        
    } catch (const std::exception& e) {
        std::cout << "❌ Exception: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
