#include "WebSocket/WebSocketServerLite.h"
#include "WebSocket/Socket.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <vector>
#include <map>
#include <atomic>
#include <sstream>
#include <string>

using namespace WebSocket;

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
    
    MultiInterfaceServer() = default;
    ~MultiInterfaceServer() {
        Stop();
    }
    
    // Discover available interfaces and create configurations
    std::vector<ServerConfig> DiscoverServerConfigs() {
        std::vector<ServerConfig> configs;
        auto localIPs = Socket::GetLocalIPAddresses();
        
        std::cout << "🔍 Discovering server configurations..." << std::endl;
        
        // Always add localhost configuration
        configs.push_back({
            "Local Development Server",
            "127.0.0.1",
            8080,
            "Local connections only - for development and testing",
            true
        });
        
        // Add all-interfaces configuration
        configs.push_back({
            "Production Server (All Interfaces)",
            "0.0.0.0", 
            8081,
            "Accept connections from any network interface - for production",
            false  // Disabled by default for security
        });
        
        // Add configurations for each specific interface
        int interfaceCounter = 1;
        for (const auto& ip : localIPs) {
            // Skip loopback and already configured addresses
            if (ip == "127.0.0.1" || ip == "0.0.0.0") continue;
            if (ip.find("::") != std::string::npos) continue; // Skip IPv6 for this demo
            
            configs.push_back({
                "Interface Server " + std::to_string(interfaceCounter++),
                ip,
                static_cast<uint16_t>(8081 + interfaceCounter),  // Different port per interface
                "Server bound to specific interface: " + ip,
                false  // Disabled by default
            });
        }
        
        return configs;
    }
    
    // Start servers based on selected configurations
    Result StartServers(const std::vector<ServerConfig>& selectedConfigs) {
        std::cout << "🚀 Starting " << selectedConfigs.size() << " server(s)..." << std::endl;
        
        for (const auto& config : selectedConfigs) {
            if (!config.enabled) {
                std::cout << "⏭️  Skipping disabled: " << config.name << std::endl;
                continue;
            }
            
            // Check if port is available
            if (!Socket::IsPortAvailable(config.port, config.bindAddress)) {
                std::cout << "❌ Port " << config.port << " not available on " << config.bindAddress 
                         << " for " << config.name << std::endl;
                continue;
            }
            
            // Create server
            auto server = std::make_unique<WebSocketServerLite>();
            server->SetPort(config.port)
                   .SetBindAddress(config.bindAddress)
                   .EnableSecurity(true)
                   .SetMaxConnections(10);
            
            // Set up event handlers with server identification
            std::string serverId = config.name + " (" + config.bindAddress + ":" + std::to_string(config.port) + ")";
            
            server->OnConnect([serverId](const std::string& clientIP) {
                std::cout << "🔗 [" << serverId << "] Client connected: " << clientIP << std::endl;
            });
            
            server->OnMessage([serverId](const std::string& message) {
                std::cout << "📨 [" << serverId << "] Received: " << message << std::endl;
                
                // Echo with server identification
                std::string response = "Echo from " + serverId + ": " + message;
                std::cout << "📤 [" << serverId << "] Sending: " << response << std::endl;
            });
            
            server->OnDisconnect([serverId](const std::string& clientIP) {
                std::cout << "🔌 [" << serverId << "] Client disconnected: " << clientIP << std::endl;
            });
            
            server->OnError([serverId](const Result& error) {
                std::cout << "❌ [" << serverId << "] Error: " << error.GetErrorMessage() << std::endl;
            });
            
            // Start the server
            auto startResult = server->Start();
            if (!startResult.IsSuccess()) {
                std::cout << "❌ Failed to start " << config.name << ": " 
                         << startResult.GetErrorMessage() << std::endl;
                continue;
            }
            
            std::cout << "✅ Started: " << config.name << " on " 
                     << config.bindAddress << ":" << config.port << std::endl;
            std::cout << "   📝 " << config.description << std::endl;
            
            // Store server
            servers[serverId] = std::move(server);
        }
        
        if (servers.empty()) {
            return Result(ERROR_CODE::INVALID_PARAMETER, "No servers were started successfully");
        }
        
        running = true;
        std::cout << "🎉 All servers started successfully!" << std::endl;
        return Result();
    }
    
    // Run all servers in separate threads
    void Run() {
        if (!running || servers.empty()) {
            std::cout << "❌ No servers running!" << std::endl;
            return;
        }
        
        std::cout << "🔄 Running " << servers.size() << " server(s)..." << std::endl;
        std::cout << "📊 Server Status:" << std::endl;
        
        for (const auto& pair : servers) {
            const auto& serverId = pair.first;
            const auto& server = pair.second;
            
            std::cout << "  📡 " << serverId << " - " 
                     << server->GetCurrentConnectionCount() << " connections" << std::endl;
            
            // Start server event loop in separate thread
            serverThreads.emplace_back([this, serverId]() {
                auto& server = servers[serverId];
                while (running && server->IsRunning()) {
                    server->ProcessEvents();
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                }
            });
        }
        
        std::cout << "🔄 Servers running. Press Ctrl+C to stop." << std::endl;
        
        // Main monitoring loop
        int statusCounter = 0;
        while (running) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            
            // Show status every 10 seconds
            if (++statusCounter % 10 == 0) {
                std::cout << "\n📊 Server Status Update:" << std::endl;
                for (const auto& pair : servers) {
                    const auto& serverId = pair.first;
                    const auto& server = pair.second;
                    std::cout << "  📡 " << serverId << " - " 
                             << server->GetCurrentConnectionCount() << " connections" << std::endl;
                }
                std::cout << std::endl;
            }
        }
    }
    
    void Stop() {
        if (!running) return;
        
        std::cout << "\n🛑 Stopping all servers..." << std::endl;
        running = false;
        
        // Stop all servers
        for (auto& pair : servers) {
            const auto& serverId = pair.first;
            auto& server = pair.second;
            
            std::cout << "🛑 Stopping: " << serverId << std::endl;
            server->Stop();
        }
        
        // Wait for all threads to finish
        for (auto& thread : serverThreads) {
            if (thread.joinable()) {
                thread.join();
            }
        }
        
        serverThreads.clear();
        servers.clear();
        
        std::cout << "✅ All servers stopped." << std::endl;
    }
    
    size_t GetServerCount() const {
        return servers.size();
    }
    
    std::vector<std::string> GetServerIds() const {
        std::vector<std::string> ids;
        for (const auto& pair : servers) {
            ids.push_back(pair.first);
        }
        return ids;
    }
};

// Test functions
class MultiInterfaceServerTest {
public:
    static bool runAllTests() {
        std::cout << "🧪 Running Multi-Interface Server Tests..." << std::endl;
        
        bool allPassed = true;
        
        allPassed &= TestIPEnumeration();
        allPassed &= TestPortAvailability();
        allPassed &= TestConfigurationGeneration();
        allPassed &= TestServerLifecycle();
        
        std::cout << "\n📊 Test Results: " << (allPassed ? "✅ ALL PASSED" : "❌ SOME FAILED") << std::endl;
        return allPassed;
    }
    
private:
    static bool TestIPEnumeration() {
        std::cout << "\n🧪 Test 1: IP Enumeration" << std::endl;
        
        try {
            auto ips = Socket::GetLocalIPAddresses();
            
            if (ips.empty()) {
                std::cout << "❌ No IP addresses found!" << std::endl;
                return false;
            }
            
            std::cout << "✅ Found " << ips.size() << " IP addresses" << std::endl;
            for (const auto& ip : ips) {
                std::cout << "  📍 " << ip << std::endl;
            }
            
            // Check if we have any valid IP (not just empty strings)
            bool hasValidIP = false;
            for (const auto& ip : ips) {
                if (!ip.empty() && ip != "0.0.0.0") {
                    hasValidIP = true;
                    break;
                }
            }
            
            if (!hasValidIP) {
                std::cout << "❌ No valid IP addresses found!" << std::endl;
                return false;
            }
            
            return true;
        } catch (const std::exception& e) {
            std::cout << "❌ Exception: " << e.what() << std::endl;
            return false;
        }
    }
    
    static bool TestPortAvailability() {
        std::cout << "\n🧪 Test 2: Port Availability" << std::endl;
        
        try {
            auto ips = Socket::GetLocalIPAddresses();
            std::vector<uint16_t> testPorts = {8080, 8081, 8082};
            
            for (const auto& ip : ips) {
                if (ip.find("::") != std::string::npos) continue; // Skip IPv6
                
                std::cout << "🔍 Testing ports on " << ip << ":" << std::endl;
                
                for (uint16_t port : testPorts) {
                    bool available = Socket::IsPortAvailable(port, ip);
                    std::cout << "  Port " << port << ": " 
                              << (available ? "✅ Available" : "❌ In use") << std::endl;
                }
            }
            
            return true;
        } catch (const std::exception& e) {
            std::cout << "❌ Exception: " << e.what() << std::endl;
            return false;
        }
    }
    
    static bool TestConfigurationGeneration() {
        std::cout << "\n🧪 Test 3: Configuration Generation" << std::endl;
        
        try {
            MultiInterfaceServer serverManager;
            auto configs = serverManager.DiscoverServerConfigs();
            
            if (configs.empty()) {
                std::cout << "❌ No configurations generated!" << std::endl;
                return false;
            }
            
            // Should have at least localhost config
            bool hasLocalhost = false;
            for (const auto& config : configs) {
                if (config.bindAddress == "127.0.0.1") {
                    hasLocalhost = true;
                    break;
                }
            }
            
            if (!hasLocalhost) {
                std::cout << "❌ No localhost configuration found!" << std::endl;
                return false;
            }
            
            std::cout << "✅ Generated " << configs.size() << " configurations:" << std::endl;
            for (const auto& config : configs) {
                std::cout << "  📋 " << config.name << std::endl;
                std::cout << "     Address: " << config.bindAddress << ":" << config.port << std::endl;
                std::cout << "     Enabled: " << (config.enabled ? "Yes" : "No") << std::endl;
                std::cout << "     " << config.description << std::endl;
            }
            
            return true;
        } catch (const std::exception& e) {
            std::cout << "❌ Exception: " << e.what() << std::endl;
            return false;
        }
    }
    
    static bool TestServerLifecycle() {
        std::cout << "\n🧪 Test 4: Server Lifecycle" << std::endl;
        
        try {
            MultiInterfaceServer serverManager;
            auto configs = serverManager.DiscoverServerConfigs();
            
            // Find an available IP and port for testing
            auto ips = Socket::GetLocalIPAddresses();
            std::string testIP;
            for (const auto& ip : ips) {
                if (ip.find("::") == std::string::npos && !ip.empty()) {
                    testIP = ip;
                    break;
                }
            }
            
            if (testIP.empty()) {
                std::cout << "❌ No suitable IP found for testing!" << std::endl;
                return false;
            }
            
            // Find an available port
            uint16_t testPort = 0;
            for (uint16_t port = 9000; port < 9100; ++port) {
                if (Socket::IsPortAvailable(port, testIP)) {
                    testPort = port;
                    break;
                }
            }
            
            if (testPort == 0) {
                std::cout << "❌ No available port found for testing!" << std::endl;
                return false;
            }
            
            // Create a test configuration
            std::vector<MultiInterfaceServer::ServerConfig> testConfigs;
            testConfigs.push_back({
                "Test Server",
                testIP,
                testPort,
                "Test server for lifecycle validation",
                true
            });
            
            std::cout << "🔧 Using test configuration: " << testIP << ":" << testPort << std::endl;
            
            // Start servers
            auto startResult = serverManager.StartServers(testConfigs);
            if (!startResult.IsSuccess()) {
                std::cout << "❌ Failed to start servers: " << startResult.GetErrorMessage() << std::endl;
                return false;
            }
            
            if (serverManager.GetServerCount() == 0) {
                std::cout << "❌ No servers started!" << std::endl;
                return false;
            }
            
            std::cout << "✅ Started " << serverManager.GetServerCount() << " server(s)" << std::endl;
            
            // Run briefly to test
            std::thread serverThread([&serverManager]() {
                serverManager.Run();
            });
            
            // Let it run for 2 seconds
            std::this_thread::sleep_for(std::chrono::seconds(2));
            
            // Stop servers
            serverManager.Stop();
            
            if (serverThread.joinable()) {
                serverThread.join();
            }
            
            std::cout << "✅ Server lifecycle test completed" << std::endl;
            return true;
            
        } catch (const std::exception& e) {
            std::cout << "❌ Exception: " << e.what() << std::endl;
            return false;
        }
    }
};

void printUsage() {
    std::cout << "🚀 Multi-Interface WebSocket Server" << std::endl;
    std::cout << "===================================" << std::endl;
    std::cout << "Usage:" << std::endl;
    std::cout << "  multi_interface_server.exe [mode]" << std::endl;
    std::cout << std::endl;
    std::cout << "Modes:" << std::endl;
    std::cout << "  test     - Run all tests and exit" << std::endl;
    std::cout << "  demo     - Run demo with user interaction" << std::endl;
    std::cout << "  auto     - Auto-start with localhost only" << std::endl;
    std::cout << std::endl;
    std::cout << "Examples:" << std::endl;
    std::cout << "  multi_interface_server.exe test" << std::endl;
    std::cout << "  multi_interface_server.exe demo" << std::endl;
    std::cout << "  multi_interface_server.exe auto" << std::endl;
}

int main(int argc, char* argv[]) {
    std::string mode = (argc > 1) ? argv[1] : "demo";
    
    if (mode == "help" || mode == "--help" || mode == "-h") {
        printUsage();
        return 0;
    }
    
    if (mode == "test") {
        // Run tests only
        bool testsPassed = MultiInterfaceServerTest::runAllTests();
        return testsPassed ? 0 : 1;
    }
    
    if (mode == "demo") {
        // Interactive demo
        std::cout << "🚀 Multi-Interface WebSocket Server Demo" << std::endl;
        std::cout << "========================================" << std::endl;
        
        MultiInterfaceServer serverManager;
        auto configs = serverManager.DiscoverServerConfigs();
        
        std::cout << "\n📋 Available Server Configurations:" << std::endl;
        for (size_t i = 0; i < configs.size(); ++i) {
            const auto& config = configs[i];
            std::cout << "  " << (i + 1) << ". " << config.name << std::endl;
            std::cout << "     Address: " << config.bindAddress << ":" << config.port << std::endl;
            std::cout << "     " << config.description << std::endl;
            std::cout << "     Currently: " << (config.enabled ? "Enabled" : "Disabled") << std::endl;
            std::cout << std::endl;
        }
        
        std::cout << "Select servers to start (e.g., '1', '1,3', 'all', or 'localhost'):" << std::endl;
        std::string input;
        std::getline(std::cin, input);
        
        // Parse user input and enable selected configurations
        if (input == "all") {
            for (auto& config : configs) {
                config.enabled = true;
            }
        } else if (input == "localhost") {
            for (auto& config : configs) {
                config.enabled = (config.bindAddress == "127.0.0.1");
            }
        } else {
            // Parse comma-separated numbers
            std::stringstream ss(input);
            std::string token;
            while (std::getline(ss, token, ',')) {
                try {
                    int index = std::stoi(token) - 1;
                    if (index >= 0 && index < static_cast<int>(configs.size())) {
                        configs[index].enabled = true;
                    }
                } catch (...) {
                    // Ignore invalid input
                }
            }
        }
        
        // Start selected servers
        auto startResult = serverManager.StartServers(configs);
        if (!startResult.IsSuccess()) {
            std::cout << "❌ Failed to start servers: " << startResult.GetErrorMessage() << std::endl;
            return 1;
        }
        
        // Run servers
        serverManager.Run();
        
        return 0;
    }
    
    if (mode == "auto") {
        // Auto-start with localhost only
        std::cout << "🚀 Auto-Starting Localhost Server" << std::endl;
        std::cout << "================================" << std::endl;
        
        MultiInterfaceServer serverManager;
        auto configs = serverManager.DiscoverServerConfigs();
        
        // Enable only localhost
        for (auto& config : configs) {
            config.enabled = (config.bindAddress == "127.0.0.1");
        }
        
        auto startResult = serverManager.StartServers(configs);
        if (!startResult.IsSuccess()) {
            std::cout << "❌ Failed to start server: " << startResult.GetErrorMessage() << std::endl;
            return 1;
        }
        
        serverManager.Run();
        return 0;
    }
    
    std::cout << "❌ Unknown mode: " << mode << std::endl;
    printUsage();
    return 1;
}
