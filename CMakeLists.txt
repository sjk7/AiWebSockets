cmake_minimum_required(VERSION 3.15)
project(aiWebSockets VERSION 1.0.0 LANGUAGES CXX)

# Fix timestamp warnings by setting CMake policies
if(POLICY CMP0115)
    cmake_policy(SET CMP0115 NEW)  # Source file extensions must be explicit
endif()

if(POLICY CMP0116)
    cmake_policy(SET CMP0116 NEW)  # generators support multiple output configurations
endif()

# Suppress timestamp checking warnings for faster builds
set(CMAKE_SUPPRESS_REGENERATION TRUE)
set(CMAKE_SKIP_INSTALL_ALL_DEPENDENCY TRUE)

# Additional timestamp and build optimization settings
set(CMAKE_DISABLE_FIND_PACKAGE_BoostAvailable TRUE)
set(CMAKE_AUTOMOC_RELAXED_MODE TRUE)
set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE "")
set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK "")

# Prevent CMake from re-running due to timestamp changes
set(CMAKE_CONFIGURE_DEPENDS "")

# Set C++17 standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Prefer Ninja for better performance on all platforms
if(UNIX AND NOT WIN32)
    # Check if Ninja is available on POSIX systems
    find_program(NINJA_EXECUTABLE ninja)
    if(NINJA_EXECUTABLE)
        message(STATUS "Found Ninja: ${NINJA_EXECUTABLE}")
        set(CMAKE_GENERATOR "Ninja" CACHE STRING "Build system generator" FORCE)
    else()
        message(STATUS "Ninja not found, using default generator")
    endif()
elseif(WIN32)
    # Check if Ninja is available on Windows
    find_program(NINJA_EXECUTABLE ninja)
    if(NINJA_EXECUTABLE)
        message(STATUS "Found Ninja on Windows: ${NINJA_EXECUTABLE}")
        set(CMAKE_GENERATOR "Ninja" CACHE STRING "Build system generator" FORCE)
        # Don't force compiler - let CMake find it automatically
        # This avoids path issues when using Ninja
    else()
        message(STATUS "Ninja not found on Windows, using Visual Studio generator")
        message(STATUS "To install Ninja for faster builds: 'pip install ninja' or 'choco install ninja'")
    endif()
endif()

# Visual Studio compiler configuration
if(WIN32)
    # Prefer Visual Studio 2022, but allow VS 2026 Insiders
    if(NOT CMAKE_GENERATOR MATCHES "Visual Studio" AND NOT CMAKE_GENERATOR MATCHES "Ninja")
        # Check for VS 2022 first
        find_program(VCVARSALL_2022 
            NAMES vcvars64.bat vcvarsall.bat
            PATHS 
                "C:/Program Files/Microsoft Visual Studio/2022/Community/VC/Auxiliary/Build"
                "C:/Program Files/Microsoft Visual Studio/2022/Professional/VC/Auxiliary/Build"
                "C:/Program Files/Microsoft Visual Studio/2022/Enterprise/VC/Auxiliary/Build"
            NO_DEFAULT_PATH
        )
        
        if(VCVARSALL_2022)
            set(CMAKE_GENERATOR "Visual Studio 17 2022" CACHE STRING "Build system generator" FORCE)
        else()
            # Fallback to VS 2019
            find_program(VCVARSALL_2019
                NAMES vcvars64.bat vcvarsall.bat
                PATHS
                    "C:/Program Files (x86)/Microsoft Visual Studio/2019/Community/VC/Auxiliary/Build"
                    "C:/Program Files (x86)/Microsoft Visual Studio/2019/Professional/VC/Auxiliary/Build"
                    "C:/Program Files (x86)/Microsoft Visual Studio/2019/Enterprise/VC/Auxiliary/Build"
                NO_DEFAULT_PATH
            )
            
            if(VCVARSALL_2019)
                set(CMAKE_GENERATOR "Visual Studio 16 2019" CACHE STRING "Build system generator" FORCE)
            endif()
        endif()
    endif()
    
    # Windows-specific definitions
    add_definitions(-DWIN32_LEAN_AND_MEAN)
    add_definitions(-DNOMINMAX)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
    set(PLATFORM_LIBS ws2_32)
    
    # Enable latest C++ features for VS 2022/2026
    if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
        # Treat all warnings as errors - no warnings allowed
        add_compile_options(/WX)
        # Enable all warnings
        add_compile_options(/W4)
        # Enable C++17 latest features
        add_compile_options(/std:c++17)
        # Enable parallel compilation
        add_compile_options(/MP)
        # Disable specific warnings that are unavoidable
        add_compile_options(/wd4244)  # conversion possible loss of data
        add_compile_options(/wd4100)  # unreferenced parameter
        # Enable string pooling and optimization
        if(CMAKE_BUILD_TYPE STREQUAL "Release")
            add_compile_options(/O2 /Ob2 /GF)
        endif()
    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        # Clang on Windows - use MSVC compatibility mode
        add_compile_options(--target=x86_64-pc-windows-msvc)
        add_compile_options(-fms-extensions)
        add_compile_options(-fms-compatibility-version=19.20)
        # Link with MSVC standard library
        add_link_options(--target=x86_64-pc-windows-msvc)
        add_link_options(-fuse-ld=lld)
        # Treat warnings as errors
        add_compile_options(-Werror -Wall -Wextra)
        # C++17 standard (don't duplicate - already set globally)
    endif()
    
elseif(APPLE)
    set(PLATFORM_LIBS)
    # Use latest Clang on macOS
    set(CMAKE_CXX_COMPILER "clang++")
    # Treat warnings as errors on macOS
    add_compile_options(-Werror -Wall -Wextra)
    
elseif(UNIX)
    set(PLATFORM_LIBS pthread)
    # Use latest GCC on Linux
    set(CMAKE_CXX_COMPILER "g++")
    # Enable C++17 features and treat warnings as errors
    add_compile_options(-std=c++17 -Werror -Wall -Wextra)
endif()

# Include directories
include_directories(include)

# Source files
set(SOCKET_SOURCES
    src/Socket.cpp
    src/WebSocketProtocol.cpp
    src/ErrorCodes.cpp
    src/WebSocketServerLite.cpp
    src/WebSocketClientLite.cpp
    src/HttpWsServer.cpp
)

# Precompiled Headers - Enable when project grows
# Uncomment these lines when project has 15+ source files
# option(ENABLE_PCH "Enable precompiled headers" OFF)
# 
# if(ENABLE_PCH)
#     # Create PCH target
#     set(PCH_HEADER include/WebSocket/PCH.h)
#     
#     # Add PCH to all targets
#     target_precompile_headers(aiWebSockets PRIVATE ${PCH_HEADER})
#     target_precompile_headers(aiWebSocketsTests PRIVATE ${PCH_HEADER})
#     target_precompile_headers(aiWebSocketsServer PRIVATE ${PCH_HEADER})
#     
#     message(STATUS "Precompiled headers enabled")
# else()
#     message(STATUS "Precompiled headers disabled (enable with -DENABLE_PCH=ON)")
# endif()

# Header files
set(WEBSOCKET_HEADERS
    include/WebSocket/Socket.h
    include/WebSocket/WebSocketProtocol.h
    include/WebSocket/ErrorCodes.h
    include/WebSocket/TestUtilities.h
    include/WebSocket/Types.h
    include/WebSocket/AddrInfoGuard.h
)

# Create library
add_library(aiWebSockets STATIC ${SOCKET_SOURCES} ${WEBSOCKET_HEADERS})
target_link_libraries(aiWebSockets ${PLATFORM_LIBS})

# Test executable
add_executable(aiWebSocketsTests
    tests/main.cpp
    tests/SocketTests.cpp
    tests/WebSocketProtocolTests.cpp
)

target_link_libraries(aiWebSocketsTests aiWebSockets ${PLATFORM_LIBS})

# Create server executable
add_executable(aiWebSocketsServer examples/server.cpp)
target_link_libraries(aiWebSocketsServer aiWebSockets)

# Create socket test executable
add_executable(socket_test examples/socket_test.cpp)
target_link_libraries(socket_test aiWebSockets ${PLATFORM_LIBS})

# Create performance test executable
add_executable(performance_test examples/performance_test.cpp)
target_link_libraries(performance_test aiWebSockets ${PLATFORM_LIBS})

add_executable(event_loop_test examples/event_loop_test.cpp)
target_link_libraries(event_loop_test aiWebSockets ${PLATFORM_LIBS})

# Server-to-client test executable
add_executable(server_to_client_test examples/server_to_client_test.cpp)
target_link_libraries(server_to_client_test aiWebSockets ${PLATFORM_LIBS})

# Large data test executable
add_executable(large_data_test examples/large_data_test.cpp)
target_link_libraries(large_data_test aiWebSockets ${PLATFORM_LIBS})

# Async performance test executable
add_executable(async_performance_test examples/async_performance_test.cpp)
target_link_libraries(async_performance_test aiWebSockets ${PLATFORM_LIBS})

# Sequential server test executable
add_executable(sequential_server examples/sequential_server.cpp)
target_link_libraries(sequential_server aiWebSockets ${PLATFORM_LIBS})

# Test client executable
add_executable(test_client examples/test_client.cpp)
target_link_libraries(test_client aiWebSockets ${PLATFORM_LIBS})

# Hybrid server executable
add_executable(hybrid_server examples/hybrid_server.cpp)
target_link_libraries(hybrid_server aiWebSockets ${PLATFORM_LIBS})

# HTTP client executable
add_executable(http_client examples/http_client.cpp)
target_link_libraries(http_client aiWebSockets ${PLATFORM_LIBS})

# WebSocket client executable
add_executable(websocket_client examples/websocket_client.cpp)
target_link_libraries(websocket_client aiWebSockets ${PLATFORM_LIBS})

# WebSocket compliance test executable
add_executable(websocket_compliance_test examples/websocket_compliance_test.cpp)
target_link_libraries(websocket_compliance_test aiWebSockets ${PLATFORM_LIBS})

# Enhanced handshake test executable
add_executable(enhanced_handshake_test examples/enhanced_handshake_test.cpp)
target_link_libraries(enhanced_handshake_test aiWebSockets ${PLATFORM_LIBS})

# HTTP quick wins test executable
add_executable(test_http_quick_wins examples/test_http_quick_wins.cpp)
target_link_libraries(test_http_quick_wins aiWebSockets ${PLATFORM_LIBS})

# Comprehensive HTTP test executable
add_executable(comprehensive_http_test examples/comprehensive_http_test.cpp)
target_link_libraries(comprehensive_http_test aiWebSockets ${PLATFORM_LIBS})

# Security test executable
add_executable(security_test examples/security_test.cpp)
target_link_libraries(security_test aiWebSockets ${PLATFORM_LIBS})

# Enhanced security test executable
add_executable(enhanced_security_test examples/enhanced_security_test.cpp)
target_link_libraries(enhanced_security_test aiWebSockets ${PLATFORM_LIBS})

# HTTP security test executable
add_executable(http_security_test examples/http_security_test.cpp)
target_link_libraries(http_security_test aiWebSockets ws2_32)

# HTTP Performance Test
add_executable(http_performance_test examples/http_performance_test.cpp)
target_link_libraries(http_performance_test aiWebSockets ws2_32)

# Raw Socket Performance Test
add_executable(raw_socket_performance_test examples/raw_socket_performance_test.cpp)
target_link_libraries(raw_socket_performance_test aiWebSockets ws2_32)

# Optimized HTTP Performance Test
add_executable(optimized_http_test examples/optimized_http_test.cpp)
target_link_libraries(optimized_http_test aiWebSockets ws2_32)

# Async HTTP Performance Test
add_executable(async_http_performance_test examples/async_http_performance_test.cpp)
target_link_libraries(async_http_performance_test aiWebSockets ws2_32)

# Direct Async I/O Test
add_executable(async_direct_test examples/async_direct_test.cpp)
target_link_libraries(async_direct_test aiWebSockets ws2_32)

# Simple Async I/O Test
add_executable(async_simple_test examples/async_simple_test.cpp)
target_link_libraries(async_simple_test aiWebSockets ws2_32)

# Clean Async I/O Test
add_executable(clean_async_test examples/clean_async_test.cpp)
target_link_libraries(clean_async_test aiWebSockets ws2_32)

# Keep-Alive Performance Test
add_executable(keepalive_performance_test examples/keepalive_performance_test.cpp)
target_link_libraries(keepalive_performance_test aiWebSockets ws2_32)

# User-Agent filtering test
add_executable(user_agent_test examples/user_agent_test.cpp)
target_link_libraries(user_agent_test aiWebSockets)
set_target_properties(user_agent_test PROPERTIES FOLDER "Tests")

# Port checking test
add_executable(port_checking_test examples/port_checking_test.cpp)
target_link_libraries(port_checking_test aiWebSockets)
set_target_properties(port_checking_test PROPERTIES FOLDER "Tests")

# Result optimization test
add_executable(result_optimization_test examples/result_optimization_test.cpp)
target_link_libraries(result_optimization_test aiWebSockets)
set_target_properties(result_optimization_test PROPERTIES FOLDER "Tests")

# AddrInfoGuard RAII test
add_executable(test_addrinfo_raii examples/test_addrinfo_raii.cpp)
target_link_libraries(test_addrinfo_raii aiWebSockets ${PLATFORM_LIBS})
set_target_properties(test_addrinfo_raii PROPERTIES FOLDER "Tests")

# Simple server example
add_executable(simple_server_example examples/simple_server_example.cpp)
target_link_libraries(simple_server_example aiWebSockets)
set_target_properties(simple_server_example PROPERTIES FOLDER "Examples")

# Simple client example
add_executable(simple_client_example examples/simple_client_example.cpp)
target_link_libraries(simple_client_example aiWebSockets)
set_target_properties(simple_client_example PROPERTIES FOLDER "Examples")

# Non-blocking server example
add_executable(nonblocking_server_example examples/nonblocking_server_example.cpp)
target_link_libraries(nonblocking_server_example aiWebSockets)
set_target_properties(nonblocking_server_example PROPERTIES FOLDER "Examples")

# Socket address examples
add_executable(socket_address_examples examples/socket_address_examples.cpp)
target_link_libraries(socket_address_examples aiWebSockets)
set_target_properties(socket_address_examples PROPERTIES FOLDER "Examples")

# WebSocket Lite address info examples
add_executable(websocket_lite_address_info examples/websocket_lite_address_info.cpp)
target_link_libraries(websocket_lite_address_info aiWebSockets)
set_target_properties(websocket_lite_address_info PROPERTIES FOLDER "Examples")

# Local IP enumeration examples
add_executable(local_ip_enumeration examples/local_ip_enumeration.cpp)
target_link_libraries(local_ip_enumeration aiWebSockets)
set_target_properties(local_ip_enumeration PROPERTIES FOLDER "Examples")

# Multi-interface server examples
add_executable(multi_interface_server examples/multi_interface_server.cpp)
target_link_libraries(multi_interface_server aiWebSockets)
set_target_properties(multi_interface_server PROPERTIES FOLDER "Examples")

# IPv6 test examples
add_executable(ipv6_test examples/ipv6_test.cpp)
target_link_libraries(ipv6_test aiWebSockets)
set_target_properties(ipv6_test PROPERTIES FOLDER "Examples")

# Boilerplate-free test
add_executable(boilerplate_free_test examples/boilerplate_free_test.cpp)
target_link_libraries(boilerplate_free_test aiWebSockets)
set_target_properties(boilerplate_free_test PROPERTIES FOLDER "Examples")

# Minimal example
add_executable(minimal_example examples/minimal_example.cpp)
target_link_libraries(minimal_example aiWebSockets)
set_target_properties(minimal_example PROPERTIES FOLDER "Examples")

# HTTP + WebSocket server example
add_executable(http_websocket_server examples/http_websocket_server.cpp)
target_link_libraries(http_websocket_server aiWebSockets)
set_target_properties(http_websocket_server PROPERTIES FOLDER "Examples")

# HTTP + WebSocket hybrid server example
add_executable(http_websocket_hybrid examples/http_websocket_hybrid.cpp)
target_link_libraries(http_websocket_hybrid aiWebSockets)
set_target_properties(http_websocket_hybrid PROPERTIES FOLDER "Examples")

# Secure HTTP + WebSocket server example
add_executable(secure_server_example examples/secure_server_example.cpp)
target_link_libraries(secure_server_example aiWebSockets)
set_target_properties(secure_server_example PROPERTIES FOLDER "Examples")

# Non-blocking hybrid server executable
add_executable(nonblocking_hybrid_server examples/nonblocking_hybrid_server.cpp)
target_link_libraries(nonblocking_hybrid_server aiWebSockets ${PLATFORM_LIBS})

# Enable testing
enable_testing()
add_test(NAME WebSocketTests COMMAND aiWebSocketsTests)

# Compiler-specific settings
if(MSVC)
    # Treat warnings as errors for better code quality
    target_compile_options(aiWebSockets PRIVATE /WX)
    target_compile_options(aiWebSocketsTests PRIVATE /WX)
    target_compile_options(aiWebSocketsServer PRIVATE /WX)
    target_compile_options(socket_test PRIVATE /WX)
    target_compile_options(performance_test PRIVATE /WX)
    target_compile_options(event_loop_test PRIVATE /WX)
    target_compile_options(large_data_test PRIVATE /WX)
    target_compile_options(sequential_server PRIVATE /WX)
    target_compile_options(async_performance_test PRIVATE /WX)
    target_compile_options(server_to_client_test PRIVATE /WX)
    target_compile_options(test_client PRIVATE /WX)
    target_compile_options(http_client PRIVATE /WX)
    target_compile_options(websocket_client PRIVATE /WX)
    target_compile_options(websocket_compliance_test PRIVATE /WX)
    target_compile_options(hybrid_server PRIVATE /WX)
    target_compile_options(nonblocking_hybrid_server PRIVATE /WX)
    target_compile_options(enhanced_handshake_test PRIVATE /WX)
    target_compile_options(test_http_quick_wins PRIVATE /WX)
    target_compile_options(comprehensive_http_test PRIVATE /WX)
    target_compile_options(security_test PRIVATE /WX)
    target_compile_options(enhanced_security_test PRIVATE /WX)
    target_compile_options(http_security_test PRIVATE /WX)
    target_compile_options(user_agent_test PRIVATE /WX)
    
    # Enable high warning levels
    target_compile_options(aiWebSockets PRIVATE /W4)
    target_compile_options(aiWebSocketsTests PRIVATE /W4)
    target_compile_options(aiWebSocketsServer PRIVATE /W4)
    target_compile_options(socket_test PRIVATE /W4)
    target_compile_options(performance_test PRIVATE /W4)
    target_compile_options(event_loop_test PRIVATE /W4)
    target_compile_options(large_data_test PRIVATE /W4)
    target_compile_options(sequential_server PRIVATE /W4)
    target_compile_options(async_performance_test PRIVATE /W4)
    target_compile_options(server_to_client_test PRIVATE /W4)
    target_compile_options(test_client PRIVATE /W4)
    target_compile_options(http_client PRIVATE /W4)
    target_compile_options(websocket_client PRIVATE /W4)
    target_compile_options(websocket_compliance_test PRIVATE /W4)
    target_compile_options(hybrid_server PRIVATE /W4)
    target_compile_options(nonblocking_hybrid_server PRIVATE /W4)
    target_compile_options(enhanced_handshake_test PRIVATE /W4)
    target_compile_options(test_http_quick_wins PRIVATE /W4)
    target_compile_options(comprehensive_http_test PRIVATE /W4)
    target_compile_options(security_test PRIVATE /W4)
    target_compile_options(enhanced_security_test PRIVATE /W4)
    target_compile_options(http_security_test PRIVATE /W4)
    target_compile_options(user_agent_test PRIVATE /W4)
else()
    # Treat warnings as errors for GCC/Clang
    target_compile_options(aiWebSockets PRIVATE -Werror)
    target_compile_options(aiWebSocketsTests PRIVATE -Werror)
    target_compile_options(aiWebSocketsServer PRIVATE -Werror)
    target_compile_options(socket_test PRIVATE -Werror)
    target_compile_options(performance_test PRIVATE -Werror)
    target_compile_options(event_loop_test PRIVATE -Werror)
    target_compile_options(large_data_test PRIVATE -Werror)
    target_compile_options(sequential_server PRIVATE -Werror)
    target_compile_options(async_performance_test PRIVATE -Werror)
    target_compile_options(server_to_client_test PRIVATE -Werror)
    target_compile_options(test_client PRIVATE -Werror)
    target_compile_options(http_client PRIVATE -Werror)
    target_compile_options(websocket_client PRIVATE -Werror)
    target_compile_options(websocket_compliance_test PRIVATE -Werror)
    target_compile_options(hybrid_server PRIVATE -Werror)
    target_compile_options(nonblocking_hybrid_server PRIVATE -Werror)
    target_compile_options(enhanced_handshake_test PRIVATE -Werror)
    target_compile_options(test_http_quick_wins PRIVATE -Werror)
    target_compile_options(comprehensive_http_test PRIVATE -Werror)
    target_compile_options(security_test PRIVATE -Werror)
    target_compile_options(enhanced_security_test PRIVATE -Werror)
    target_compile_options(http_security_test PRIVATE -Werror)
    target_compile_options(user_agent_test PRIVATE -Werror)
    target_compile_options(port_checking_test PRIVATE -Werror)
    target_compile_options(result_optimization_test PRIVATE -Werror)
    target_compile_options(test_addrinfo_raii PRIVATE -Werror)
    
    # Enable comprehensive warnings
    target_compile_options(aiWebSockets PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(aiWebSocketsTests PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(aiWebSocketsServer PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(socket_test PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(performance_test PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(event_loop_test PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(large_data_test PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(sequential_server PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(async_performance_test PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(server_to_client_test PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(test_client PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(http_client PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(websocket_client PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(websocket_compliance_test PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(hybrid_server PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(nonblocking_hybrid_server PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(enhanced_handshake_test PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(test_http_quick_wins PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(comprehensive_http_test PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(security_test PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(enhanced_security_test PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(http_security_test PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(user_agent_test PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(port_checking_test PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(result_optimization_test PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(test_addrinfo_raii PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Debug information
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    if(MSVC)
        target_compile_options(aiWebSockets PRIVATE /Zi)
        target_compile_options(aiWebSocketsTests PRIVATE /Zi)
        target_compile_options(aiWebSocketsServer PRIVATE /Zi)
        target_compile_options(socket_test PRIVATE /Zi)
        target_compile_options(performance_test PRIVATE /Zi)
        target_compile_options(event_loop_test PRIVATE /Zi)
    else()
        target_compile_options(aiWebSockets PRIVATE -g)
        target_compile_options(aiWebSocketsTests PRIVATE -g)
        target_compile_options(aiWebSocketsServer PRIVATE -g)
        target_compile_options(socket_test PRIVATE -g)
        target_compile_options(performance_test PRIVATE -g)
        target_compile_options(event_loop_test PRIVATE -g)
    endif()
endif()
