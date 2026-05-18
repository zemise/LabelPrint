# Quick test: build C++ → generate TSPL → print ASCII label
param(
    [string]$PrinterName = "Xprinter XP-360B #2"
)

$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $PSScriptRoot

Write-Host "=== Build ===" -ForegroundColor Cyan
& "C:\Program Files\Microsoft Visual Studio\18\Community\Common7\Tools\Launch-VsDevShell.ps1" -SkipAutomaticLocation
cmake --build "$root\out\build\x64-Debug" 2>&1
if ($LASTEXITCODE -ne 0) { throw "Build failed" }

Write-Host "=== Generate ===" -ForegroundColor Cyan
Push-Location $root
& "$root\out\build\x64-Debug\labelprint_demo.exe" 2>&1
Pop-Location

Write-Host "=== Print ===" -ForegroundColor Cyan
& "$root\RawTSPL.ps1" -PrinterName $PrinterName -TsplFile "label_medical.tspl"

Write-Host "Done." -ForegroundColor Green
