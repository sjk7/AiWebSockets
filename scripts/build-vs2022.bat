@echo off
REM Build script specifically for Visual Studio 2022

echo Building with Visual Studio 2022...

REM Navigate to project root
cd /d "%~dp0\.."

REM Clean build directory
if exist build (
    echo Cleaning existing build directory...
    rmdir /s /q build
)

REM Configure CMake for VS 2022
echo Configuring CMake for Visual Studio 2022...
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 -DCMAKE_BUILD_TYPE=Debug
if %ERRORLEVEL% neq 0 (
    echo CMake configuration failed for VS 2022!
    exit /b 1
)

REM Build the project
echo Building project with VS 2022 (warnings as errors enabled)...
cmake --build build --config Debug -- /p:TreatWarningsAsErrors=true
if %ERRORLEVEL% neq 0 (
    echo Build failed with VS 2022 (warnings treated as errors)!
    exit /b 1
)

echo Build successful with Visual Studio 2022!

REM Run tests
echo Running tests...
build\Debug\aiWebSocketsTests.exe
if %ERRORLEVEL% neq 0 (
    echo Tests failed!
    exit /b 1
)

echo All tests passed with VS 2022!

REM Commit changes
echo Committing changes...
git add .
git commit -m "Auto-commit after VS 2022 build and tests - warning-free"

echo VS 2022 build and commit process completed!
