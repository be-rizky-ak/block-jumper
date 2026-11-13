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

    [Alias("SdkDir")]
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

# Discover SDK root if not explicitly provided
if ([string]::IsNullOrWhiteSpace($SdkPath)) {
    $sdkCandidates = @(
        $env:AION_SDK_DIR,
        (Join-Path $ScriptDir "..\..\aion\dist\sdk\AionSDK"),
        (Join-Path $ScriptDir "..\..\..\aion\dist\sdk\AionSDK"),
        (Join-Path $ScriptDir "..\..\dist\sdk\AionSDK"),
        "C:\AionSDK",
        (Join-Path $env:USERPROFILE "AionSDK"),
        # Legacy paths for fallback compatibility
        (Join-Path $ScriptDir "..\..\aion\dist\AionSDK"),
        (Join-Path $ScriptDir "..\..\..\aion\dist\AionSDK"),
        (Join-Path $ScriptDir "..\..\dist\AionSDK"),
        (Join-Path $ScriptDir "..\..\aion\dist\sdk\Release\AionSDK")
    )

    foreach ($cand in $sdkCandidates) {
        if ($cand -and (Test-Path $cand)) {
            $winDir = Join-Path $cand "Windows"
            $rootConfig = Join-Path $cand "AionEngineConfig.cmake"
            $rootSdkConfig = Join-Path $cand "AionSDKConfig.cmake"
            if ((Test-Path $winDir) -or (Test-Path $rootConfig) -or (Test-Path $rootSdkConfig)) {
                $SdkPath = (Resolve-Path $cand).Path
                break
            } elseif (Test-Path (Join-Path $cand "lib\cmake\AionEngine\AionEngineConfig.cmake")) {
                $SdkPath = (Resolve-Path $cand).Path
                break
            }
        }
    }
}

# Validate SDK path and ensure platform distribution is available
if ($SdkPath) {
    if (-not (Test-Path $SdkPath)) {
        Write-Error "Provided Aion SDK path does not exist: $SdkPath"
        exit 1
    }

    $isUnifiedRoot = $false
    if ((Test-Path (Join-Path $SdkPath "AionEngineConfig.cmake")) -or
        (Test-Path (Join-Path $SdkPath "AionSDKConfig.cmake")) -or
        ((Test-Path (Join-Path $SdkPath "Windows")) -and (Test-Path (Join-Path $SdkPath "Web")))) {
        $isUnifiedRoot = $true
    }

    if ($isUnifiedRoot) {
        $winDir = Join-Path $SdkPath "Windows"
        if (-not (Test-Path $winDir)) {
            Write-Error @"
===================================================
[ERROR] Missing Windows Platform Distribution in Unified SDK!
===================================================
The unified Aion SDK located at '$SdkPath' does not contain the 'Windows' platform distribution.
The current build targets native Windows.

Actionable Guidance:
  1. Run package.bat in the Aion repository to generate the Windows distribution.
  2. Rebuild Block Jumper using the unified SDK root:
       .\build.ps1 -SdkPath $SdkPath
===================================================
"@
            exit 1
        }
    } else {
        if (Test-Path (Join-Path $SdkPath "lib\cmake\AionEngine\Web.cmake")) {
            Write-Error @"
===================================================
[ERROR] Incompatible Aion SDK Distribution Detected!
===================================================
The SDK located at '$SdkPath' is a WebAssembly distribution.
The current build targets native Windows.

Actionable Guidance:
  1. Specify the unified Aion SDK root:
       .\build.ps1 -SdkPath path\to\AionSDK
===================================================
"@
            exit 1
        }
        $versionFile = Join-Path $SdkPath "lib\cmake\AionEngine\AionEngineConfigVersion.cmake"
        if (Test-Path $versionFile) {
            $verContent = Get-Content $versionFile -Raw -ErrorAction SilentlyContinue
            if ($verContent -match "_AION_SDK_IS_WEB\s+1") {
                Write-Error @"
===================================================
[ERROR] Incompatible Aion SDK Distribution Detected!
===================================================
The SDK located at '$SdkPath' is a WebAssembly distribution.
The current build targets native Windows.

Actionable Guidance:
  1. Specify the unified Aion SDK root:
       .\build.ps1 -SdkPath path\to\AionSDK
===================================================
"@
                exit 1
            }
        }
        Write-Warning "[DEPRECATION NOTICE] You passed a platform-specific SDK path: '$SdkPath'."
        Write-Warning "Please migrate to specifying the unified SDK root (e.g. -SdkPath path\to\AionSDK)."
        Write-Warning "Proceeding using backward-compatibility path..."
    }
}

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
