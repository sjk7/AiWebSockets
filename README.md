# aiWebSockets

A comprehensive, cross-platform C++ WebSocket implementation with server-side functionality and JavaScript client testing.

## Features

- **Cross-platform**: Windows, Linux, and macOS support
- **Modern C++**: C++17 standard, no exceptions
- **Single-threaded**: Event-loop architecture with callback-style API
- **Comprehensive Protocol**: Full WebSocket protocol support including ping/pong, extensions, and subprotocols
- **TDD Approach**: Test-driven development with CTest
- **No External Dependencies**: Pure C++ implementation with platform-specific socket APIs
- **Debugging Ready**: LLDB debugging configuration for Windsurf
- **High Performance**: Async I/O with 23+ MB/s throughput

## Architecture

```
aiWebSockets/
├── CMakeLists.txt              # Main CMake configuration
├── include/WebSocket/          # Public headers
│   ├── ErrorCodes.h           # Error handling and result types
│   ├── Types.h                # Common types and enums
│   ├── Socket.h               # Cross-platform socket wrapper
│   ├── WebSocketProtocol.h    # WebSocket protocol implementation
│   ├── HttpWsServer.h         # HTTP + WebSocket server implementation
│   └── WebSocketServerLite.h  # Lightweight WebSocket server
├── src/                       # Implementation files
├── tests/                     # TDD test suite
├── examples/                  # Example applications
│   ├── server.cpp             # WebSocket server example
│   └── client.html            # JavaScript test client
├── .vscode/                   # Windsurf debugging configuration
└── README.md                  # This file
```

## Building

### Prerequisites

- CMake 3.15 or higher
- C++17 compatible compiler
- **Visual Studio 2022** (recommended) or **Visual Studio 2026 Insiders** on Windows
- **Ninja** (recommended on Linux/macOS for faster builds)
- LLDB (for debugging, optional)

### Build System Performance

#### Ninja vs MSBuild/Make

**Ninja Benefits:**
- **2-10x faster** incremental builds
- **Automatic parallelization** (uses all CPU cores)
- **Minimal disk I/O** (only rebuilds what changed)
- **Better dependency tracking**
- **Cleaner output** (less verbose)

**Windows**: MSBuild is already well-optimized, Ninja provides modest gains
**Linux/macOS**: Ninja provides significant performance improvements over make

### Visual Studio Build Options

#### Option 1: Visual Studio 2022 (Recommended)
```bash
# Build specifically with VS 2022
scripts\build-vs2022.bat
```

#### Option 2: Visual Studio 2026 Insiders
```bash
# Build specifically with VS 2026 Insiders
scripts\build-vs2026.bat
```

#### Option 3: Ninja on Windows (if installed)
```bash
# Use Ninja with Visual Studio compiler (faster parallel builds)
scripts\build-with-ninja.bat
```

#### Option 4: Auto-Detect (VS 2022 preferred, falls back to VS 2026)
```bash
# Automatically detects and uses available VS version
scripts\build-and-commit.bat
```

### POSIX Builds (Linux/macOS) - Ninja Preferred

```bash
# POSIX build with Ninja (auto-installs if needed)
./scripts/build-posix.sh
```

### Manual Build Steps

#### Windows
```bash
# Configure the project (VS 2022)
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 -DCMAKE_BUILD_TYPE=Debug

# Configure with Ninja (if available)
cmake -S . -B build -G "Ninja" -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_COMPILER=cl

# Build all targets
cmake --build build --config Debug
```

#### Linux/macOS
```bash
# Configure with Ninja (preferred)
cmake -S . -B build -G "Ninja" -DCMAKE_BUILD_TYPE=Debug

# Or with make (fallback)
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug

# Build with Ninja (automatic parallelization)
ninja -C build

# Or with make
make -C build -j$(nproc)
```

### Individual Targets

```bash
# Build only the server example
cmake --build build --target aiWebSocketsServer --config Debug

# Build only the tests
cmake --build build --target aiWebSocketsTests --config Debug
```

## Usage

### Server Example

```cpp
#include "WebSocket/HttpWsServer.h"

// Configure server
SecurityConfig security;
security.maxConnectionsPerIP = 10;
security.maxConnectionsTotal = 100;

// Create server
HttpWsServer server(8080, "0.0.0.0", security);

// Set up event handlers
server.OnHttpRequest([](const HTTPRequest& request) -> std::string {
    return "HTTP Response";
});

server.OnWebSocketMessage([](const WebSocketMessageWithIP& message) -> std::string {
    return "Echo: " + message.message.AsText();
});

// Start server
auto result = server.Start();
if (!result.IsSuccess()) {
    printf("Failed to start server: %s\n", result.GetErrorMessage().c_str());
    return 1;
}
```

### JavaScript Client

Open `examples/client.html` in a web browser to test the WebSocket connection.

## API Design

### Error Handling

No exceptions are used. All functions return a `Result` struct:

```cpp
struct Result {
    ERROR_CODE ErrorCode;
    std::string ErrorMessage;
    
    bool IsSuccess() const;
    bool IsError() const;
};
```

### Socket Wrapper

The `Socket` class provides cross-platform socket operations:

```cpp
// Raw I/O methods
Result SendRaw(const void* data, size_t length);
std::pair<Result, size_t> ReceiveRaw(void* buffer, size_t bufferSize);

// WebSocket-aware methods
Result Send(const std::vector<uint8_t>& data);
std::pair<Result, std::vector<uint8_t>> Receive(size_t maxLength = 4096);
```

### Callback System

Event-driven architecture with comprehensive callbacks:

```cpp
using ConnectionCallback = std::function<void(std::shared_ptr<WebSocketConnection>)>;
using MessageCallback = std::function<void(std::shared_ptr<WebSocketConnection>, const WebSocketMessage&)>;
using CloseCallback = std::function<void(std::shared_ptr<WebSocketConnection>, uint16_t code, const std::string& reason)>;
using ErrorCallback = std::function<void(std::shared_ptr<WebSocketConnection>, const Result& result)>;
```

## Testing

The project follows Test-Driven Development (TDD):

```bash
# Run all tests
ctest --test-dir build

# Run specific test
./build/tests/aiWebSocketsTests
```

Tests are written to fail first, then implemented to pass.

## Debugging in Windsurf

The project includes LLDB debugging configuration:

1. Set breakpoints in your C++ code
2. Press F5 or use "Run > Start Debugging"
3. Select "Debug WebSocket Server" or "Debug WebSocket Tests"

## WebSocket Protocol Features

- **Handshake**: Complete WebSocket handshake processing
- **Frame Parsing**: Full RFC 6455 frame parsing and generation
- **Message Types**: Text, binary, ping, pong, and close frames
- **Extensions**: Framework for WebSocket extensions
- **Subprotocols**: Support for protocol negotiation
- **Error Handling**: Generous in parsing, strict in sending

## Platform Support

### Windows
- Uses Winsock2 API
- WSAGetLastError() for error reporting
- Visual Studio 2019+ or MinGW-w64

### Linux
- Uses POSIX socket API
- errno for error reporting
- GCC 7+ or Clang 5+

### macOS
- Uses POSIX socket API
- errno for error reporting
- Xcode 10+ or Clang 5+

## Future Enhancements

- TLS/SSL support (extension points ready)
- WebSocket compression
- HTTP/2 upgrade support
- More sophisticated connection pooling
- Performance optimizations

## Contributing

1. Follow the existing code style
2. Write tests before implementation (TDD)
3. No exceptions in the codebase
4. Use C-style I/O for logging
5. Maintain cross-platform compatibility

## License

This project is provided as-is for educational and development purposes.
