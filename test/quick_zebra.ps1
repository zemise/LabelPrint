# Quick test: build demo -> generate ZPL with Zebra profile -> print on ZD888
param(
    [string]$PrinterName = "ZDesigner ZD888-203dpi ZPL"
)

$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $PSScriptRoot

Write-Host "=== Build ===" -ForegroundColor Cyan
& "C:\Program Files\Microsoft Visual Studio\18\Community\Common7\Tools\Launch-VsDevShell.ps1" -SkipAutomaticLocation
cmake --build "$root\out\build\x64-Debug" 2>&1
if ($LASTEXITCODE -ne 0) { throw "Build failed" }

Write-Host "=== Generate Zebra ZPL ===" -ForegroundColor Cyan
Push-Location $root
& "$root\out\build\x64-Debug\labelprint_demo.exe" --profile zebra_zd888 2>&1
Pop-Location
if ($LASTEXITCODE -ne 0) { throw "Generator failed" }

Write-Host "=== Print ZPL ===" -ForegroundColor Cyan
& "$root\RawZPL.ps1" -PrinterName $PrinterName -ZplFile "label_medical.zpl"

Write-Host "Done." -ForegroundColor Green
