# Quick test: C++ generate Chinese TSPL → print Chinese label
param(
    [string]$PrinterName = "Xprinter XP-360B #2"
)

$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $PSScriptRoot

Write-Host "=== Generate Chinese TSPL (C++ TsplGb18030Backend) ===" -ForegroundColor Cyan
Push-Location $root
$output = & "$root\out\build\x64-Debug\labelprint_demo.exe" 2>&1
Pop-Location
if ($LASTEXITCODE -ne 0) { throw "Generator failed" }
Write-Host "       OK"

Write-Host "=== Print ===" -ForegroundColor Cyan
& "$root\RawTSPL.ps1" -PrinterName $PrinterName -TsplFile "label_medical_cn_new.prn"

Write-Host "Done." -ForegroundColor Green
