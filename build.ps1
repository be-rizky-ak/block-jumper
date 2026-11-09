<#
.SYNOPSIS
    Block Jumper - Windows PowerShell Build Script
.DESCRIPTION
    Configures and compiles Block Jumper using CMake and Visual Studio (x64).
#>

[CmdletBinding()]
param (
    [ValidateSet("Release", "Debug")]
    [string]$BuildConfig = "Release",

    [string]$SdkPath = $env:AION_SDK_DIR,

    [switch]$Clean,

    [switch]$Help
)

if ($Help) {
    Write-Host "Usage: .\build.ps1 [-BuildConfig <Release|Debug>] [-SdkPath <path>] [-Clean]"
    exit 0
}

$ErrorActionPreference = "Stop"

$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
Set-Location $ScriptDir

$BuildDir = Join-Path $ScriptDir "build"

Write-Host "===================================================" -ForegroundColor Cyan
Write-Host " Block Jumper Windows Build Workflow (PowerShell)" -ForegroundColor Cyan
Write-Host "===================================================" -ForegroundColor Cyan
Write-Host " Configuration: $BuildConfig"
Write-Host " Project Root:  $ScriptDir"
if ($SdkPath) {
    Write-Host " Aion SDK:      $SdkPath"
} else {
    Write-Host " Aion SDK:      (Not specified; searching standard paths / CMake cache)"
}
Write-Host "===================================================" -ForegroundColor Cyan

if ($Clean -and (Test-Path $BuildDir)) {
    Write-Host "[BUILD] Cleaning existing build directory: $BuildDir..." -ForegroundColor Yellow
    Remove-Item -Path $BuildDir -Recurse -Force
}

if (-not (Test-Path $BuildDir)) {
    New-Item -ItemType Directory -Path $BuildDir -Force | Out-Null
}

$CMakeArgs = @("-S", $ScriptDir, "-B", $BuildDir)
if ($PSVersionTable.Platform -eq "Win32NT" -or $env:OS -like "*Windows*") {
    $CMakeArgs += @("-A", "x64")
}
if ($SdkPath) {
    $CMakeArgs += "-DCMAKE_PREFIX_PATH=$SdkPath"
}
$CMakeArgs += "-DCMAKE_BUILD_TYPE=$BuildConfig"
if ($BuildConfig -eq "Release" -or $BuildConfig -eq "RelWithDebInfo" -or $BuildConfig -eq "MinSizeRel") {
    $CMakeArgs += "-DAION_ENABLE_CONSOLE_LOG=OFF"
} else {
    $CMakeArgs += "-DAION_ENABLE_CONSOLE_LOG=ON"
}

Write-Host "[BUILD] Configuring CMake..." -ForegroundColor Green
& cmake @CMakeArgs
if ($LASTEXITCODE -ne 0) {
    Write-Error "CMake configuration failed with exit code $LASTEXITCODE. Ensure Visual Studio 2019/2022 with C++20 and the Aion SDK are installed."
    exit $LASTEXITCODE
}

Write-Host "[BUILD] Compiling Block Jumper ($BuildConfig)..." -ForegroundColor Green
& cmake --build $BuildDir --config $BuildConfig
if ($LASTEXITCODE -ne 0) {
    Write-Error "Compilation failed with exit code $LASTEXITCODE."
    exit $LASTEXITCODE
}

$ExePath = Join-Path $BuildDir "$BuildConfig\BlockJumper.exe"
if (-not (Test-Path $ExePath)) {
    $ExePath = Join-Path $BuildDir "BlockJumper.exe"
}
if (-not (Test-Path $ExePath)) {
    $ExePath = Join-Path $BuildDir "BlockJumper"
}

Write-Host "===================================================" -ForegroundColor Cyan
Write-Host "[BUILD] Build completed successfully!" -ForegroundColor Green
if (Test-Path $ExePath) {
    Write-Host " Executable: $ExePath" -ForegroundColor Green
}
Write-Host "===================================================" -ForegroundColor Cyan
exit 0
