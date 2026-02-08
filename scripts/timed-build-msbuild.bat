@echo off
REM Time MSBuild build for comparison

echo Timing MSBuild build...

REM Navigate to project root
cd /d "%~dp0\.."

REM Clean build directory
if exist build (
    echo Cleaning existing build directory...
    rmdir /s /q build
)

REM Configure CMake first
echo Configuring CMake with MSBuild...
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 -DCMAKE_BUILD_TYPE=Debug
if %ERRORLEVEL% neq 0 (
    echo CMake configuration failed!
    exit /b 1
)

REM Start timing
echo Starting MSBuild build timer...
powershell -Command "Measure-Command { cmake --build build --config Debug -- /p:TreatWarningsAsErrors=true } | Select-Object TotalSeconds" > msbuild-time.txt

echo MSBuild build completed. Time saved to msbuild-time.txt
type msbuild-time.txt
