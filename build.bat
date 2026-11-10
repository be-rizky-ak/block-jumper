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
echo   Release         Build in Release configuration (default)
echo   Debug           Build in Debug configuration
echo.
echo Options:
echo   --sdk-path ^<path^> Path to Aion SDK installation (or set AION_SDK_DIR)
echo   --clean          Clean build directory before configuring
echo   --help, -h       Display this help message
exit /b 0

:after_args
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
