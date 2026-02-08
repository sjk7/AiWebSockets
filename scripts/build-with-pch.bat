@echo off
REM Build with Precompiled Headers (for future use when project grows)

echo Building with Precompiled Headers...

REM Navigate to project root
cd /d "%~dp0\.."

REM Clean build directory
if exist build (
    echo Cleaning existing build directory...
    rmdir /s /q build
)

REM Setup Visual Studio environment
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"

REM Configure CMake with PCH enabled
echo Configuring CMake with PCH...
cmake -S . -B build -G "Ninja" -DCMAKE_BUILD_TYPE=Debug -DENABLE_PCH=ON
if %ERRORLEVEL% neq 0 (
    echo CMake configuration failed with PCH!
    exit /b 1
)

REM Build with PCH
echo Building with PCH...
cmake --build build
if %ERRORLEVEL% neq 0 (
    echo Build failed with PCH!
    exit /b 1
)

echo Build successful with PCH!

REM Run tests
echo Running tests...
build\aiWebSocketsTests.exe
if %ERRORLEVEL% neq 0 (
    echo Tests failed!
    exit /b 1
)

echo All tests passed with PCH!

REM Commit changes
git add .
git commit -m "Build with Precompiled Headers enabled"

echo PCH build process completed!
