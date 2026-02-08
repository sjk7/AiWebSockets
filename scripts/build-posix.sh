#!/bin/bash

# POSIX build script using Ninja for optimal performance

echo "POSIX build with Ninja..."

# Navigate to project root
cd "$(dirname "$0")/.."

# Check for Ninja
if ! command -v ninja &> /dev/null; then
    echo "Ninja not found. Installing Ninja..."
    if command -v apt-get &> /dev/null; then
        sudo apt-get update && sudo apt-get install -y ninja-build
    elif command -v yum &> /dev/null; then
        sudo yum install -y ninja-build
    elif command -v brew &> /dev/null; then
        brew install ninja
    else
        echo "Please install Ninja manually"
        exit 1
    fi
fi

echo "Using Ninja generator for optimal performance..."

# Clean build directory
if [ -d "build" ]; then
    echo "Cleaning existing build directory..."
    rm -rf build
fi

# Configure CMake with Ninja
echo "Configuring CMake with Ninja..."
cmake -S . -B build -G "Ninja" -DCMAKE_BUILD_TYPE=Debug
if [ $? -ne 0 ]; then
    echo "CMake configuration failed!"
    exit 1
fi

# Build with Ninja (automatically uses all available cores)
echo "Building with Ninja..."
ninja -C build
if [ $? -ne 0 ]; then
    echo "Build failed!"
    exit 1
fi

echo "Build successful with Ninja!"

# Run tests
echo "Running tests..."
build/aiWebSocketsTests
if [ $? -ne 0 ]; then
    echo "Tests failed!"
    exit 1
fi

echo "All tests passed!"

# Commit changes
echo "Committing changes..."
git add .
git commit -m "POSIX build with Ninja - $(date)"

echo "POSIX build process completed!"
