# CodeLLDB Debugging Guide for aiWebSockets

This guide explains how to debug the aiWebSockets project using CodeLLDB in Visual Studio Code.

## Prerequisites

### Required Extensions
1. **CodeLLDB** - Install from VS Code marketplace
2. **C/C++** - Microsoft's C/C++ extension for IntelliSense
3. **CMake** - Official CMake extension (optional but recommended)

### Installation
```bash
# In VS Code, install CodeLLDB extension
Ctrl+Shift+X -> Search "CodeLLDB" -> Install
```

## Debugging Configurations

### Available Debug Configurations

1. **Debug Performance Test (CodeLLDB)**
   - File: `performance_test.exe`
   - Purpose: Debug performance measurement and throughput analysis

2. **Debug Large Data Test (CodeLLDB)**
   - File: `large_data_test.exe`
   - Purpose: Debug large data transfers (300MB bidirectional)

3. **Debug WebSocket Server (CodeLLDB)**
   - File: `aiWebSocketsServer.exe`
   - Purpose: Debug WebSocket server functionality

4. **Debug Unit Tests (CodeLLDB)**
   - File: `aiWebSocketsTests.exe`
   - Purpose: Debug unit test failures

5. **Debug Event Loop Test (CodeLLDB)**
   - File: `event_loop_test.exe`
   - Purpose: Debug event loop functionality

6. **Debug Server-to-Client Test (CodeLLDB)**
   - File: `server_to_client_test.exe`
   - Purpose: Debug server-initiated transfers

## How to Debug

### Step 1: Set Breakpoints
1. Open any source file (`.cpp` or `.h`)
2. Click in the gutter to the left of line numbers
3. Red dot appears = breakpoint set

### Step 2: Start Debugging
1. Press `F5` or go to **Run → Start Debugging**
2. Select debug configuration from dropdown
3. CodeLLDB will build and launch the target

### Step 3: Debug Controls
- **F5**: Continue/Start
- **F10**: Step Over
- **F11**: Step Into
- **Shift+F11**: Step Out
- **F9**: Toggle Breakpoint
- **Ctrl+Shift+F5**: Restart

## Debugging Scenarios

### Performance Analysis
```cpp
// Set breakpoint in performance_test.cpp at MeasureTransfer
TestResult MeasureTransfer(size_t dataSize, size_t chunkSize = 65536) {
    // Break here to analyze performance characteristics
    TestResult result{"", dataSize, 0.0, 0.0, 0.0, false};
    
    // Debug test data creation
    std::vector<uint8_t> testData = WebSocket::CreateTestData(dataSize);
    
    // Step through socket operations
    Socket serverSocket, clientSocket;
    // ...
}
```

### Socket Communication Debugging
```cpp
// Breakpoints in Socket.cpp for send/receive operations
std::pair<Result, size_t> Socket::SendRaw(const void* data, size_t length) {
    // Debug data transmission
    printf("DEBUG: Send progress: %zu bytes this call, %zu total\n", (size_t)result, totalSent);
    
    // Step through network operations
    int result = send(m_socket, buffer + totalSent, (int)(length - totalSent), 0);
    // ...
}
```

### Data Integrity Verification
```cpp
// Breakpoints in TestUtilities.h for data verification
inline bool VerifyDataIntegrity(const std::vector<uint8_t>& receivedData, size_t expectedSize) {
    // Debug data integrity checks
    if (receivedData.size() != expectedSize) {
        return false; // Break here to investigate size mismatch
    }
    
    for (size_t i = 0; i < expectedSize; i++) {
        if (receivedData[i] != static_cast<uint8_t>(i % 256)) {
            return false; // Break here to investigate data corruption
        }
    }
    // ...
}
```

## Advanced Debugging Features

### 1. Conditional Breakpoints
```cpp
// Right-click breakpoint → Edit Breakpoint → Condition
// Example: Break only on large data transfers
dataSize > 1024 * 1024  // Break when data > 1MB
```

### 2. Watch Variables
- **Variables Panel**: View local variables
- **Watch Panel**: Add custom expressions
- **Memory View**: Inspect raw memory

### 3. Call Stack
- Navigate through function calls
- See calling context
- Jump to different stack frames

### 4. Debug Console
```lldb
# LLDB commands in debug console
p variable_name        # Print variable
p *pointer            # Dereference pointer
p array[0-10]         # Show array elements
thread list           # List threads
bt                    # Backtrace
```

## Common Debugging Workflows

### 1. Performance Bottleneck Analysis
1. Set breakpoints in `MeasureTransfer` function
2. Use performance counters in watch window
3. Step through send/receive loops
4. Analyze timing measurements

### 2. Socket Connection Issues
1. Break in `Socket::Connect` and `Socket::Accept`
2. Check socket state and error codes
3. Verify network parameters
4. Debug handshake process

### 3. Data Corruption Investigation
1. Break in `VerifyDataIntegrity` function
2. Compare sent vs received data
3. Check buffer boundaries
4. Analyze memory layout

### 4. Memory Leak Detection
1. Use LLDB's memory debugging features
2. Set breakpoints on allocation/deallocation
3. Track object lifecycles
4. Verify RAII patterns

## Debugging Tips

### Performance Debugging
```cpp
// Use timing breakpoints
auto startTime = std::chrono::high_resolution_clock::now();
// ... code to measure
auto endTime = std::chrono::high_resolution_clock::now();
// Set breakpoint here to analyze timing
```

### Network Debugging
```cpp
// Debug socket states
printf("DEBUG: Socket state: %s, Error: %s\n", 
       IsValid() ? "Valid" : "Invalid", 
       GetLastSystemError().c_str());
```

### Threading Debugging
```cpp
// Debug thread synchronization
std::cout << "DEBUG: Thread " << std::this_thread::get_id() 
          << " entering critical section" << std::endl;
```

## Troubleshooting

### Common Issues

#### "Unable to start debugging"
- Ensure CodeLLDB extension is installed
- Check build configuration exists
- Verify executable path is correct

#### "Breakpoints not hitting"
- Ensure debug symbols are generated
- Check Release vs Debug build
- Verify source file paths match

#### "Symbols not loaded"
- Build with debug information
- Check PDB file generation
- Verify source paths

### Solutions

#### Enable Debug Symbols
```cmake
# In CMakeLists.txt
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_options(aiWebSockets PRIVATE -g)
endif()
```

#### Fix Path Issues
```json
// In launch.json, ensure correct paths
"program": "${workspaceFolder}/build-release/Release/performance_test.exe"
```

## Integration with Git Workflow

### Debug Before Commit
1. Run debug session on critical tests
2. Verify all breakpoints work as expected
3. Check performance characteristics
4. Use automated build-and-commit script

### Debug After Changes
1. Set breakpoints on modified code
2. Run relevant test configurations
3. Verify behavior matches expectations
4. Commit with confidence

## Best Practices

### 1. Strategic Breakpoints
- Set breakpoints at function entry/exit
- Use conditional breakpoints for specific scenarios
- Focus on critical paths and error conditions

### 2. Watch Expressions
- Monitor key variables (throughput, error rates)
- Track object states during operations
- Use expressions for complex calculations

### 3. Performance Analysis
- Use timing breakpoints for performance measurement
- Profile critical sections
- Identify bottlenecks through step-through

### 4. Systematic Testing
- Test all debug configurations
- Verify breakpoints in different scenarios
- Document debugging workflows

## Advanced Features

### 1. Remote Debugging (Future)
- Debug on remote machines
- Network performance analysis
- Cross-platform debugging

### 2. Core Dump Analysis
- Analyze crash dumps
- Post-mortem debugging
- Crash reproduction

### 3. Performance Profiling
- Integration with performance tools
- Real-time performance monitoring
- Bottleneck identification

---

**With CodeLLDB debugging configured, you now have comprehensive debugging capabilities for the aiWebSockets project!** 🚀

Every aspect of the WebSocket implementation can be analyzed, from low-level socket operations to high-level performance characteristics.
