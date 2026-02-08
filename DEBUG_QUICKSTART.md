# 🚀 CodeLLDB Debugging - Quick Start

## Immediate Debugging Steps

### 1. Install CodeLLDB Extension
```
VS Code → Extensions → Search "CodeLLDB" → Install
```

### 2. Set a Breakpoint
- Open `examples/performance_test.cpp`
- Click next to line 64: `std::vector<uint8_t> testData = WebSocket::CreateTestData(dataSize);`
- Red dot should appear

### 3. Start Debugging
- Press `F5` or click Run → Start Debugging
- Select "Debug Performance Test (CodeLLDB)"
- Debugger will build and launch

### 4. Debug Controls
- `F5` - Continue
- `F10` - Step Over  
- `F11` - Step Into
- `Shift+F11` - Step Out

## Debugging Scenarios

### 🎯 Performance Analysis
```cpp
// Breakpoint in MeasureTransfer function
TestResult MeasureTransfer(size_t dataSize, size_t chunkSize = 65536) {
    // 👆 Set breakpoint here
    TestResult result{"", dataSize, 0.0, 0.0, 0.0, false};
    std::vector<uint8_t> testData = WebSocket::CreateTestData(dataSize);
    // Watch variables: dataSize, testData.size()
}
```

### 🔍 Socket Operations
```cpp
// Breakpoint in Socket.cpp SendRaw function
std::pair<Result, size_t> Socket::SendRaw(const void* data, size_t length) {
    // 👆 Set breakpoint here to debug data transmission
    printf("DEBUG: Sending %zu bytes...\n", length);
    // Watch: data, length, totalSent
}
```

### 📊 Data Integrity
```cpp
// Breakpoint in TestUtilities.h VerifyDataIntegrity
inline bool VerifyDataIntegrity(const std::vector<uint8_t>& receivedData, size_t expectedSize) {
    // 👆 Set breakpoint here to catch data corruption
    if (receivedData.size() != expectedSize) {
        return false; // Investigate size mismatch
    }
    // Watch: receivedData, expectedSize, i
}
```

## Available Debug Configurations

1. **Debug Performance Test** - Analyze throughput and timing
2. **Debug Large Data Test** - Debug 300MB transfers
3. **Debug Unit Tests** - Debug test failures
4. **Debug WebSocket Server** - Server functionality
5. **Debug Event Loop** - Event loop behavior
6. **Debug Server-to-Client** - Bidirectional transfers

## Quick Debugging Workflow

### For Performance Issues:
1. Set breakpoint in `MeasureTransfer`
2. Watch `dataSize` and `chunkSize` variables
3. Step through send/receive loops
4. Analyze timing measurements

### For Socket Issues:
1. Set breakpoints in `Socket::Connect`, `Socket::Accept`
2. Check socket state and error codes
3. Verify network parameters

### For Data Issues:
1. Set breakpoint in `VerifyDataIntegrity`
2. Compare sent vs received data
3. Check buffer boundaries

## Pro Tips

### Conditional Breakpoints
```cpp
// Right-click breakpoint → Edit Breakpoint → Condition
dataSize > 1024 * 1024  // Only break for >1MB transfers
```

### Watch Window
- Add expressions: `throughputMBps`, `bytesTransferred`
- Monitor: `testData.size()`, `receivedData.size()`

### Debug Console
```lldb
p dataSize              # Print data size
p testData.size()       # Print vector size
p result.throughputMBps # Print throughput
```

## Troubleshooting

**"Can't start debugging"**
- Install CodeLLDB extension
- Ensure build-release exists

**"Breakpoints not hitting"**
- Check Release vs Debug build
- Verify source file paths

**"Symbols not loaded"**
- Build with debug symbols
- Check PDB file generation

---

**Ready to debug! 🎯**

1. Set breakpoint in `performance_test.cpp` line 64
2. Press `F5`
3. Select "Debug Performance Test (CodeLLDB)"
4. Start debugging your WebSocket implementation!
