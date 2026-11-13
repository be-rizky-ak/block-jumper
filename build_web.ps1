# ==============================================================================
# Block Jumper - WebAssembly / WebGL 2 Build & Export Workflow (PowerShell)
# ==============================================================================

[CmdletBinding(PositionalBinding = $false)]
param(
    [Alias("c")]
    [ValidateSet("Release", "Debug", IgnoreCase = $true)]
    [string]$Config = "Release",

    [Alias("t")]
    [string]$Target = "BlockJumper",

    [Alias("o")]
    [string]$ExportDir = "",

    [Alias("b")]
    [string]$BuildDir = "build_web",

    [Alias("s", "SdkDir")]
    [string]$SdkPath = $env:AION_SDK_DIR,

    [switch]$Clean,

    [Alias("h")]
    [switch]$Help,

    [Parameter(ValueFromRemainingArguments = $true)]
    [string[]]$ExtraCmakeArgs
)

$ErrorActionPreference = "Stop"

function Show-Help {
    @"
Usage: .\build_web.ps1 [options] [-- <extra-cmake-args>]

Build and export Block Jumper for WebAssembly and WebGL 2.

Options:
  -c, -Config <Config>       Build configuration: Release (default) or Debug
  -t, -Target <Target>       Build target executable (default: BlockJumper)
  -o, -ExportDir <Dir>      Distribution export directory (default: <build-dir>\export)
  -b, -BuildDir <Dir>       CMake build directory (default: build_web)
  -s, -SdkPath <Dir>, -SdkDir <Dir>  Path to Aion Web SDK installation (e.g. dist\sdk\Web\AionSDK)
  -Clean                    Clean build directory before configuring
  -h, -Help                 Show this help message and exit

Examples:
  .\build_web.ps1                            # Build Block Jumper in Release
  .\build_web.ps1 -Config Debug              # Build Debug configuration with assertions
  .\build_web.ps1 -Clean                     # Perform clean configure and build
  .\build_web.ps1 -ExportDir dist\web        # Export distribution to custom directory
"@
}

if ($Help) {
    Show-Help
    exit 0
}

$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Definition
$GameRoot = $ScriptDir

if ([string]::IsNullOrWhiteSpace($BuildDir)) {
    $BuildDir = "build_web"
}
if (-not [System.IO.Path]::IsPathRooted($BuildDir)) {
    $BuildDir = Join-Path $GameRoot $BuildDir
}

if ([string]::IsNullOrWhiteSpace($ExportDir)) {
    $ExportDir = Join-Path $BuildDir "export"
} elseif (-not [System.IO.Path]::IsPathRooted($ExportDir)) {
    $ExportDir = Join-Path $GameRoot $ExportDir
}

Write-Host "========================================================================" -ForegroundColor Cyan
Write-Host "          Block Jumper - WebAssembly & WebGL 2 Export Workflow          " -ForegroundColor Cyan
Write-Host "========================================================================" -ForegroundColor Cyan

# Step 1: Detect and activate Emscripten SDK environment
$emcmakeCmd = Get-Command emcmake -ErrorAction SilentlyContinue
$emccCmd    = Get-Command emcc -ErrorAction SilentlyContinue

if (-not $emcmakeCmd -or -not $emccCmd) {
    $emsdkCandidates = @(
        $env:EMSDK,
        (Join-Path $env:USERPROFILE "emsdk"),
        "C:\emsdk",
        (Join-Path $env:LOCALAPPDATA "emsdk")
    )

    $activated = $false
    foreach ($cand in $emsdkCandidates) {
        if ($cand -and (Test-Path $cand)) {
            $envScript = Join-Path $cand "emsdk_env.ps1"
            if (Test-Path $envScript) {
                Write-Host "--> Sourcing Emscripten environment from: $envScript" -ForegroundColor Yellow
                & $envScript
                $activated = $true
                break
            }
        }
    }

    if (-not $activated) {
        Write-Error "Emscripten SDK was not detected in PATH, and emsdk_env.ps1 could not be found."
        Write-Host "Prerequisites Setup Instructions:"
        Write-Host "  1. Install Emscripten SDK (supported versions: 3.1.50 - 3.1.64+):"
        Write-Host "       git clone https://github.com/emscripten-core/emsdk.git `$env:USERPROFILE\emsdk"
        Write-Host "       cd `$env:USERPROFILE\emsdk"
        Write-Host "       .\emsdk.bat install 3.1.64"
        Write-Host "       .\emsdk.bat activate 3.1.64"
        Write-Host "  2. Activate SDK environment:"
        Write-Host "       & `"`$env:USERPROFILE\emsdk\emsdk_env.ps1`""
        exit 1
    }
}

# Step 2: Validate essential build tools
$requiredTools = @("emcmake", "emcc", "em++", "cmake", "ninja")
$missingTools = @()

foreach ($tool in $requiredTools) {
    if (-not (Get-Command $tool -ErrorAction SilentlyContinue)) {
        $missingTools += $tool
    }
}

if ($missingTools.Count -gt 0) {
    Write-Error "The following required build tools are missing from PATH: $($missingTools -join ', ')"
    exit 1
}

# Step 3: Discover Aion SDK Root Path if not explicitly provided
if ([string]::IsNullOrWhiteSpace($SdkPath)) {
    $sdkCandidates = @(
        $env:AION_SDK_DIR,
        (Join-Path $GameRoot "..\..\aion\dist\sdk\AionSDK"),
        (Join-Path $GameRoot "..\..\..\aion\dist\sdk\AionSDK"),
        (Join-Path $GameRoot "..\..\dist\sdk\AionSDK"),
        "C:\AionSDK",
        (Join-Path $env:USERPROFILE "AionSDK"),
        # Legacy paths for fallback compatibility
        (Join-Path $GameRoot "..\..\aion\dist\AionSDK"),
        (Join-Path $GameRoot "..\..\..\aion\dist\AionSDK"),
        (Join-Path $GameRoot "..\..\dist\AionSDK"),
        (Join-Path $GameRoot "..\..\aion\dist\sdk\Web\AionSDK"),
        (Join-Path $GameRoot "..\..\..\aion\dist\sdk\Web\AionSDK"),
        (Join-Path $GameRoot "..\..\dist\sdk\Web\AionSDK"),
        "C:\AionSDK\Web",
        (Join-Path $env:USERPROFILE "AionSDK\Web")
    )

    foreach ($cand in $sdkCandidates) {
        if ($cand -and (Test-Path $cand)) {
            # Unified SDK root containing Web distribution
            $webDir = Join-Path $cand "Web"
            $rootConfig = Join-Path $cand "AionEngineConfig.cmake"
            $rootSdkConfig = Join-Path $cand "AionSDKConfig.cmake"
            if ((Test-Path $webDir) -and ((Test-Path $rootConfig) -or (Test-Path $rootSdkConfig) -or (Test-Path (Join-Path $webDir "lib\cmake\AionEngine")))) {
                $SdkPath = (Resolve-Path $cand).Path
                break
            }
            # Legacy Web SDK candidate
            elseif (Test-Path (Join-Path $cand "lib\cmake\AionEngine\AionEngineConfig.cmake")) {
                $versionFile = Join-Path $cand "lib\cmake\AionEngine\AionEngineConfigVersion.cmake"
                $isWeb = $false
                if (Test-Path $versionFile) {
                    $verContent = Get-Content $versionFile -Raw -ErrorAction SilentlyContinue
                    if ($verContent -match "_AION_SDK_IS_WEB\s+1") {
                        $isWeb = $true
                    }
                }
                if (-not $isWeb -and (Test-Path (Join-Path $cand "lib\cmake\AionEngine\Web.cmake"))) {
                    $isWeb = $true
                }
                if ($isWeb) {
                    $SdkPath = (Resolve-Path $cand).Path
                    break
                }
            }
        }
    }
}

if ($SdkPath) {
    if (-not (Test-Path $SdkPath)) {
        Write-Error "Provided Aion SDK path does not exist: $SdkPath"
        exit 1
    }

    # Detect if provided path is a unified SDK root
    $isUnifiedRoot = $false
    if ((Test-Path (Join-Path $SdkPath "AionEngineConfig.cmake")) -or
        (Test-Path (Join-Path $SdkPath "AionSDKConfig.cmake")) -or
        ((Test-Path (Join-Path $SdkPath "Web")) -and ((Test-Path (Join-Path $SdkPath "Windows")) -or (Test-Path (Join-Path $SdkPath "Linux"))))) {
        $isUnifiedRoot = $true
    }

    if ($isUnifiedRoot) {
        # Unified SDK root: verify that the Web distribution exists within it
        $webDir = Join-Path $SdkPath "Web"
        if (-not (Test-Path $webDir)) {
            Write-Error @"
========================================================================
[ERROR] Missing Web Platform Distribution in Unified SDK!
========================================================================
The unified Aion SDK located at '$SdkPath' does not contain the 'Web' platform distribution.
The current build targets WebAssembly (Emscripten / WebGL 2).

Actionable Guidance:
  1. Build and install the Aion Web SDK distribution:
       cd path\to\aion
       .\build_web.ps1 -Config Release
  2. Specify the unified SDK root when building Block Jumper:
       .\build_web.ps1 -SdkPath $SdkPath
========================================================================
"@
            exit 1
        }
    } else {
        # Legacy platform-specific SDK path provided
        # Check if user accidentally provided a legacy native desktop SDK path
        $versionFile = Join-Path $SdkPath "lib\cmake\AionEngine\AionEngineConfigVersion.cmake"
        if (Test-Path $versionFile) {
            $verContent = Get-Content $versionFile -Raw -ErrorAction SilentlyContinue
            if ($verContent -match "_AION_SDK_IS_WEB\s+0") {
                Write-Error @"
========================================================================
[ERROR] Incompatible Aion SDK Distribution Detected!
========================================================================
The SDK located at '$SdkPath' is a legacy native desktop SDK distribution.
The current build targets WebAssembly (Emscripten / WebGL 2).

Actionable Guidance:
  1. Build the unified Aion SDK with Web distribution:
       cd path\to\aion
       .\build_web.ps1 -Config Release
  2. Specify the unified SDK root when building Block Jumper:
       .\build_web.ps1 -SdkPath path\to\aion\dist\sdk\AionSDK
========================================================================
"@
                exit 1
            }
        }

        # If it is a legacy Web SDK path, emit a clear migration notice
        Write-Warning "[DEPRECATION NOTICE] You passed a platform-specific Web SDK path: '$SdkPath'."
        Write-Warning "Please migrate to specifying the unified SDK root directory (e.g. -SdkPath path\to\aion\dist\sdk\AionSDK)."
        Write-Warning "Proceeding using backward-compatibility path..."
    }
}

if ($SdkPath) {
    Write-Host "Using Aion SDK     : $SdkPath" -ForegroundColor Green
} else {
    Write-Host "Using Aion SDK     : (Searching default CMake prefixes / system paths)"
}

$emccVer  = (& emcc --version | Select-Object -First 1)
$cmakeVer = (& cmake --version | Select-Object -First 1)
$ninjaVer = (& ninja --version | Select-Object -First 1)

Write-Host "Detected Compiler  : $emccVer"
Write-Host "Detected CMake     : $cmakeVer"
Write-Host "Detected Ninja     : $ninjaVer"
Write-Host "Build Target       : $Target"
Write-Host "Configuration      : $Config"
Write-Host "Build Directory    : $BuildDir"
Write-Host "Export Directory   : $ExportDir"
Write-Host "========================================================================" -ForegroundColor Cyan

# Step 4: Clean build directory if requested or generator mismatch
if ($Clean -and (Test-Path $BuildDir)) {
    Write-Host "--> Cleaning existing build directory: $BuildDir" -ForegroundColor Yellow
    Remove-Item -Path $BuildDir -Recurse -Force
}

$cmakeGenArgs = @()
$hasCustomG = $false
if ($ExtraCmakeArgs) {
    foreach ($arg in $ExtraCmakeArgs) {
        if ($arg -eq "-G" -or $arg -like "-G*") {
            $hasCustomG = $true
            break
        }
    }
}

if (-not $hasCustomG) {
    $cmakeGenArgs = @("-G", "Ninja")
    $cacheFile = Join-Path $BuildDir "CMakeCache.txt"
    if (Test-Path $cacheFile) {
        $cacheContent = Get-Content $cacheFile -ErrorAction SilentlyContinue
        $genLine = $cacheContent | Where-Object { $_ -match "^CMAKE_GENERATOR:INTERNAL=(.+)$" } | Select-Object -First 1
        if ($genLine) {
            $cachedGen = ($genLine -split "=", 2)[1].Trim()
            if ($cachedGen -and $cachedGen -ne "Ninja") {
                Write-Host "--> Generator mismatch in existing cache ($cachedGen vs Ninja). Recreating $BuildDir..." -ForegroundColor Yellow
                Remove-Item -Path $BuildDir -Recurse -Force
            }
        }
    }
}

# Step 5: Configure CMake with Emscripten toolchain
$cmakeArgs = @(
    "-S", $GameRoot,
    "-B", $BuildDir
)
$cmakeArgs += $cmakeGenArgs
$cmakeArgs += "-DCMAKE_BUILD_TYPE=$Config"
$cmakeArgs += "-DCMAKE_FIND_ROOT_PATH_MODE_PACKAGE=BOTH"

if ($SdkPath) {
    $cmakeArgs += "-DCMAKE_PREFIX_PATH=$SdkPath"
}

if ($Config -eq "Release" -or $Config -eq "RelWithDebInfo" -or $Config -eq "MinSizeRel") {
    $cmakeArgs += "-DAION_ENABLE_CONSOLE_LOG=OFF"
} else {
    $cmakeArgs += "-DAION_ENABLE_CONSOLE_LOG=ON"
}

if ($ExtraCmakeArgs) {
    $cmakeArgs += $ExtraCmakeArgs
}

Write-Host "--> Configuring CMake with Emscripten Toolchain..." -ForegroundColor Green
& emcmake cmake @cmakeArgs
if ($LASTEXITCODE -ne 0) {
    Write-Error @"
========================================================================
CMake configuration failed with exit code $LASTEXITCODE.

If the error above indicates that package 'AionEngine' was not found or is incompatible:
  - Ensure the Aion Web SDK distribution has been built and staged:
      cd path\to\aion
      .\build_web.ps1 -Config Release
  - Provide the Web SDK path explicitly:
      .\build_web.ps1 -SdkPath path\to\aion\dist\sdk\AionSDK
========================================================================
"@
    exit $LASTEXITCODE
}

# Step 6: Build target
Write-Host "--> Compiling target '$Target'..." -ForegroundColor Green
& cmake --build $BuildDir --target $Target --parallel
if ($LASTEXITCODE -ne 0) {
    Write-Error "Compilation failed with exit code $LASTEXITCODE."
    exit $LASTEXITCODE
}

# Step 7: Validate generated artifacts
$htmlArtifact = Join-Path $BuildDir "$Target.html"
$jsArtifact   = Join-Path $BuildDir "$Target.js"
$wasmArtifact = Join-Path $BuildDir "$Target.wasm"
$dataArtifact = Join-Path $BuildDir "$Target.data"

$expectedArtifacts = @($htmlArtifact, $jsArtifact, $wasmArtifact, $dataArtifact)
foreach ($art in $expectedArtifacts) {
    if (-not (Test-Path -Path $art -PathType Leaf)) {
        Write-Error "Expected build artifact missing: $art"
        exit 1
    }
    $item = Get-Item $art
    if ($item.Length -le 0) {
        Write-Error "Generated build artifact is empty (0 bytes): $art"
        exit 1
    }
}

# Step 8: Stage clean distribution export directory
Write-Host "--> Staging clean web export to: $ExportDir" -ForegroundColor Green
if (Test-Path $ExportDir) {
    Remove-Item -Path $ExportDir -Recurse -Force
}
New-Item -ItemType Directory -Path $ExportDir -Force | Out-Null

Copy-Item -Path $htmlArtifact -Destination (Join-Path $ExportDir "index.html")
Copy-Item -Path $htmlArtifact -Destination (Join-Path $ExportDir "$Target.html")
Copy-Item -Path $jsArtifact   -Destination (Join-Path $ExportDir "$Target.js")
Copy-Item -Path $wasmArtifact -Destination (Join-Path $ExportDir "$Target.wasm")
Copy-Item -Path $dataArtifact -Destination (Join-Path $ExportDir "$Target.data")

# Step 9: Generate provenance export manifest
function Get-Sha256Hex([string]$filePath) {
    $hash = Get-FileHash -Path $filePath -Algorithm SHA256
    return $hash.Hash.ToLower()
}

$indexHash      = Get-Sha256Hex (Join-Path $ExportDir "index.html")
$targetHtmlHash = Get-Sha256Hex (Join-Path $ExportDir "$Target.html")
$jsHash         = Get-Sha256Hex (Join-Path $ExportDir "$Target.js")
$wasmHash       = Get-Sha256Hex (Join-Path $ExportDir "$Target.wasm")
$dataHash       = Get-Sha256Hex (Join-Path $ExportDir "$Target.data")
$buildTimestamp = (Get-Date).ToUniversalTime().ToString("yyyy-MM-ddTHH:mm:ssZ")

$gitCommit = "unknown"
try {
    $gitCommit = (& git -C $GameRoot rev-parse HEAD 2>$null)
} catch {
}

$manifestFile = Join-Path $ExportDir "export_manifest.json"
$manifestContent = @"
{
  "project": "BlockJumper",
  "version": "1.0.0",
  "target": "$Target",
  "config": "$Config",
  "timestamp": "$buildTimestamp",
  "git_commit": "$gitCommit",
  "artifacts": {
    "index.html": "$indexHash",
    "$Target.html": "$targetHtmlHash",
    "$Target.js": "$jsHash",
    "$Target.wasm": "$wasmHash",
    "$Target.data": "$dataHash"
  }
}
"@
Set-Content -Path $manifestFile -Value $manifestContent -Encoding Ascii

# Step 10: Validate staged package
Write-Host "--> Validating export package..." -ForegroundColor Green
$requiredExports = @(
    (Join-Path $ExportDir "index.html"),
    (Join-Path $ExportDir "$Target.html"),
    (Join-Path $ExportDir "$Target.js"),
    (Join-Path $ExportDir "$Target.wasm"),
    (Join-Path $ExportDir "$Target.data"),
    (Join-Path $ExportDir "export_manifest.json")
)

foreach ($file in $requiredExports) {
    if (-not (Test-Path -Path $file -PathType Leaf)) {
        Write-Error "Export validation failed for: $file"
        exit 1
    }
    $fItem = Get-Item $file
    if ($fItem.Length -le 0) {
        Write-Error "Exported file is empty (0 bytes): $file"
        exit 1
    }
}

Write-Host ""
Write-Host "========================================================================" -ForegroundColor Cyan
Write-Host "                   Export Completed Successfully!                      " -ForegroundColor Green
Write-Host "========================================================================" -ForegroundColor Cyan
Write-Host "Distribution directory ready for static hosting:"
Write-Host "  $ExportDir"
Write-Host ""
Write-Host "Exported Files:"
Get-ChildItem -Path $ExportDir | Format-Table Name, Length, LastWriteTime
Write-Host ""
Write-Host "To test locally using Python's static HTTP server:"
Write-Host "  python -m http.server 8080 --directory `"$ExportDir`""
Write-Host ""
Write-Host "Then open in any WebGL 2 compatible browser:"
Write-Host "  http://localhost:8080/"
Write-Host "========================================================================" -ForegroundColor Cyan
exit 0
