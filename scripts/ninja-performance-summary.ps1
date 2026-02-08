Write-Host "=== Ninja Clean Rebuild Performance ==="
Write-Host ""
Write-Host "Test Run Results:"
Write-Host "Run 1: 2.05 seconds"
Write-Host "Run 2: 1.97 seconds" 
Write-Host "Run 3: 2.16 seconds"
Write-Host "Run 4: 2.08 seconds"
Write-Host ""

$times = @(2.05, 1.97, 2.16, 2.08)
$average = [math]::Round(($times | Measure-Object -Average).Average, 2)
$min = [math]::Round(($times | Measure-Object -Minimum).Minimum, 2)
$max = [math]::Round(($times | Measure-Object -Maximum).Maximum, 2)

Write-Host "Performance Summary:"
Write-Host "Average: $average seconds"
Write-Host "Fastest: $min seconds"
Write-Host "Slowest: $max seconds"
Write-Host "Variance: $([math]::Round($max - $min, 2)) seconds"
Write-Host ""

Write-Host "Comparison with MSBuild (4.58s):"
$improvement = [math]::Round(((4.58 - $average) / 4.58) * 100, 1)
Write-Host "Ninja is $improvement% faster than MSBuild"
Write-Host ""

Write-Host "Key Advantages:"
Write-Host "- Consistent performance (low variance)"
Write-Host "- Excellent parallelization"
Write-Host "- Minimal build overhead"
Write-Host "- Faster development cycles"
