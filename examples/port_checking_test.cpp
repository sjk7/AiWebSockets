#include <iostream>
#include <thread>
#include <chrono>
#include "WebSocket/Socket.h"

using namespace WebSocket;

// Forward declaration
void TestWithSpecificPort(uint16_t testPort);

void TestPortAvailable() {
    std::cout << "🧪 Testing Port Availability Check" << std::endl;
    std::cout << "===================================" << std::endl;
    
    // Test 1: Check if port 8080 is initially available (positive case)
    std::cout << "\n📋 Test 1: Checking if port 8080 is available (should be available)" << std::endl;
    bool port8080Available = Socket::IsPortAvailable(8080, "127.0.0.1");
    if (port8080Available) {
        std::cout << "✅ PASS: Port 8080 is available" << std::endl;
    } else {
        std::cout << "❌ FAIL: Port 8080 should be available but reported as in use" << std::endl;
        std::cout << "💡 Make sure no other server is running on port 8080" << std::endl;
        
        // Let's try a different port that should definitely be available
        std::cout << "\n🔄 Trying alternative port 9999..." << std::endl;
        bool port9999Available = Socket::IsPortAvailable(9999, "127.0.0.1");
        if (port9999Available) {
            std::cout << "✅ Port 9999 is available - using it for tests" << std::endl;
            TestWithSpecificPort(9999);
            return;
        } else {
            std::cout << "❌ Even port 9999 is not available - there might be an issue with the test" << std::endl;
            return;
        }
    }
    
    TestWithSpecificPort(8080);
}

void TestWithSpecificPort(uint16_t testPort) {
    std::cout << "\n🔧 Running detailed tests with port " << testPort << std::endl;
    
    // Test 2: Bind to the port to make it unavailable
    std::cout << "\n📋 Test 2: Binding to port " << testPort << " to make it unavailable" << std::endl;
    Socket serverSocket;
    auto createResult = serverSocket.Create(SOCKET_FAMILY::IPV4, SOCKET_TYPE::TCP);
    if (!createResult.IsSuccess()) {
        std::cout << "❌ FAIL: Could not create test socket" << std::endl;
        return;
    }
    
    // Socket initialization is now automatic
    
    auto bindResult = serverSocket.Bind("127.0.0.1", testPort);
    if (!bindResult.IsSuccess()) {
        std::cout << "❌ FAIL: Could not bind to port " << testPort << ": " << bindResult.GetErrorMessage() << std::endl;
        return;
    }
    std::cout << "✅ Successfully bound to port " << testPort << std::endl;
    
    // Test 3: Check if the port is now unavailable (negative case)
    std::cout << "\n📋 Test 3: Checking if port " << testPort << " is available (should be in use)" << std::endl;
    bool portAvailableAfterBind = Socket::IsPortAvailable(testPort, "127.0.0.1");
    if (!portAvailableAfterBind) {
        std::cout << "✅ PASS: Port " << testPort << " correctly reported as in use" << std::endl;
    } else {
        std::cout << "❌ FAIL: Port " << testPort << " should be in use but reported as available" << std::endl;
    }
    
    // Test 4: Check if a different port is still available (positive case)
    uint16_t differentPort = testPort + 1;
    std::cout << "\n📋 Test 4: Checking if port " << differentPort << " is available (should be available)" << std::endl;
    bool differentPortAvailable = Socket::IsPortAvailable(differentPort, "127.0.0.1");
    if (differentPortAvailable) {
        std::cout << "✅ PASS: Port " << differentPort << " is available" << std::endl;
    } else {
        std::cout << "❌ FAIL: Port " << differentPort << " should be available but reported as in use" << std::endl;
    }
    
    // Test 5: Test invalid address
    std::cout << "\n📋 Test 5: Checking invalid address (should return false)" << std::endl;
    bool invalidAddress = Socket::IsPortAvailable(8082, "invalid.address");
    if (!invalidAddress) {
        std::cout << "✅ PASS: Invalid address correctly handled" << std::endl;
    } else {
        std::cout << "❌ FAIL: Invalid address should return false" << std::endl;
    }
    
    // Test 6: Test binding to the occupied port (should fail with enhanced error message)
    std::cout << "\n📋 Test 6: Testing enhanced bind error message" << std::endl;
    Socket secondSocket;
    auto createResult2 = secondSocket.Create(SOCKET_FAMILY::IPV4, SOCKET_TYPE::TCP);
    if (createResult2.IsSuccess()) {
        auto bindResult2 = secondSocket.Bind("127.0.0.1", testPort);
        if (!bindResult2.IsSuccess()) {
            std::string errorMsg = bindResult2.GetErrorMessage();
            if (errorMsg.find("Port " + std::to_string(testPort)) != std::string::npos && 
                errorMsg.find("already in use") != std::string::npos) {
                std::cout << "✅ PASS: Enhanced error message contains port number and 'already in use'" << std::endl;
                std::cout << "   Error: " << errorMsg << std::endl;
            } else {
                std::cout << "❌ FAIL: Error message doesn't contain expected port information" << std::endl;
                std::cout << "   Error: " << errorMsg << std::endl;
            }
        } else {
            std::cout << "❌ FAIL: Second bind should have failed but succeeded" << std::endl;
        }
    }
    
    // Clean up
    serverSocket.Close();
    std::cout << "\n🧹 Cleaned up test socket" << std::endl;
    
    // Test 7: Check if port is available again after cleanup (positive case)
    std::cout << "\n📋 Test 7: Checking if port " << testPort << " is available after cleanup (should be available)" << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(200)); // Small delay for OS cleanup
    bool portAvailableAfterCleanup = Socket::IsPortAvailable(testPort, "127.0.0.1");
    if (portAvailableAfterCleanup) {
        std::cout << "✅ PASS: Port " << testPort << " is available after cleanup" << std::endl;
    } else {
        std::cout << "⚠️  WARN: Port " << testPort << " still reported as in use after cleanup (might need more time)" << std::endl;
    }
}

void TestEdgeCases() {
    std::cout << "\n\n🧪 Testing Edge Cases" << std::endl;
    std::cout << "====================" << std::endl;
    
    // Test 1: Test privileged port (should work on most systems, but might fail on some)
    std::cout << "\n📋 Edge Case 1: Testing privileged port 80" << std::endl;
    bool port80Available = Socket::IsPortAvailable(80, "127.0.0.1");
    std::cout << "   Port 80 availability: " << (port80Available ? "Available" : "In use") << std::endl;
    
    // Test 2: Test high port number
    std::cout << "\n📋 Edge Case 2: Testing high port number 65535" << std::endl;
    bool port65535Available = Socket::IsPortAvailable(65535, "127.0.0.1");
    std::cout << "   Port 65535 availability: " << (port65535Available ? "Available" : "In use") << std::endl;
    
    // Test 3: Test port 0 (should not be allowed)
    std::cout << "\n📋 Edge Case 3: Testing port 0 (invalid)" << std::endl;
    bool port0Available = Socket::IsPortAvailable(0, "127.0.0.1");
    std::cout << "   Port 0 availability: " << (port0Available ? "Available" : "In use") << std::endl;
    
    // Test 4: Test with empty address (should use INADDR_ANY)
    std::cout << "\n📋 Edge Case 4: Testing with empty address" << std::endl;
    bool emptyAddrAvailable = Socket::IsPortAvailable(8083, "");
    std::cout << "   Empty address port 8083 availability: " << (emptyAddrAvailable ? "Available" : "In use") << std::endl;
}

int main() {
    std::cout << "🔍 Port Checking Test Suite" << std::endl;
    std::cout << "===========================" << std::endl;
    std::cout << "💡 This test verifies the port availability checking functionality" << std::endl;
    std::cout << "   including both positive (available) and negative (in use) scenarios." << std::endl;
    
    try {
        TestPortAvailable();
        TestEdgeCases();
        
        std::cout << "\n\n🎯 Port Checking Test Complete" << std::endl;
        std::cout << "=============================" << std::endl;
        std::cout << "✅ All tests executed successfully!" << std::endl;
        std::cout << "💡 Review the results above for any failures or warnings." << std::endl;
        
    } catch (const std::exception& e) {
        std::cout << "\n❌ Test failed with exception: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
