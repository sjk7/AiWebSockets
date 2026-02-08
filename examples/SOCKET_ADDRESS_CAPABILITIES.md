# Socket Address Capabilities - Complete Guide

## 🎯 Your Questions Answered

### **1. Server: Can we get the remote address (foreign IP) for every socket connected?**

**✅ YES!** Multiple ways available:

#### **Low-Level Socket API:**
```cpp
// After accepting a connection
auto acceptResult = serverSocket.Accept();
if (acceptResult.first.IsSuccess() && acceptResult.second) {
    std::string clientIP = acceptResult.second->RemoteAddress();  // Foreign IP
    uint16_t clientPort = acceptResult.second->RemotePort();      // Foreign Port
    
    std::cout << "Client connected from: " << clientIP << ":" << clientPort << std::endl;
}
```

#### **WebSocket Lite API:**
```cpp
server.OnConnect([](const std::string& clientIP) {
    std::cout << "Client connected: " << clientIP << std::endl;
    // Enhanced with proxy detection:
    // - X-Forwarded-For header parsing
    // - X-Real-IP header support  
    // - Falls back to socket.RemoteAddress()
});
```

#### **Enhanced Proxy-Aware Detection:**
```cpp
// Gets real client IP even behind proxies
std::string realIP = GetClientIP(socket, httpRequest);
// Priority: X-Forwarded-For → X-Real-IP → socket.RemoteAddress()
```

---

### **2. Client: Can we get both the remote address and our local IP and port?**

**✅ YES!** Full address information available:

#### **Low-Level Socket API:**
```cpp
Socket clientSocket;
clientSocket.Create(SOCKET_FAMILY::IPV4, SOCKET_TYPE::TCP);
clientSocket.Connect("127.0.0.1", 8080);

// Local address (your side)
std::string localIP = clientSocket.LocalAddress();  // Your IP
uint16_t localPort = clientSocket.LocalPort();      // Your port

// Remote address (server side)  
std::string remoteIP = clientSocket.RemoteAddress(); // Server IP
uint16_t remotePort = clientSocket.RemotePort();    // Server port

std::cout << "Local: " << localIP << ":" << localPort << std::endl;
std::cout << "Remote: " << remoteIP << ":" << remotePort << std::endl;
```

#### **WebSocket Lite API:**
```cpp
WebSocketClientLite client("127.0.0.1", 8080);
client.Connect();

// WebSocket Lite abstracts socket details for simplicity
// But you can extend it to expose socket info if needed
```

---

### **3. Server: Can we enumerate and choose an interface if we want, but default to all?**

**✅ YES!** Full interface control available:

#### **Interface Binding Options:**
```cpp
// Option 1: Bind to all interfaces (default for most servers)
serverSocket.Bind("0.0.0.0", 8080);  // INADDR_ANY - all network interfaces

// Option 2: Bind to localhost only
serverSocket.Bind("127.0.0.1", 8080);  // Local connections only

// Option 3: Bind to specific interface
serverSocket.Bind("192.168.1.100", 8080);  // Specific network interface

// Option 4: Let system choose (binds to 0.0.0.0)
serverSocket.Bind("", 8080);  // Empty string = 0.0.0.0
```

#### **WebSocket Lite API:**
```cpp
WebSocketServerLite server;

// Configure binding (optional - defaults to 127.0.0.1:8080)
server.SetBindAddress("0.0.0.0")  // All interfaces
      .SetPort(8080);

server.Start();
```

#### **Interface Detection:**
```cpp
// Check if port is available on specific interface
bool available = Socket::IsPortAvailable(8080, "127.0.0.1");
bool allInterfacesAvailable = Socket::IsPortAvailable(8080, "0.0.0.0");
```

---

## 🔧 Available Socket Methods

### **Address Information Methods:**
```cpp
// All available in Socket class
std::string LocalAddress() const;    // Your local IP
uint16_t LocalPort() const;          // Your local port
std::string RemoteAddress() const;   // Remote/foreign IP  
uint16_t RemotePort() const;         // Remote/foreign port
```

### **Binding Methods:**
```cpp
Result Bind(const std::string& address, uint16_t port);
// address options: "127.0.0.1", "0.0.0.0", "192.168.1.x", or ""
```

### **Utility Methods:**
```cpp
static bool IsPortAvailable(uint16_t port, const std::string& address);
static bool IsIPAddress(const std::string& address);
```

---

## 📋 Complete Examples

### **Server Example - Full Address Tracking:**
```cpp
void demonstrateServerAddresses() {
    Socket serverSocket;
    serverSocket.Create(SOCKET_FAMILY::IPV4, SOCKET_TYPE::TCP);
    
    // Bind to all interfaces
    serverSocket.Bind("0.0.0.0", 8080);
    serverSocket.Listen(5);
    
    std::cout << "Server listening on: " 
              << serverSocket.LocalAddress() << ":" 
              << serverSocket.LocalPort() << std::endl;
    
    while (true) {
        auto acceptResult = serverSocket.Accept();
        if (acceptResult.first.IsSuccess() && acceptResult.second) {
            // Get client's foreign address
            std::string clientIP = acceptResult.second->RemoteAddress();
            uint16_t clientPort = acceptResult.second->RemotePort();
            
            // Get connection details from client's perspective
            std::string clientLocalIP = acceptResult.second->LocalAddress();
            uint16_t clientLocalPort = acceptResult.second->LocalPort();
            
            std::cout << "Client from " << clientIP << ":" << clientPort
                     << " connected to " << clientLocalIP << ":" << clientLocalPort << std::endl;
        }
    }
}
```

### **Client Example - Full Address Information:**
```cpp
void demonstrateClientAddresses() {
    Socket clientSocket;
    clientSocket.Create(SOCKET_FAMILY::IPV4, SOCKET_TYPE::TCP);
    
    // Address info before connection
    std::cout << "Before connect - Local: " 
              << clientSocket.LocalAddress() << ":" 
              << clientSocket.LocalPort() << std::endl;
    
    clientSocket.Connect("127.0.0.1", 8080);
    
    // Full address info after connection
    std::cout << "Local: " << clientSocket.LocalAddress() << ":" 
              << clientSocket.LocalPort() << std::endl;
    std::cout << "Remote: " << clientSocket.RemoteAddress() << ":" 
              << clientSocket.RemotePort() << std::endl;
}
```

### **Interface Selection Example:**
```cpp
void demonstrateInterfaceBinding() {
    std::vector<std::string> interfaces = {
        "127.0.0.1",    // Loopback only
        "0.0.0.0",      // All interfaces  
        "192.168.1.100" // Specific interface
    };
    
    for (const auto& iface : interfaces) {
        Socket testSocket;
        testSocket.Create(SOCKET_FAMILY::IPV4, SOCKET_TYPE::TCP);
        
        auto bindResult = testSocket.Bind(iface, 0); // Port 0 = system chooses
        if (bindResult.IsSuccess()) {
            std::cout << "Successfully bound to " << iface 
                     << " on port " << testSocket.LocalPort() << std::endl;
        }
        testSocket.Close();
    }
}
```

---

## 🎯 Summary - All Your Questions Answered

### **✅ Server - Foreign IP for Every Connection:**
- **Low-level**: `socket->RemoteAddress()` and `socket->RemotePort()`
- **WebSocket Lite**: `OnConnect` callback with proxy-aware IP detection
- **Enhanced**: X-Forwarded-For and X-Real-IP header parsing

### **✅ Client - Local and Remote Address:**
- **Local**: `socket.LocalAddress()` and `socket.LocalPort()`
- **Remote**: `socket.RemoteAddress()` and `socket.RemotePort()`
- **Available**: Before and after connection

### **✅ Server - Interface Selection:**
- **All interfaces**: `"0.0.0.0"` (INADDR_ANY)
- **Local only**: `"127.0.0.1"`  
- **Specific**: `"192.168.1.x"` or any valid interface IP
- **Default**: WebSocket Lite defaults to `"127.0.0.1:8080"`

---

## 🚀 Ready to Use

All these capabilities are:
- ✅ **Fully implemented** in the current codebase
- ✅ **Cross-platform** (Windows/Linux/macOS)
- ✅ **Production ready** with error handling
- ✅ **Available in both** low-level and WebSocket Lite APIs
- ✅ **Demonstrated in** working examples

**Run the examples to see everything in action:**
- `socket_address_examples.exe` - Low-level socket API demo
- `websocket_lite_address_info.exe` - High-level WebSocket Lite demo
