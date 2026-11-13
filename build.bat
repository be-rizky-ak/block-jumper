@echo off
setlocal EnableDelayedExpansion

:: ==============================================================================
:: Block Jumper - Windows Batch Build Entrypoint
:: Resolves project root directory regardless of caller working directory
:: ==============================================================================

set "PROJECT_ROOT=%~dp0"
cd /d "%PROJECT_ROOT%"

set "BUILD_CONFIG=Release"
set "SDK_PATH=%AION_SDK_DIR%"
set "CLEAN_BUILD=0"

:parse_args
if "%~1"=="" goto after_args
if /i "%~1"=="Release" (
    set "BUILD_CONFIG=Release"
    shift
    goto parse_args
)
if /i "%~1"=="Debug" (
    set "BUILD_CONFIG=Debug"
    shift
    goto parse_args
)
if /i "%~1"=="--clean" (
    set "CLEAN_BUILD=1"
    shift
    goto parse_args
)
if /i "%~1"=="--sdk-path" (
    set "SDK_PATH=%~2"
    shift
    shift
    goto parse_args
)
if /i "%~1"=="-SdkPath" (
    set "SDK_PATH=%~2"
    shift
    shift
    goto parse_args
)
if /i "%~1"=="--sdk-dir" (
    set "SDK_PATH=%~2"
    shift
    shift
    goto parse_args
)
if /i "%~1"=="-SdkDir" (
    set "SDK_PATH=%~2"
    shift
    shift
    goto parse_args
)
if /i "%~1"=="--help" goto show_help
if /i "%~1"=="-h" goto show_help

if exist "%~1" (
    set "SDK_PATH=%~1"
    shift
    goto parse_args
)

echo [ERROR] Unknown option or argument: %~1
goto show_help

:show_help
echo Usage: build.bat [CONFIG] [OPTIONS]
echo.
echo Configurations:
echo   Release                       Build in Release configuration (default)
echo   Debug                         Build in Debug configuration
echo.
echo Options:
echo   --sdk-path, -SdkPath ^<path^>   Path to unified Aion SDK root (or set AION_SDK_DIR)
echo   --clean                        Clean build directory before configuring
echo   --help, -h                     Display this help message
exit /b 0

:after_args
:: Discover SDK root if not explicitly provided
if not defined SDK_PATH (
    if exist "%PROJECT_ROOT%..\..\aion\dist\sdk\AionSDK" (
        set "SDK_PATH=%PROJECT_ROOT%..\..\aion\dist\sdk\AionSDK"
    ) else if exist "%PROJECT_ROOT%..\..\..\aion\dist\sdk\AionSDK" (
        set "SDK_PATH=%PROJECT_ROOT%..\..\..\aion\dist\sdk\AionSDK"
    ) else if exist "%PROJECT_ROOT%..\..\dist\sdk\AionSDK" (
        set "SDK_PATH=%PROJECT_ROOT%..\..\dist\sdk\AionSDK"
    ) else if exist "C:\AionSDK" (
        set "SDK_PATH=C:\AionSDK"
    ) else if defined USERPROFILE if exist "%USERPROFILE%\AionSDK" (
        set "SDK_PATH=%USERPROFILE%\AionSDK"
    ) else if exist "%PROJECT_ROOT%..\..\aion\dist\AionSDK" (
        set "SDK_PATH=%PROJECT_ROOT%..\..\aion\dist\AionSDK"
    ) else if exist "%PROJECT_ROOT%..\..\aion\dist\sdk\Release\AionSDK" (
        set "SDK_PATH=%PROJECT_ROOT%..\..\aion\dist\sdk\Release\AionSDK"
    )
)

:: Validate SDK path and ensure platform distribution is available
if defined SDK_PATH (
    if not exist "%SDK_PATH%" (
        echo [ERROR] Provided Aion SDK path does not exist: %SDK_PATH%
        exit /b 1
    )

    set "IS_UNIFIED_ROOT=0"
    if exist "%SDK_PATH%\AionEngineConfig.cmake" set "IS_UNIFIED_ROOT=1"
    if exist "%SDK_PATH%\AionSDKConfig.cmake" set "IS_UNIFIED_ROOT=1"
    if exist "%SDK_PATH%\Windows" if exist "%SDK_PATH%\Web" set "IS_UNIFIED_ROOT=1"

    if "!IS_UNIFIED_ROOT!"=="1" (
        if not exist "%SDK_PATH%\Windows" (
            echo ===================================================
            echo [ERROR] Missing Windows Platform Distribution in Unified SDK!
            echo ===================================================
            echo The unified Aion SDK located at '%SDK_PATH%' does not contain the 'Windows' platform distribution.
            echo The current build targets native Windows.
            echo.
            echo Actionable Guidance:
            echo   1. Run package.bat in the Aion repository to generate the Windows distribution.
            echo   2. Rebuild Block Jumper using the unified SDK root:
            echo        .\build.bat -SdkPath %SDK_PATH%
            echo ===================================================
            exit /b 1
        )
    ) else (
        if exist "%SDK_PATH%\lib\cmake\AionEngine\Web.cmake" (
            echo ===================================================
            echo [ERROR] Incompatible Aion SDK Distribution Detected!
            echo ===================================================
            echo The SDK located at '%SDK_PATH%' is a WebAssembly distribution.
            echo The current build targets native Windows.
            echo.
            echo Actionable Guidance:
            echo   1. Specify the unified Aion SDK root:
            echo        .\build.bat -SdkPath path\to\AionSDK
            echo ===================================================
            exit /b 1
        )
        if exist "%SDK_PATH%\lib\cmake\AionEngine\AionEngineConfigVersion.cmake" (
            findstr /c:"_AION_SDK_IS_WEB 1" "%SDK_PATH%\lib\cmake\AionEngine\AionEngineConfigVersion.cmake" >nul 2>&1
            if !errorlevel! equ 0 (
                echo ===================================================
                echo [ERROR] Incompatible Aion SDK Distribution Detected!
                echo ===================================================
                echo The SDK located at '%SDK_PATH%' is a WebAssembly distribution.
                echo The current build targets native Windows.
                echo.
                echo Actionable Guidance:
                echo   1. Specify the unified Aion SDK root:
                echo        .\build.bat -SdkPath path\to\AionSDK
                echo ===================================================
                exit /b 1
            )
        )
        echo [DEPRECATION NOTICE] You passed a platform-specific SDK path: '%SDK_PATH%'.
        echo Please migrate to specifying the unified SDK root (e.g. -SdkPath path\to\AionSDK).
        echo Proceeding using backward-compatibility path...
    )
)
echo ===================================================
echo  Block Jumper Windows Build Workflow
echo ===================================================
echo  Configuration: %BUILD_CONFIG%
echo  Project Root:  %PROJECT_ROOT%
if defined SDK_PATH (
    echo  Aion SDK:      %SDK_PATH%
) else (
    echo  Aion SDK:      (Not specified; searching standard paths / CMake cache)
)
echo ===================================================

set "BUILD_DIR=build"

if "%CLEAN_BUILD%"=="1" (
    if exist "%BUILD_DIR%" (
        echo [BUILD] Cleaning build directory: %BUILD_DIR%...
        rmdir /s /q "%BUILD_DIR%"
        if !errorlevel! neq 0 (
            echo [ERROR] Failed to clean build directory.
            exit /b !errorlevel!
        )
    )
)

if not exist "%BUILD_DIR%" (
    mkdir "%BUILD_DIR%"
)

:: Construct CMake arguments targeting x64 on Windows
set "CMAKE_CONFIG_ARGS=-S . -B "%BUILD_DIR%" -A x64 -DCMAKE_BUILD_TYPE=%BUILD_CONFIG%"
if defined SDK_PATH (
    set "CMAKE_CONFIG_ARGS=!CMAKE_CONFIG_ARGS! -DCMAKE_PREFIX_PATH="%SDK_PATH%""
)
if /i "%BUILD_CONFIG%"=="Release" (
    set "CMAKE_CONFIG_ARGS=!CMAKE_CONFIG_ARGS! -DAION_ENABLE_CONSOLE_LOG=OFF"
) else (
    set "CMAKE_CONFIG_ARGS=!CMAKE_CONFIG_ARGS! -DAION_ENABLE_CONSOLE_LOG=ON"
)

echo [BUILD] Configuring CMake...
cmake !CMAKE_CONFIG_ARGS!
if !errorlevel! neq 0 (
    echo [ERROR] CMake configuration failed with exit code !errorlevel!.
    echo [ERROR] Ensure Visual Studio 2019/2022 with C++20 and the Aion SDK are installed.
    exit /b !errorlevel!
)

echo [BUILD] Compiling Block Jumper (%BUILD_CONFIG%)...
cmake --build "%BUILD_DIR%" --config %BUILD_CONFIG%
if !errorlevel! neq 0 (
    echo [ERROR] Compilation failed with exit code !errorlevel!.
    exit /b !errorlevel!
)

echo ===================================================
echo [BUILD] Build completed successfully!
if exist "%BUILD_DIR%\%BUILD_CONFIG%\BlockJumper.exe" (
    echo  Executable: %PROJECT_ROOT%%BUILD_DIR%\%BUILD_CONFIG%\BlockJumper.exe
) else if exist "%BUILD_DIR%\BlockJumper.exe" (
    echo  Executable: %PROJECT_ROOT%%BUILD_DIR%\BlockJumper.exe
)
echo ===================================================
exit /b 0
