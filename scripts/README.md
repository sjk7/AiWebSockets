# Build and Commit Scripts

This directory contains automated build and commit scripts for the aiWebSockets project.

## Available Scripts

### 1. PowerShell Script (Recommended)
```powershell
.\scripts\build-and-commit.ps1
```

**Features:**
- ✅ Builds project in Release mode
- ✅ Runs all unit tests
- ✅ Verifies performance test functionality
- ✅ Verifies large data test functionality
- ✅ Auto-commits changes only if all checks pass
- ✅ Detailed logging and progress indicators
- ✅ Cross-platform compatible

### 2. Batch Script (Windows CMD)
```cmd
.\scripts\build-and-commit-simple.bat
```

**Features:**
- ✅ Builds project in Release mode
- ✅ Runs all unit tests
- ✅ Verifies performance test functionality
- ✅ Auto-commits changes only if tests pass
- ✅ Simple and reliable

## Usage

### Method 1: Direct Execution
```powershell
# From project root
.\scripts\build-and-commit.ps1
```

### Method 2: PowerShell Function (Add to your profile)
Add this to your PowerShell profile (`$PROFILE`):
```powershell
function Build-Commit {
    Set-Location "C:\Users\cool\Desktop\aiWebSockets"
    .\scripts\build-and-commit.ps1
}
```

Then simply run:
```powershell
Build-Commit
```

### Method 3: Git Alias
Add to your `.gitconfig`:
```ini
[alias]
    build-commit = "!f() { cd \"$(git rev-parse --show-toplevel)\" && powershell -ExecutionPolicy Bypass -File scripts/build-and-commit.ps1; }; f"
```

Then run from anywhere in the repo:
```bash
git build-commit
```

## What the Scripts Do

### Build Process
1. **Configure**: Uses existing CMake configuration
2. **Build**: Builds in Release mode with optimizations
3. **Test**: Runs all unit tests (53 tests)
4. **Verify**: Runs performance and large data tests
5. **Commit**: Only commits if all checks pass

### Verification Steps
- ✅ **Unit Tests**: All 53 core library tests
- ✅ **Performance Test**: Validates throughput and data integrity
- ✅ **Large Data Test**: Validates 300MB bidirectional transfers
- ✅ **Build Success**: Release mode compilation

### Commit Message Format
```
Auto-commit after successful build and tests - YYYY-MM-DD HH:MM:SS

Build: Release mode
Tests: Unit tests passed
Performance: Verified
Large Data: Verified

Build system: CMake + MSVC
Platform: Windows x64
```

## Safety Features

### ✅ Safe Committing
- **Only commits on successful build**
- **Only commits if all tests pass**
- **Preserves code quality**
- **Maintains project stability**

### ✅ Error Handling
- **Build failures**: No commit, clear error message
- **Test failures**: No commit, detailed failure info
- **Git issues**: Graceful handling, no data loss

### ✅ Performance Monitoring
- **Build time tracking**
- **Test execution monitoring**
- **Performance regression detection**
- **Large data transfer validation**

## Troubleshooting

### Common Issues

#### "Execution Policy" Error (PowerShell)
```powershell
Set-ExecutionPolicy -ExecutionPolicy RemoteSigned -Scope CurrentUser
```

#### "Build failed" Error
- Check CMake configuration
- Verify Visual Studio installation
- Check for missing dependencies

#### "Tests failed" Error
- Run tests manually: `.\build-release\Release\aiWebSocketsTests.exe`
- Check for platform-specific issues
- Verify test environment

### Manual Override
If you need to commit without running tests (not recommended):
```bash
git add -A
git commit -m "Manual commit - bypassing build verification"
```

## Integration with IDE

### Visual Studio Code
Add to `.vscode/tasks.json`:
```json
{
    "label": "Build and Commit",
    "type": "shell",
    "command": "powershell",
    "args": ["-ExecutionPolicy", "Bypass", "-File", "${workspaceFolder}/scripts/build-and-commit.ps1"],
    "group": "build",
    "presentation": {
        "echo": true,
        "reveal": "always",
        "focus": false,
        "panel": "new"
    }
}
```

### Visual Studio
Add as external tool:
- **Title**: Build and Commit
- **Command**: powershell.exe
- **Arguments**: -ExecutionPolicy Bypass -File $(SolutionDir)scripts\build-and-commit.ps1
- **Initial Directory**: $(SolutionDir)

## Best Practices

1. **Run after significant changes**: New features, bug fixes, optimizations
2. **Before releases**: Ensure code quality and stability
3. **After refactoring**: Verify no regressions introduced
4. **Regular intervals**: Maintain clean commit history

## Performance Impact

- **Build Time**: ~30-60 seconds
- **Test Time**: ~10-20 seconds
- **Verification Time**: ~5-10 seconds
- **Total Time**: ~45-90 seconds

The scripts ensure every commit represents:
- ✅ **Working code**
- ✅ **Passing tests**
- ✅ **Verified performance**
- ✅ **Validated data integrity**

This maintains high code quality and project reliability! 🚀
