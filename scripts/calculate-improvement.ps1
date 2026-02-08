$msbuild = 4.5751697
$ninja = 2.0522168
$improvement = [math]::Round((($msbuild - $ninja) / $msbuild) * 100, 1)

Write-Host "=== Build Performance Comparison ==="
Write-Host "MSBuild Time: 4.58 seconds"
Write-Host "Ninja Time: 2.05 seconds"
Write-Host "Performance Improvement: $improvement% faster"
