#!/usr/bin/env bash
set -euo pipefail

# ==============================================================================
# Block Jumper - WebAssembly / WebGL 2 Build & Export Workflow (Bash)
# ==============================================================================

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
GAME_ROOT="${SCRIPT_DIR}"

# Default parameters
CONFIG="Release"
TARGET="BlockJumper"
BUILD_DIR="${GAME_ROOT}/build_web"
EXPORT_DIR=""
SDK_PATH="${AION_SDK_DIR:-}"
CLEAN_BUILD=0
EXTRA_CMAKE_ARGS=()

show_help() {
    cat << EOF
Usage: ./build_web.sh [options] [-- <extra-cmake-args>]

Build and export Block Jumper for WebAssembly and WebGL 2.

Options:
  -c, --config <Config>       Build configuration: Release (default) or Debug
  -t, --target <Target>       Build target executable (default: BlockJumper)
  -o, --export-dir <Dir>      Distribution export directory (default: <build-dir>/export)
  -b, --build-dir <Dir>       CMake build directory (default: build_web)
  -s, --sdk-path, -SdkPath, --sdk-dir <Dir>  Path to unified Aion SDK root (or set AION_SDK_DIR)
  --clean                     Clean build directory before configuring
  -h, --help                  Show this help message and exit

Examples:
  ./build_web.sh                             # Build Block Jumper in Release
  ./build_web.sh -SdkPath path/to/AionSDK    # Build using unified Aion SDK root
  ./build_web.sh --config Debug              # Build Debug configuration with assertions
  ./build_web.sh --clean                     # Clean configure and build
  ./build_web.sh --export-dir dist/web       # Export distribution to custom directory
EOF
}

# Parse command line options
while [[ $# -gt 0 ]]; do
    case "$1" in
        -c|--config)
            CONFIG="$2"
            shift 2
            ;;
        -t|--target)
            TARGET="$2"
            shift 2
            ;;
        -o|--export-dir)
            EXPORT_DIR="$2"
            shift 2
            ;;
        -b|--build-dir)
            BUILD_DIR="$2"
            shift 2
            ;;
        -s|--sdk-path|-SdkPath|--sdk-dir|-SdkDir)
            SDK_PATH="$2"
            shift 2
            ;;
        --clean)
            CLEAN_BUILD=1
            shift
            ;;
        -h|--help)
            show_help
            exit 0
            ;;
        --)
            shift
            EXTRA_CMAKE_ARGS=("$@")
            break
            ;;
        *)
            echo "Error: Unknown argument '$1'. Use --help for usage information." >&2
            exit 1
            ;;
    esac
done

if [[ -z "${EXPORT_DIR}" ]]; then
    EXPORT_DIR="${BUILD_DIR}/export"
fi

echo "========================================================================"
echo "          Block Jumper - WebAssembly & WebGL 2 Export Workflow          "
echo "========================================================================"

# Step 1: Detect and activate Emscripten SDK environment if needed
if ! command -v emcmake &> /dev/null || ! command -v emcc &> /dev/null; then
    EMSDK_ENV_CANDIDATES=(
        "${EMSDK:-}/emsdk_env.sh"
        "${HOME}/emsdk/emsdk_env.sh"
        "/opt/emsdk/emsdk_env.sh"
        "/tmp/emsdk/emsdk_env.sh"
    )

    FOUND_ENV=""
    for cand in "${EMSDK_ENV_CANDIDATES[@]}"; do
        if [[ -n "${cand}" && -f "${cand}" ]]; then
            FOUND_ENV="${cand}"
            break
        fi
    done

    if [[ -n "${FOUND_ENV}" ]]; then
        echo "--> Sourcing Emscripten environment from: ${FOUND_ENV}"
        export EMSDK_QUIET=1
        # shellcheck disable=SC1090
        source "${FOUND_ENV}"
    else
        echo "Error: Emscripten SDK was not detected in PATH, and emsdk_env.sh could not be found." >&2
        echo "" >&2
        echo "Prerequisite Setup Instructions:" >&2
        echo "  1. Install Emscripten SDK (supported versions: 3.1.50 - 3.1.64+):" >&2
        echo "       git clone https://github.com/emscripten-core/emsdk.git ~/emsdk" >&2
        echo "       cd ~/emsdk" >&2
        echo "       ./emsdk install 3.1.64" >&2
        echo "       ./emsdk activate 3.1.64" >&2
        echo "  2. Activate SDK environment:" >&2
        echo "       source ~/emsdk/emsdk_env.sh" >&2
        exit 1
    fi
fi

# Step 2: Validate essential build tools
MISSING_TOOLS=()
for tool in emcmake emcc em++ cmake ninja; do
    if ! command -v "${tool}" &> /dev/null; then
        MISSING_TOOLS+=("${tool}")
    fi
done

if [[ ${#MISSING_TOOLS[@]} -gt 0 ]]; then
    echo "Error: The following required tools are missing from PATH: ${MISSING_TOOLS[*]}" >&2
    exit 1
fi

# Step 3: Discover Aion SDK Root Path if not explicitly provided
if [[ -z "${SDK_PATH}" ]]; then
    SDK_CANDIDATES=(
        "${AION_SDK_DIR:-}"
        "${GAME_ROOT}/../../aion/dist/sdk/AionSDK"
        "${GAME_ROOT}/../../../aion/dist/sdk/AionSDK"
        "${GAME_ROOT}/../../dist/sdk/AionSDK"
        "/app/aion/dist/sdk/AionSDK"
        "${HOME}/AionSDK"
        "/opt/AionSDK"
        # Legacy paths for fallback compatibility
        "${GAME_ROOT}/../../aion/dist/AionSDK"
        "${GAME_ROOT}/../../../aion/dist/AionSDK"
        "${GAME_ROOT}/../../dist/AionSDK"
        "/app/aion/dist/AionSDK"
        "${GAME_ROOT}/../../aion/dist/sdk/Web/AionSDK"
        "/app/aion/dist/sdk/Web/AionSDK"
        "${HOME}/AionSDK/Web"
    )

    for cand in "${SDK_CANDIDATES[@]}"; do
        if [[ -n "${cand}" && -d "${cand}" ]]; then
            # Unified SDK root containing Web distribution
            if [[ -d "${cand}/Web" && ( -f "${cand}/AionEngineConfig.cmake" || -f "${cand}/AionSDKConfig.cmake" || -d "${cand}/Web/lib/cmake/AionEngine" ) ]]; then
                SDK_PATH="${cand}"
                break
            # Legacy Web SDK candidate
            elif [[ -f "${cand}/lib/cmake/AionEngine/AionEngineConfig.cmake" ]]; then
                if grep -q '_AION_SDK_IS_WEB 1' "${cand}/lib/cmake/AionEngine/AionEngineConfigVersion.cmake" 2>/dev/null || [[ -f "${cand}/lib/cmake/AionEngine/Web.cmake" ]]; then
                    SDK_PATH="${cand}"
                    break
                fi
            fi
        fi
    done
fi

if [[ -n "${SDK_PATH}" ]]; then
    if [[ ! -d "${SDK_PATH}" ]]; then
        echo "Error: Provided Aion SDK path does not exist: ${SDK_PATH}" >&2
        exit 1
    fi

    # Detect if provided path is a unified SDK root
    IS_UNIFIED_ROOT=0
    if [[ -f "${SDK_PATH}/AionEngineConfig.cmake" || -f "${SDK_PATH}/AionSDKConfig.cmake" || ( -d "${SDK_PATH}/Web" && -d "${SDK_PATH}/Windows" ) || ( -d "${SDK_PATH}/Web" && -d "${SDK_PATH}/Linux" ) ]]; then
        IS_UNIFIED_ROOT=1
    fi

    if [[ ${IS_UNIFIED_ROOT} -eq 1 ]]; then
        # Unified SDK root: verify that the Web distribution exists within it
        if [[ ! -d "${SDK_PATH}/Web" ]]; then
            echo "========================================================================" >&2
            echo "[ERROR] Missing Web Platform Distribution in Unified SDK!" >&2
            echo "========================================================================" >&2
            echo "The unified Aion SDK located at '${SDK_PATH}' does not contain the 'Web' platform distribution." >&2
            echo "The current build targets WebAssembly (Emscripten / WebGL 2)." >&2
            echo "" >&2
            echo "Actionable Guidance:" >&2
            echo "  1. Build and install the Aion Web SDK distribution:" >&2
            echo "       cd path/to/aion" >&2
            echo "       ./build_web.sh --config Release" >&2
            echo "  2. Specify the unified SDK root when building Block Jumper:" >&2
            echo "       ./build_web.sh -SdkPath ${SDK_PATH}" >&2
            echo "========================================================================" >&2
            exit 1
        fi
    else
        # Legacy platform-specific SDK path provided
        # Check if user accidentally provided a legacy native desktop SDK path
        VERSION_FILE="${SDK_PATH}/lib/cmake/AionEngine/AionEngineConfigVersion.cmake"
        if [[ -f "${VERSION_FILE}" ]]; then
            if grep -q '_AION_SDK_IS_WEB 0' "${VERSION_FILE}" 2>/dev/null; then
                echo "========================================================================" >&2
                echo "[ERROR] Incompatible Aion SDK Distribution Detected!" >&2
                echo "========================================================================" >&2
                echo "The SDK located at '${SDK_PATH}' is a legacy native desktop SDK distribution." >&2
                echo "The current build targets WebAssembly (Emscripten / WebGL 2)." >&2
                echo "" >&2
                echo "Actionable Guidance:" >&2
                echo "  1. Build the unified Aion SDK with Web distribution:" >&2
                echo "       cd path/to/aion" >&2
                echo "       ./build_web.sh --config Release" >&2
                echo "  2. Specify the unified SDK root when building Block Jumper:" >&2
                echo "       ./build_web.sh -SdkPath path/to/aion/dist/sdk/AionSDK" >&2
                echo "========================================================================" >&2
                exit 1
            fi
        fi

        # If it is a legacy Web SDK path, emit a clear migration notice
        echo "[DEPRECATION NOTICE] You passed a platform-specific Web SDK path: '${SDK_PATH}'."
        echo "Please migrate to specifying the unified SDK root directory (e.g. -SdkPath path/to/aion/dist/sdk/AionSDK)."
        echo "Proceeding using backward-compatibility path..."
    fi
fi

if [[ -n "${SDK_PATH}" ]]; then
    echo "Using Aion SDK     : ${SDK_PATH}"
else
    echo "Using Aion SDK     : (Searching default CMake prefixes / system paths)"
fi

EMCC_VERSION_LINE=$(emcc --version | head -n 1)
CMAKE_VERSION_LINE=$(cmake --version | head -n 1)
NINJA_VERSION_LINE=$(ninja --version | head -n 1)

echo "Detected Compiler  : ${EMCC_VERSION_LINE}"
echo "Detected CMake     : ${CMAKE_VERSION_LINE}"
echo "Detected Ninja     : ${NINJA_VERSION_LINE}"
echo "Build Target       : ${TARGET}"
echo "Configuration      : ${CONFIG}"
echo "Build Directory    : ${BUILD_DIR}"
echo "Export Directory   : ${EXPORT_DIR}"
echo "========================================================================"

# Step 4: Clean build directory if requested or generator mismatch
if [[ ${CLEAN_BUILD} -eq 1 ]]; then
    echo "--> Cleaning existing build directory: ${BUILD_DIR}"
    rm -rf "${BUILD_DIR}"
fi

CMAKE_GENERATOR_ARGS=()
HAS_CUSTOM_G=0
for arg in "${EXTRA_CMAKE_ARGS[@]+"${EXTRA_CMAKE_ARGS[@]}"}"; do
    if [[ "${arg}" == "-G" || "${arg}" =~ ^-G.+ ]]; then
        HAS_CUSTOM_G=1
        break
    fi
done

if [[ ${HAS_CUSTOM_G} -eq 0 ]]; then
    CMAKE_GENERATOR_ARGS=("-G" "Ninja")
    if [[ -f "${BUILD_DIR}/CMakeCache.txt" ]]; then
        CACHE_GEN=$(grep -E "^CMAKE_GENERATOR:INTERNAL=" "${BUILD_DIR}/CMakeCache.txt" 2>/dev/null | cut -d= -f2 || true)
        if [[ -n "${CACHE_GEN}" && "${CACHE_GEN}" != "Ninja" ]]; then
            echo "--> Generator mismatch in existing cache (${CACHE_GEN} vs Ninja). Recreating ${BUILD_DIR}..."
            rm -rf "${BUILD_DIR}"
        fi
    fi
fi

# Step 5: Configure CMake with Emscripten toolchain
CMAKE_CONFIG_ARGS=(
    -S "${GAME_ROOT}"
    -B "${BUILD_DIR}"
    ${CMAKE_GENERATOR_ARGS[@]+"${CMAKE_GENERATOR_ARGS[@]}"}
    -DCMAKE_BUILD_TYPE="${CONFIG}"
    -DCMAKE_FIND_ROOT_PATH_MODE_PACKAGE=BOTH
)

if [[ -n "${SDK_PATH}" ]]; then
    CMAKE_CONFIG_ARGS+=("-DCMAKE_PREFIX_PATH=${SDK_PATH}")
fi

if [[ "${CONFIG}" == "Release" || "${CONFIG}" == "RelWithDebInfo" || "${CONFIG}" == "MinSizeRel" ]]; then
    CMAKE_CONFIG_ARGS+=("-DAION_ENABLE_CONSOLE_LOG=OFF")
else
    CMAKE_CONFIG_ARGS+=("-DAION_ENABLE_CONSOLE_LOG=ON")
fi

echo "--> Configuring CMake with Emscripten Toolchain..."
if ! emcmake cmake "${CMAKE_CONFIG_ARGS[@]}" ${EXTRA_CMAKE_ARGS[@]+"${EXTRA_CMAKE_ARGS[@]}"}; then
    EXIT_CODE=$?
    echo "" >&2
    echo "========================================================================" >&2
    echo "CMake configuration failed with exit code ${EXIT_CODE}." >&2
    echo "" >&2
    echo "If the error indicates that package 'AionEngine' was not found or is incompatible:" >&2
    echo "  - Ensure the Aion Web SDK distribution has been built and staged:" >&2
    echo "      cd <path-to-aion>" >&2
    echo "      ./build_web.sh --config Release" >&2
    echo "  - Provide the Web SDK path explicitly:" >&2
    echo "      ./build_web.sh --sdk-path <path-to-aion>/dist/sdk/AionSDK" >&2
    echo "========================================================================" >&2
    exit ${EXIT_CODE}
fi

# Step 6: Build target
echo "--> Compiling target '${TARGET}'..."
cmake --build "${BUILD_DIR}" --target "${TARGET}" --parallel

# Step 7: Validate generated artifacts
HTML_ARTIFACT="${BUILD_DIR}/${TARGET}.html"
JS_ARTIFACT="${BUILD_DIR}/${TARGET}.js"
WASM_ARTIFACT="${BUILD_DIR}/${TARGET}.wasm"
DATA_ARTIFACT="${BUILD_DIR}/${TARGET}.data"

for art in "${HTML_ARTIFACT}" "${JS_ARTIFACT}" "${WASM_ARTIFACT}" "${DATA_ARTIFACT}"; do
    if [[ ! -f "${art}" ]]; then
        echo "Error: Expected build artifact missing: ${art}" >&2
        exit 1
    fi
    if [[ ! -s "${art}" ]]; then
        echo "Error: Generated build artifact is empty (0 bytes): ${art}" >&2
        exit 1
    fi
done

# Step 8: Stage clean, ready-to-host web export directory
echo "--> Staging clean web export to: ${EXPORT_DIR}"
rm -rf "${EXPORT_DIR}"
mkdir -p "${EXPORT_DIR}"

cp "${HTML_ARTIFACT}" "${EXPORT_DIR}/index.html"
cp "${HTML_ARTIFACT}" "${EXPORT_DIR}/${TARGET}.html"
cp "${JS_ARTIFACT}"   "${EXPORT_DIR}/${TARGET}.js"
cp "${WASM_ARTIFACT}" "${EXPORT_DIR}/${TARGET}.wasm"
cp "${DATA_ARTIFACT}" "${EXPORT_DIR}/${TARGET}.data"

# Step 9: Generate provenance export manifest
compute_sha256() {
    local target_file="$1"
    if command -v sha256sum &>/dev/null; then
        sha256sum "${target_file}" | awk '{print $1}'
    elif command -v shasum &>/dev/null; then
        shasum -a 256 "${target_file}" | awk '{print $1}'
    else
        cmake -E sha256sum "${target_file}" | awk '{print $1}'
    fi
}

INDEX_HASH="$(compute_sha256 "${EXPORT_DIR}/index.html")"
TARGET_HTML_HASH="$(compute_sha256 "${EXPORT_DIR}/${TARGET}.html")"
JS_HASH="$(compute_sha256 "${EXPORT_DIR}/${TARGET}.js")"
WASM_HASH="$(compute_sha256 "${EXPORT_DIR}/${TARGET}.wasm")"
DATA_HASH="$(compute_sha256 "${EXPORT_DIR}/${TARGET}.data")"
BUILD_TIMESTAMP="$(date -u +"%Y-%m-%dT%H:%M:%SZ" 2>/dev/null || date +"%Y-%m-%dT%H:%M:%SZ")"
GIT_COMMIT="$(git -C "${GAME_ROOT}" rev-parse HEAD 2>/dev/null || echo "unknown")"

MANIFEST_FILE="${EXPORT_DIR}/export_manifest.json"
cat << EOF > "${MANIFEST_FILE}"
{
  "project": "BlockJumper",
  "version": "1.0.0",
  "target": "${TARGET}",
  "config": "${CONFIG}",
  "timestamp": "${BUILD_TIMESTAMP}",
  "git_commit": "${GIT_COMMIT}",
  "artifacts": {
    "index.html": "${INDEX_HASH}",
    "${TARGET}.html": "${TARGET_HTML_HASH}",
    "${TARGET}.js": "${JS_HASH}",
    "${TARGET}.wasm": "${WASM_HASH}",
    "${TARGET}.data": "${DATA_HASH}"
  }
}
EOF

# Step 10: Validate staged package
echo "--> Validating export package..."
REQUIRED_EXPORTS=(
    "${EXPORT_DIR}/index.html"
    "${EXPORT_DIR}/${TARGET}.html"
    "${EXPORT_DIR}/${TARGET}.js"
    "${EXPORT_DIR}/${TARGET}.wasm"
    "${EXPORT_DIR}/${TARGET}.data"
    "${EXPORT_DIR}/export_manifest.json"
)

for file in "${REQUIRED_EXPORTS[@]}"; do
    if [[ ! -f "${file}" || ! -s "${file}" ]]; then
        echo "Error: Export validation failed for: ${file}" >&2
        exit 1
    fi
done

echo ""
echo "========================================================================"
echo "                   Export Completed Successfully!                      "
echo "========================================================================"
echo "Distribution directory ready for static hosting:"
echo "  ${EXPORT_DIR}"
echo ""
echo "Exported Files:"
ls -lh "${EXPORT_DIR}"
echo ""
echo "To test locally using Python's static HTTP server:"
echo "  python3 -m http.server 8080 --directory \"${EXPORT_DIR}\""
echo ""
echo "Then open in any WebGL 2 compatible browser:"
echo "  http://localhost:8080/"
echo "========================================================================"
