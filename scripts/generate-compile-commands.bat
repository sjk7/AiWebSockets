@echo off
REM Generate compilation database for IDE

echo Generating compilation database...

REM Navigate to project root
cd /d "%~dp0\.."

REM Clean build directory
if exist build (
    echo Cleaning existing build directory...
    rmdir /s /q build
)

REM Setup Visual Studio environment
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"

REM Configure CMake with Ninja and compile commands
echo Configuring CMake with compile commands...
cmake -S . -B build -G "Ninja" -DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
if %ERRORLEVEL% neq 0 (
    echo CMake configuration failed!
    exit /b 1
)

REM Check if compile_commands.json was created
if exist build\compile_commands.json (
    echo ✅ compile_commands.json generated successfully!
    echo Copying to project root for IDE...
    copy build\compile_commands.json compile_commands.json
    echo ✅ compilation database ready for IDE
) else (
    echo ❌ compile_commands.json not found
)

echo Done!
