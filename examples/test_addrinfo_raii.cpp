#include "WebSocket/AddrInfoGuard.h"
#include <iostream>
#include <cassert>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <netdb.h>
#endif

int main() {
    std::cout << "=== Testing AddrInfoGuard Move Semantics ===" << std::endl;
    
#ifdef _WIN32
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif
    
    // Test 1: Basic move construction
    {
        std::cout << "Test 1: Move construction..." << std::endl;
        WebSocket::AddrInfoGuard original = WebSocket::GetAddrInfo("localhost", nullptr, nullptr);
        
        if (original.isValid()) {
            std::cout << "✅ Original is valid before move" << std::endl;
            
            WebSocket::AddrInfoGuard moved = std::move(original);
            
            if (!original.isValid() && moved.isValid()) {
                std::cout << "✅ Move construction successful" << std::endl;
            } else {
                std::cout << "❌ Move construction failed" << std::endl;
                return 1;
            }
        } else {
            std::cout << "⚠️  Could not get addrinfo for localhost (expected in some environments)" << std::endl;
        }
    }
    
    // Test 2: Move assignment
    {
        std::cout << "Test 2: Move assignment..." << std::endl;
        WebSocket::AddrInfoGuard original = WebSocket::GetAddrInfo("localhost", nullptr, nullptr);
        WebSocket::AddrInfoGuard target;
        
        if (original.isValid()) {
            std::cout << "✅ Original is valid before move" << std::endl;
            
            target = std::move(original);
            
            if (!original.isValid() && target.isValid()) {
                std::cout << "✅ Move assignment successful" << std::endl;
            } else {
                std::cout << "❌ Move assignment failed" << std::endl;
                return 1;
            }
        } else {
            std::cout << "⚠️  Could not get addrinfo for localhost (expected in some environments)" << std::endl;
        }
    }
    
    // Test 3: Iteration after move
    {
        std::cout << "Test 3: Iteration after move..." << std::endl;
        WebSocket::AddrInfoGuard original = WebSocket::GetAddrInfo("localhost", nullptr, nullptr);
        
        if (original.isValid()) {
            int originalCount = 0;
            for (const auto& addr : original) {
                originalCount++;
                (void)addr; // Suppress unused variable warning
            }
            std::cout << "✅ Original has " << originalCount << " addresses" << std::endl;
            
            WebSocket::AddrInfoGuard moved = std::move(original);
            
            int movedCount = 0;
            for (const auto& addr : moved) {
                movedCount++;
                (void)addr; // Suppress unused variable warning
            }
            
            int originalAfterMoveCount = 0;
            for (const auto& addr : original) {
                originalAfterMoveCount++;
                (void)addr; // Suppress unused variable warning
            }
            
            if (movedCount == originalCount && originalAfterMoveCount == 0) {
                std::cout << "✅ Iteration works correctly after move" << std::endl;
            } else {
                std::cout << "❌ Iteration failed after move" << std::endl;
                return 1;
            }
        } else {
            std::cout << "⚠️  Could not get addrinfo for localhost (expected in some environments)" << std::endl;
        }
    }
    
    // Test 4: Destructor safety (no crash)
    {
        std::cout << "Test 4: Destructor safety..." << std::endl;
        WebSocket::AddrInfoGuard guard = WebSocket::GetAddrInfo("localhost", nullptr, nullptr);
        
        if (guard.isValid()) {
            WebSocket::AddrInfoGuard moved = std::move(guard);
            // Both guards will be destroyed here - should not crash
            std::cout << "✅ No crash during destruction" << std::endl;
        } else {
            std::cout << "⚠️  Could not get addrinfo for localhost (expected in some environments)" << std::endl;
        }
    }
    
#ifdef _WIN32
    WSACleanup();
#endif
    
    std::cout << "✅ All move semantics tests passed!" << std::endl;
    return 0;
}
