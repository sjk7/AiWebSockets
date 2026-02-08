@echo off
REM Clean build with warnings as errors

echo Building with warnings as errors...

REM Navigate to project root
cd /d "%~dp0\.."

REM Clean build directory
if exist build (
    echo Cleaning existing build directory...
    rmdir /s /q build
)

REM Configure CMake for VS 2022
echo Configuring CMake...
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 -DCMAKE_BUILD_TYPE=Debug
if %ERRORLEVEL% neq 0 (
    echo CMake configuration failed!
    exit /b 1
)

REM Build with warnings as errors
echo Building with warnings as errors...
cmake --build build --config Debug -- /p:TreatWarningsAsErrors=true
if %ERRORLEVEL% neq 0 (
    echo Build failed - warnings treated as errors!
    exit /b 1
)

echo Build successful with no warnings!

REM Run tests
echo Running tests...
build\Debug\aiWebSocketsTests.exe
if %ERRORLEVEL% neq 0 (
    echo Tests failed!
    exit /b 1
)

echo All tests passed!

REM Commit
git add .
git commit -m "Warning-free build and tests"

echo Clean build process completed!
