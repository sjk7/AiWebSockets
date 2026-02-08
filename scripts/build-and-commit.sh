#!/bin/bash

# Build and commit script
# This script builds the project and commits changes if successful

set -e  # Exit on any error

echo "Starting build process..."

# Navigate to project root
cd "$(dirname "$0")/.."

# Clean build directory
if [ -d "build" ]; then
    echo "Cleaning existing build directory..."
    rm -rf build
fi

# Configure CMake
echo "Configuring CMake..."
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug

# Build the project
echo "Building project..."
if cmake --build build; then
    echo "✅ Build successful!"
    
    # Run tests to verify everything works
    echo "Running tests..."
    if build/Debug/aiWebSocketsTests.exe; then
        echo "✅ All tests passed!"
        
        # Commit changes
        echo "Committing changes..."
        git add .
        TIMESTAMP=$(date +"%Y-%m-%d %H:%M:%S")
        git commit -m "Auto-commit after successful build and tests - $TIMESTAMP"
        
        echo "✅ Changes committed successfully!"
        echo "📊 Git status:"
        git log --oneline -5
    else
        echo "❌ Tests failed - not committing"
        exit 1
    fi
else
    echo "❌ Build failed - not committing"
    exit 1
fi
