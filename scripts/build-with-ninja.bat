@echo off
REM Build with Ninja generator (faster parallel builds)

echo Checking for Ninja...
where ninja >nul 2>&1
if %ERRORLEVEL% neq 0 (
    echo Ninja not found. Please install Ninja from:
    echo https://github.com/ninja-build/ninja/releases
    echo Or via: choco install ninja
    echo Falling back to Visual Studio generator...
    goto :vs_build
)

echo Found Ninja! Using Ninja generator for faster builds...

REM Navigate to project root
cd /d "%~dp0\.."

REM Clean build directory
if exist build (
    echo Cleaning existing build directory...
    rmdir /s /q build
)

REM Configure CMake with Ninja
echo Configuring CMake with Ninja...
cmake -S . -B build -G "Ninja" -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_COMPILER=cl
if %ERRORLEVEL% neq 0 (
    echo CMake configuration failed with Ninja!
    exit /b 1
)

REM Build with Ninja (automatically parallel)
echo Building with Ninja...
cmake --build build
if %ERRORLEVEL% neq 0 (
    echo Build failed with Ninja!
    exit /b 1
)

echo Build successful with Ninja!

REM Run tests
echo Running tests...
build\aiWebSocketsTests.exe
if %ERRORLEVEL% neq 0 (
    echo Tests failed!
    exit /b 1
)

echo All tests passed with Ninja!
goto :end

:vs_build
echo Falling back to Visual Studio build...
call scripts\build-clean.bat

:end
echo Build process completed!
