# Build Scripts

This directory contains build scripts for the aiWebSockets project.

## Available Scripts

### Windows Scripts

#### 1. build-vs2022.bat
```cmd
.\scripts\build-vs2022.bat
```
Builds the project using Visual Studio 2022 with Ninja generator.

#### 2. build-with-ninja.bat
```cmd
.\scripts\build-with-ninja.bat
```
Builds the project using Ninja build system for optimal performance.

#### 3. build-with-pch.bat
```cmd
.\scripts\build-with-pch.bat
```
Builds the project with precompiled headers enabled for faster builds.

#### 4. build-clean.bat
```cmd
.\scripts\build-clean.bat
```
Performs a clean build by removing existing build directory first.

### POSIX Scripts

#### 5. build-posix.sh
```bash
./scripts/build-posix.sh
```
Cross-platform build script for Linux/macOS systems.

## Usage

1. Navigate to project root directory
2. Run the appropriate script for your system and preferences
3. All scripts will:
   - Configure CMake if needed
   - Build the project
   - Run tests
   - Report success/failure

## Requirements

- **CMake 3.15+**
- **C++17 compatible compiler**
- **Ninja** (recommended for performance)
- **Visual Studio 2022** (for Windows builds)

## Notes

- Scripts automatically create build directories
- Failed builds will stop execution with error messages
- Test results are displayed after successful builds
