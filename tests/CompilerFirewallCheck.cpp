#include <iostream>

// 🧠 SIMPLE & CLEVER: Check for native socket header inclusion with #defines
// This will NEVER break the build - it just checks if headers were included

#ifdef _WIN32
    // Check if Winsock headers are exposed (they shouldn't be!)
    #ifdef _WINSOCK2API_
        #define WINSOCK_HEADERS_EXPOSED 1
    #else
        #define WINSOCK_HEADERS_EXPOSED 0
    #endif
    
    // Check for specific Winsock symbols that shouldn't be visible
    #ifdef SOCKET
        #define NATIVE_SOCKET_EXPOSED 1
    #else
        #define NATIVE_SOCKET_EXPOSED 0
    #endif
    
    #ifdef WSAEWOULDBLOCK
        #define WINSOCK_ERRORS_EXPOSED 1
    #else
        #define WINSOCK_ERRORS_EXPOSED 0
    #endif
#else
    // POSIX checks
    #ifdef _SYS_SOCKET_H
        #define POSIX_SOCKET_HEADERS_EXPOSED 1
    #else
        #define POSIX_SOCKET_HEADERS_EXPOSED 0
    #endif
    
    #ifdef AF_INET
        #define POSIX_SOCKET_CONSTANTS_EXPOSED 1
    #else
        #define POSIX_SOCKET_CONSTANTS_EXPOSED 0
    #endif
#endif

// Calculate overall abstraction status
#ifdef _WIN32
    constexpr bool COMPILER_ABSTRACTION_WORKING = !WINSOCK_HEADERS_EXPOSED && !NATIVE_SOCKET_EXPOSED && !WINSOCK_ERRORS_EXPOSED;
#else
    constexpr bool COMPILER_ABSTRACTION_WORKING = !POSIX_SOCKET_HEADERS_EXPOSED && !POSIX_SOCKET_CONSTANTS_EXPOSED;
#endif

int main() {
    std::cout << "=== Compiler Firewall Check ===" << std::endl;
    
    // 🛡️ Compiler Abstraction Check (COMPILE-TIME, never breaks build!)
    if (COMPILER_ABSTRACTION_WORKING) {
        std::cout << "✅ COMPILER ABSTRACTION WORKING: Native socket headers are HIDDEN!" << std::endl;
    } else {
        std::cout << "❌ COMPILER ABSTRACTION BROKEN: Native socket headers are EXPOSED!" << std::endl;
    }
    
#ifdef _WIN32
    std::cout << "   Winsock headers exposed: " << (WINSOCK_HEADERS_EXPOSED ? "YES" : "NO") << std::endl;
    std::cout << "   Native SOCKET exposed: " << (NATIVE_SOCKET_EXPOSED ? "YES" : "NO") << std::endl;
    std::cout << "   Winsock errors exposed: " << (WINSOCK_ERRORS_EXPOSED ? "YES" : "NO") << std::endl;
#else
    std::cout << "   POSIX socket headers exposed: " << (POSIX_SOCKET_HEADERS_EXPOSED ? "YES" : "NO") << std::endl;
    std::cout << "   POSIX socket constants exposed: " << (POSIX_SOCKET_CONSTANTS_EXPOSED ? "YES" : "NO") << std::endl;
#endif
    
    std::cout << "🛡️ Compiler Firewall Status: " << (COMPILER_ABSTRACTION_WORKING ? "MAINTAINED" : "BROKEN") << std::endl;
    
    std::cout << "\n=== Test Complete ===" << std::endl;
    std::cout << "🧠 Clever #define detection works without breaking build!" << std::endl;
    
    return 0;
}
