# Windsorf Build Setup

This project is configured for optimal development in Windsorf with multiple build system options.

## 🚀 Quick Start

### Option 1: Use the Status Bar (Recommended)
1. **Configure**: Click the build status in the status bar → Select "Ninja Release"
2. **Build**: Click the build status → "Build" (or `Ctrl+Shift+B`)
3. **Run**: Open terminal → `./build-release/aiWebSocketsTests.exe`

### Option 2: Use Command Palette
1. `Ctrl+Shift+P` → "CMake: Configure" → Select "ninja-release"
2. `Ctrl+Shift+P` → "CMake: Build" → Select "ALL_BUILD"
3. `Ctrl+Shift+P` → "Tasks: Run Task" → "Run Tests"

## 🔧 Build Configurations

### Available Presets:
- **ninja-release** (Default) - Fast builds with Ninja
- **ninja-debug** - Debug builds with Ninja  
- **vs2022-debug** - Visual Studio 2022 for IDE integration
- **vs2022-release** - Visual Studio 2022 release builds

### Build Directories:
- `build-release/` - Ninja release builds (recommended)
- `build-debug/` - Ninja debug builds
- `build-vs2022/` - Visual Studio builds
- `build/` - Legacy (do not use)

## 🛠️ Common Tasks

### Build Everything:
```bash
cmake --build build-release --config Release
```

### Run Tests:
```bash
./build-release/aiWebSocketsTests.exe
```

### Clean Build:
```bash
cmake --build build-release --target clean
cmake --build build-release --config Release
```

### Specific Targets:
```bash
# Build just the tests
cmake --build build-release --target aiWebSocketsTests

# Build specific examples
cmake --build build-release --target secure_server_example
cmake --build build-release --target performance_test
```

## 🐛 Troubleshooting

### "Generator does not support instance specification" Error:
This happens when Windsorf tries to use Visual Studio flags with Ninja generator.

**Solution:**
1. Close Windsorf
2. Delete `build/` directory: `Remove-Item -Recurse build`
3. Restart Windsorf
4. Use status bar to select "Ninja Release" preset

### Build Fails with Missing Headers:
1. Run `CMake: Configure` from command palette
2. Select "ninja-release" preset
3. Try building again

### IntelliSense Not Working:
1. Open `CMakePresets.json` to verify presets
2. Run `CMake: Configure` 
3. Restart Windsorf

## 📁 Project Structure

```
aiWebSockets/
├── .vscode/
│   ├── settings.json     - Windsorf settings
│   ├── tasks.json        - Build tasks
│   └── launch.json       - Debug configurations
├── CMakePresets.json     - CMake presets (NEW)
├── build-release/        - Ninja release builds
├── build-debug/          - Ninja debug builds  
└── build-vs2022/         - VS2022 builds
```

## 🎯 Recommended Workflow

1. **Use Ninja builds** for development (faster)
2. **Use VS2022 builds** only for debugging in Visual Studio
3. **Always build after changes** (follow workflow invariants)
4. **Run tests** before committing changes

## 🚨 Important Notes

- **Never use the `build/` directory** - it's legacy
- **Prefer `build-release/` for daily development**
- **Ninja is faster** than Visual Studio builds
- **All executables are in `build-release/`**
- **CMakePresets.json** ensures consistent configuration

## 🔗 Debugging

Use the debug configurations in the Run and Debug panel:
- Debug Unit Tests
- Debug Performance Test  
- Debug WebSocket Server
- Debug Large Data Test
- Debug Event Loop Test

All debug configurations automatically build before running.
