@echo off
REM Time Ninja build with VS environment

echo Timing Ninja build with VS environment...

REM Navigate to project root
cd /d "%~dp0\.."

REM Setup Visual Studio environment
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"

REM Clean build directory
if exist build (
    echo Cleaning existing build directory...
    rmdir /s /q build
)

REM Configure CMake with Ninja
echo Configuring CMake with Ninja...
cmake -S . -B build -G "Ninja" -DCMAKE_BUILD_TYPE=Debug
if %ERRORLEVEL% neq 0 (
    echo CMake configuration failed with Ninja!
    exit /b 1
)

REM Start timing
echo Starting Ninja build timer...
powershell -Command "Measure-Command { cmake --build build } | Select-Object TotalSeconds" > ninja-time.txt

echo Ninja build completed. Time saved to ninja-time.txt
type ninja-time.txt
