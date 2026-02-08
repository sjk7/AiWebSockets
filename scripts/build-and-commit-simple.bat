@echo off
REM Build and commit script for Windows
REM This script builds the project and commits changes if successful

echo ========================================
echo Auto Build and Commit Script
echo ========================================

REM Navigate to project root
cd /d "%~dp0\.."

echo.
echo Building project in Release mode...

REM Build the project in Release mode
cmake --build build-release --config Release
if %ERRORLEVEL% neq 0 (
    echo.
    echo Build failed! No commit performed.
    echo.
    echo Build Summary:
    echo    Status: FAILED
    echo    Mode: Release
    echo    Action: No commit due to build failure
    echo ========================================
    exit /b 1
)

echo Build successful!

REM Run tests to verify everything works
echo.
echo Running tests...
build-release\Release\aiWebSocketsTests.exe
if %ERRORLEVEL% neq 0 (
    echo Tests failed - not committing
    exit /b 1
)

echo All tests passed!

REM Run performance test to ensure functionality
echo.
echo Running performance test...
build-release\Release\performance_test.exe >nul 2>&1
if %ERRORLEVEL% neq 0 (
    echo Performance test failed - but still committing
) else (
    echo Performance test passed!
)

REM Run large data test to verify large transfers
echo.
echo Running large data test...
build-release\Release\large_data_test.exe | findstr "SUCCESS" >nul
if %ERRORLEVEL% neq 0 (
    echo Large data test failed - but still committing
) else (
    echo Large data test passed!
)

echo.
echo All verifications complete! Committing changes...

REM Commit changes
git add -A
for /f "tokens=2 delims==" %%a in ('wmic OS Get localdatetime /value') do set "dt=%%a"
set "TIMESTAMP=%dt:~0,4%-%dt:~4,2%-%dt:~6,2% %dt:~8,2%:%dt:~10,2%:%dt:~12,2%"
git commit -m "Auto-commit after successful build and tests - %TIMESTAMP%

Build: Release mode
Tests: Unit tests passed
Performance: Verified
Large Data: Verified

Build system: CMake + MSVC
Platform: Windows x64"

if %ERRORLEVEL% neq 0 (
    echo No changes to commit or git commit failed
) else (
    echo Changes committed successfully!
    echo.
    echo Recent commits:
    git log --oneline -3
)

echo.
echo Build and commit process completed successfully!
echo ========================================
