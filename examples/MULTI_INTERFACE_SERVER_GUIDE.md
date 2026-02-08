# Multi-Interface Server Example - Complete Guide

## 🎯 Overview

This example demonstrates how to use the `Socket::GetLocalIPAddresses()` method to create WebSocket servers on multiple network interfaces with comprehensive testing.

## 🚀 Features Implemented

### **Core Functionality:**
- ✅ **IP Enumeration** - Discover all local network interfaces
- ✅ **Multi-Interface Servers** - Create servers on different IPs
- ✅ **Port Availability Checking** - Avoid port conflicts
- ✅ **Configuration Management** - Flexible server setup
- ✅ **Comprehensive Testing** - Full test suite validation

### **Server Management:**
- ✅ **Automatic Discovery** - Enumerate available interfaces
- ✅ **Dynamic Configuration** - Generate server configs automatically
- ✅ **Concurrent Operation** - Run multiple servers simultaneously
- ✅ **Graceful Shutdown** - Clean server lifecycle management
- ✅ **Status Monitoring** - Real-time connection tracking

---

## 📋 Usage Examples

### **1. Run Tests Only**
```bash
multi_interface_server.exe test
```

**Output:**
```
🧪 Running Multi-Interface Server Tests...
🧪 Test 1: IP Enumeration
✅ Found 4 IP addresses
  📍 192.168.1.101
  📍 fdc2:d184:4ec7:bc00:19e9:c566:334d:9e16
  📍 fdc2:d184:4ec7:bc00:e070:e792:a322:f14f
  📍 fe80::ec58:d2a8:f89:8abe

🧪 Test 2: Port Availability
🔍 Testing ports on 192.168.1.101:
  Port 8080: ✅ Available
  Port 8081: ✅ Available
  Port 8082: ✅ Available

🧪 Test 3: Configuration Generation
✅ Generated 5 configurations:
  📋 Local Development Server - 127.0.0.1:8080
  📋 Production Server (All Interfaces) - 0.0.0.0:8081
  📋 Interface Server 1 - 192.168.1.101:8083

🧪 Test 4: Server Lifecycle
✅ Started: Test Server on 192.168.1.101:9000
🔄 Running 1 server(s)...
📊 Server Status: 0 connections

📊 Test Results: ✅ ALL PASSED
```

### **2. Interactive Demo**
```bash
multi_interface_server.exe demo
```

**Features:**
- 📋 Lists all available server configurations
- 🔢 User selects which servers to start
- 🚀 Starts selected servers with status monitoring
- 🔄 Real-time connection tracking

### **3. Auto-Start Localhost**
```bash
multi_interface_server.exe auto
```

**Features:**
- 🚀 Automatically starts localhost server only
- 📡 Simple development setup
- 🔒 Secure local-only access

---

## 🔧 Architecture Overview

### **MultiInterfaceServer Class**
```cpp
class MultiInterfaceServer {
private:
    std::map<std::string, std::unique_ptr<WebSocketServerLite>> servers;
    std::atomic<bool> running{false};
    std::vector<std::thread> serverThreads;
    
public:
    struct ServerConfig {
        std::string name;
        std::string bindAddress;
        uint16_t port;
        std::string description;
        bool enabled;
    };
    
    // Core methods
    std::vector<ServerConfig> DiscoverServerConfigs();
    Result StartServers(const std::vector<ServerConfig>& selectedConfigs);
    void Run();
    void Stop();
};
```

### **Key Methods:**

#### **1. IP Discovery**
```cpp
auto configs = serverManager.DiscoverServerConfigs();
```

**Generates configurations for:**
- 🔄 Local Development Server (127.0.0.1:8080)
- 🌐 Production Server (0.0.0.0:8081) 
- 🎯 Interface-Specific Servers (one per local IP)

#### **2. Server Management**
```cpp
auto startResult = serverManager.StartServers(selectedConfigs);
serverManager.Run();
serverManager.Stop();
```

#### **3. Status Monitoring**
```cpp
size_t count = serverManager.GetServerCount();
auto serverIds = serverManager.GetServerIds();
```

---

## 🧪 Test Suite

### **Test 1: IP Enumeration**
- **Purpose:** Validate `Socket::GetLocalIPAddresses()` functionality
- **Validates:** IP discovery, duplicate removal, sorting
- **Expected:** At least one valid local IP found

### **Test 2: Port Availability**
- **Purpose:** Test `Socket::IsPortAvailable()` on multiple interfaces
- **Validates:** Port checking, IPv4/IPv6 handling
- **Expected:** Accurate availability reporting

### **Test 3: Configuration Generation**
- **Purpose:** Verify automatic server configuration creation
- **Validates:** Config generation, port assignment, descriptions
- **Expected:** Proper configuration structure

### **Test 4: Server Lifecycle**
- **Purpose:** End-to-end server creation and management
- **Validates:** Server start, event handling, graceful shutdown
- **Expected:** Successful server operation and cleanup

---

## 📊 Configuration Examples

### **Generated Configurations:**
```cpp
// Automatically generated based on local IPs
{
    {"Local Development Server", "127.0.0.1", 8080, "Local connections only", true},
    {"Production Server", "0.0.0.0", 8081, "All interfaces", false},
    {"Interface Server 1", "192.168.1.101", 8083, "Specific interface", false},
    {"Interface Server 2", "10.0.0.15", 8084, "Specific interface", false}
}
```

### **Custom Configuration:**
```cpp
std::vector<MultiInterfaceServer::ServerConfig> customConfigs;
customConfigs.push_back({
    "Custom Server",
    "192.168.1.100",
    9000,
    "Custom server for specific use case",
    true
});
```

---

## 🎯 Use Cases

### **1. Development Environment**
```bash
# Start localhost server for development
multi_interface_server.exe auto
```

### **2. Multi-NIC Production**
```bash
# Interactive selection for production deployment
multi_interface_server.exe demo
# Select: 1,3,4 (localhost + specific interfaces)
```

### **3. Testing & Validation**
```bash
# Run comprehensive test suite
multi_interface_server.exe test
```

### **4. Network Discovery**
```cpp
// Discover all network interfaces
auto ips = Socket::GetLocalIPAddresses();
for (const auto& ip : ips) {
    std::cout << "Interface: " << ip << std::endl;
}
```

---

## 🔍 Real-World Scenarios

### **Scenario 1: Development Machine**
```
Available IPs:
- 127.0.0.1 (localhost)
- 192.168.1.101 (WiFi)
- 10.0.0.15 (VPN)

Generated Servers:
- Local Development Server (127.0.0.1:8080)
- Interface Server 1 (192.168.1.101:8083)
- Interface Server 2 (10.0.0.15:8084)
```

### **Scenario 2: Production Server**
```
Available IPs:
- 127.0.0.1 (localhost)
- 203.0.113.100 (public)
- 192.168.1.100 (private)

Recommended Setup:
- Production Server (0.0.0.0:8081) - Accept all connections
- Or Interface Server (203.0.113.100:8083) - Public only
```

### **Scenario 3: Multi-Homed Host**
```
Available IPs:
- 127.0.0.1 (localhost)
- 192.168.1.50 (LAN)
- 10.0.0.25 (DMZ)
- 172.16.0.10 (Internal)

Flexible Deployment:
- Choose specific interface per service
- Different security policies per network
- Load balancing across interfaces
```

---

## 🛡️ Security Considerations

### **Binding Best Practices:**
```cpp
// ✅ Secure: Specific interface only
server.SetBindAddress("192.168.1.100");

// ⚠️  Caution: All interfaces (exposed)
server.SetBindAddress("0.0.0.0");

// ✅ Secure: Local development only
server.SetBindAddress("127.0.0.1");
```

### **Port Selection:**
- ✅ Use port availability checking
- ✅ Avoid well-known ports (80, 443, 22)
- ✅ Use different ports per interface
- ✅ Document port assignments

### **Access Control:**
- ✅ Enable security features
- ✅ Set connection limits
- ✅ Monitor connection activity
- ✅ Log connection attempts

---

## 📈 Performance Features

### **Concurrent Operation:**
```cpp
// Each server runs in its own thread
for (const auto& config : configs) {
    serverThreads.emplace_back([&server, config]() {
        while (running) {
            server.ProcessEvents();
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    });
}
```

### **Resource Management:**
- ✅ Automatic cleanup on shutdown
- ✅ Thread-safe operations
- ✅ Memory-efficient connection tracking
- ✅ Graceful error handling

### **Scalability:**
- ✅ Support for multiple interfaces
- ✅ Configurable connection limits
- ✅ Non-blocking event processing
- ✅ Efficient resource utilization

---

## ✅ Summary

### **What This Example Demonstrates:**

1. **🔍 IP Discovery:** `Socket::GetLocalIPAddresses()` usage
2. **🚀 Multi-Interface Servers:** Creating servers on different IPs
3. **🧪 Comprehensive Testing:** Full validation suite
4. **📊 Configuration Management:** Flexible server setup
5. **🔄 Concurrent Operation:** Multiple servers running simultaneously
6. **🛡️ Security Features:** Proper binding and access control

### **Key Benefits:**

- ✅ **Production Ready:** Full testing and validation
- ✅ **Flexible Deployment:** Choose interfaces dynamically
- ✅ **Developer Friendly:** Multiple operation modes
- ✅ **Well Documented:** Comprehensive examples and guides
- ✅ **Maintainable:** Clean architecture and error handling

### **Ready to Use:**

```bash
# Test everything
multi_interface_server.exe test

# Try interactive demo
multi_interface_server.exe demo

# Quick localhost server
multi_interface_server.exe auto
```

**This example provides a complete, production-ready solution for multi-interface WebSocket server deployment!** 🎉✅
