@echo off
REM Build script specifically for Visual Studio 2026 Insiders

echo Building with Visual Studio 2026 Insiders...

REM Navigate to project root
cd /d "%~dp0\.."

REM Clean build directory
if exist build (
    echo Cleaning existing build directory...
    rmdir /s /q build
)

REM Configure CMake for VS 2026 Insiders
echo Configuring CMake for Visual Studio 2026 Insiders...
cmake -S . -B build -G "Visual Studio 18 2026" -A x64 -DCMAKE_BUILD_TYPE=Debug
if %ERRORLEVEL% neq 0 (
    echo CMake configuration failed for VS 2026 Insiders!
    exit /b 1
)

REM Build the project
echo Building project with VS 2026 Insiders (warnings as errors enabled)...
cmake --build build --config Debug -- /p:TreatWarningsAsErrors=true
if %ERRORLEVEL% neq 0 (
    echo Build failed with VS 2026 Insiders (warnings treated as errors)!
    exit /b 1
)

echo Build successful with Visual Studio 2026 Insiders!

REM Run tests
echo Running tests...
build\Debug\aiWebSocketsTests.exe
if %ERRORLEVEL% neq 0 (
    echo Tests failed!
    exit /b 1
)

echo All tests passed with VS 2026 Insiders!

REM Commit changes
echo Committing changes...
git add .
for /f "tokens=2 delims==" %%I in ('wmic OS Get localdatetime /value') do set "dt=%%I"
set "TIMESTAMP=%dt:~0,4%-%dt:~6,2%-%dt:~8,2% %dt:~10,2%:%dt:~12,2%:%dt:~14,2%"
git commit -m "Auto-commit after VS 2026 Insiders build and tests - %TIMESTAMP%"

echo VS 2026 Insiders build and commit process completed!
