cmake_minimum_required(VERSION 3.15)
project(aiWebSockets VERSION 1.0.0 LANGUAGES CXX)

# Export compile commands for IDE IntelliSense support
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

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

# Platform-specific configuration
if(WIN32)
    # Windows-specific definitions
    add_definitions(-DNOMINMAX)
    add_definitions(-DWIN32_LEAN_AND_MEAN)
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
    src/SocketBase.cpp
    src/AddrInfo.cpp
    src/WebSocketProtocol.cpp
    src/ErrorCodes.cpp
    src/WebSocketServerLite.cpp
    src/WebSocketClientLite.cpp
    src/HttpWsServer.cpp
)

# Header files
set(WEBSOCKET_HEADERS
    include/WebSocket/Socket.h
    include/WebSocket/SocketBase.h
    include/WebSocket/WebSocketProtocol.h
    include/WebSocket/ErrorCodes.h
    include/WebSocket/TestUtilities.h
    include/WebSocket/Types.h
    include/WebSocket/AddrInfo.h
    include/WebSocket/OS.h
)

# Create library
add_library(aiWebSockets STATIC ${SOCKET_SOURCES} ${WEBSOCKET_HEADERS})
target_link_libraries(aiWebSockets ${PLATFORM_LIBS})

# Compiler-specific options
if(MSVC)
    # Treat warnings as errors for better code quality
    target_compile_options(aiWebSockets PRIVATE /WX)
    # Enable high warning levels
    target_compile_options(aiWebSockets PRIVATE /W4)
else()
    # Treat warnings as errors for GCC/Clang
    target_compile_options(aiWebSockets PRIVATE -Werror)
    # Enable comprehensive warnings
    target_compile_options(aiWebSockets PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Debug information
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    if(MSVC)
        target_compile_options(aiWebSockets PRIVATE /Zi)
    else()
        target_compile_options(aiWebSockets PRIVATE -g)
    endif()
endif()

# Install rules (optional)
install(TARGETS aiWebSockets 
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)

install(DIRECTORY include/ 
    DESTINATION include
    FILES_MATCHING PATTERN "*.h"
)

# Enable testing
enable_testing()

# Test executable
add_executable(aiWebSocketsTests tests/main.cpp)
target_link_libraries(aiWebSocketsTests aiWebSockets)

# Add test to CTest
add_test(NAME WebSocketLibraryTests COMMAND aiWebSocketsTests)

# Additional test executables (when they have content)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tests/SocketTests.cpp")
    add_executable(SocketTests tests/SocketTests.cpp)
    target_link_libraries(SocketTests aiWebSockets)
    add_test(NAME SocketTests COMMAND SocketTests)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tests/WebSocketProtocolTests.cpp")
    add_executable(WebSocketProtocolTests tests/WebSocketProtocolTests.cpp)
    target_link_libraries(WebSocketProtocolTests aiWebSockets)
    add_test(NAME WebSocketProtocolTests COMMAND WebSocketProtocolTests)
endif()
