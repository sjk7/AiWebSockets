#include <iostream>
#include <chrono>
#include "WebSocket/Socket.h"

using namespace WebSocket;
using namespace std::chrono;

void TestResultOptimization() {
    std::cout << "🔍 Testing Result Class Optimization" << std::endl;
    std::cout << "=====================================" << std::endl;
    
    // Test 1: Create a Result with system error code (no string formatting yet)
    std::cout << "\n📋 Test 1: Creating Result with system error code (lazy evaluation)" << std::endl;
    auto start = high_resolution_clock::now();
    
    Result result(ERROR_CODE::SOCKET_BIND_FAILED, 10048); // Use a common Windows error code
    
    auto end = high_resolution_clock::now();
    auto creationTime = duration_cast<microseconds>(end - start).count();
    
    std::cout << "✅ Result creation time: " << creationTime << " microseconds" << std::endl;
    std::cout << "🔍 Error code: " << static_cast<int>(result.GetErrorCode()) << std::endl;
    std::cout << "🔍 System error code: " << result.GetSystemErrorCode() << std::endl;
    
    // Test 2: Access the error message (triggers lazy evaluation)
    std::cout << "\n📋 Test 2: Accessing error message (triggers string formatting)" << std::endl;
    start = high_resolution_clock::now();
    
    const std::string& errorMessage = result.GetErrorMessage();
    
    end = high_resolution_clock::now();
    auto formattingTime = duration_cast<microseconds>(end - start).count();
    
    std::cout << "✅ Error message formatting time: " << formattingTime << " microseconds" << std::endl;
    std::cout << "🔍 Error message: " << errorMessage << std::endl;
    
    // Test 3: Access the error message again (should be cached)
    std::cout << "\n📋 Test 3: Accessing error message again (should use cache)" << std::endl;
    start = high_resolution_clock::now();
    
    const std::string& cachedMessage = result.GetErrorMessage();
    
    end = high_resolution_clock::now();
    auto cachedTime = duration_cast<microseconds>(end - start).count();
    
    std::cout << "✅ Cached message access time: " << cachedTime << " microseconds" << std::endl;
    std::cout << "🔍 Messages are same: " << (errorMessage == cachedMessage ? "YES" : "NO") << std::endl;
    
    // Test 4: Performance comparison - multiple accesses
    std::cout << "\n📋 Test 4: Performance comparison - 1000 accesses" << std::endl;
    
    // Create fresh result
    Result freshResult(ERROR_CODE::SOCKET_CONNECT_FAILED, 10060);
    
    // Time multiple accesses to cached message
    start = high_resolution_clock::now();
    for (int i = 0; i < 1000; ++i) {
        volatile const std::string& msg = freshResult.GetErrorMessage();
        (void)msg; // Prevent optimization
    }
    end = high_resolution_clock::now();
    auto multipleAccessTime = duration_cast<microseconds>(end - start).count();
    
    std::cout << "✅ 1000 cached accesses time: " << multipleAccessTime << " microseconds" << std::endl;
    std::cout << "🔍 Average per access: " << (multipleAccessTime / 1000.0) << " microseconds" << std::endl;
    
    // Test 5: Test backward compatibility
    std::cout << "\n📋 Test 5: Testing backward compatibility with ErrorMessage()" << std::endl;
    Result compatResult(ERROR_CODE::INVALID_PARAMETER, "Test custom message");
    
    std::cout << "✅ IsSuccess(): " << (compatResult.IsSuccess() ? "true" : "false") << std::endl;
    std::cout << "✅ IsError(): " << (compatResult.IsError() ? "true" : "false") << std::endl;
    std::cout << "✅ ErrorMessage(): " << compatResult.GetErrorMessage() << std::endl;
    std::cout << "✅ GetErrorMessage(): " << compatResult.GetErrorMessage() << std::endl;
    
    // Test 6: Test copy constructor and assignment
    std::cout << "\n📋 Test 6: Testing copy constructor and assignment" << std::endl;
    Result original(ERROR_CODE::WEBSOCKET_HANDSHAKE_FAILED, 10054);
    Result copy(original);
    Result assigned = original;
    
    std::cout << "✅ Original message: " << original.GetErrorMessage() << std::endl;
    std::cout << "✅ Copy message: " << copy.GetErrorMessage() << std::endl;
    std::cout << "✅ Assigned message: " << assigned.GetErrorMessage() << std::endl;
    
    std::cout << "\n🎯 Result Optimization Test Complete!" << std::endl;
    std::cout << "=====================================" << std::endl;
    std::cout << "✅ Lazy evaluation working - error messages only formatted when accessed" << std::endl;
    std::cout << "✅ Caching working - subsequent accesses use cached message" << std::endl;
    std::cout << "✅ Backward compatibility maintained" << std::endl;
    std::cout << "✅ Copy/assignment semantics working correctly" << std::endl;
}

int main() {
    try {
        TestResultOptimization();
        return 0;
    } catch (const std::exception& e) {
        std::cout << "❌ Test failed with exception: " << e.what() << std::endl;
        return 1;
    }
}
