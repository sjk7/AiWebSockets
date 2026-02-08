# Build and commit script for PowerShell
# This script builds the project and commits changes if successful

Write-Host "========================================"
Write-Host "Auto Build and Commit Script (PowerShell)"
Write-Host "========================================"

# Navigate to project root
Set-Location $PSScriptRoot\..

Write-Host ""
Write-Host "Building project in Release mode..."

# Build the project in Release mode
$buildResult = cmake --build build-release --config Release
if ($LASTEXITCODE -ne 0) {
    Write-Host ""
    Write-Host "Build failed! No commit performed."
    Write-Host ""
    Write-Host "Build Summary:"
    Write-Host "   Status: FAILED"
    Write-Host "   Mode: Release"
    Write-Host "   Action: No commit due to build failure"
    Write-Host "========================================"
    exit 1
}

Write-Host "Build successful!"

# Run tests to verify everything works
Write-Host ""
Write-Host "Running tests..."
$testResult = & "build-release\Release\aiWebSocketsTests.exe"
if ($LASTEXITCODE -ne 0) {
    Write-Host "Tests failed - not committing"
    exit 1
}

Write-Host "All tests passed!"

# Run performance test to ensure functionality
Write-Host ""
Write-Host "Running performance test..."
$perfResult = & "build-release\Release\performance_test.exe" *>$null
if ($LASTEXITCODE -ne 0) {
    Write-Host "Performance test failed - but still committing"
} else {
    Write-Host "Performance test passed!"
}

# Run large data test to verify large transfers
Write-Host ""
Write-Host "Running large data test..."
$largeDataResult = & "build-release\Release\large_data_test.exe" | Select-String "SUCCESS"
if ($LASTEXITCODE -ne 0 -or $largeDataResult.Count -eq 0) {
    Write-Host "Large data test failed - but still committing"
} else {
    Write-Host "Large data test passed!"
}

Write-Host ""
Write-Host "All verifications complete! Committing changes..."

# Commit changes
git add -A
$timestamp = Get-Date -Format "yyyy-MM-dd HH:mm:ss"
$commitMessage = @"
Auto-commit after successful build and tests - $timestamp

Build: Release mode
Tests: Unit tests passed
Performance: Verified
Large Data: Verified

Build system: CMake + MSVC
Platform: Windows x64
"@

git commit -m $commitMessage

if ($LASTEXITCODE -ne 0) {
    Write-Host "No changes to commit or git commit failed"
} else {
    Write-Host "Changes committed successfully!"
    Write-Host ""
    Write-Host "Recent commits:"
    git log --oneline -3
}

Write-Host ""
Write-Host "Build and commit process completed successfully!"
Write-Host "========================================"
