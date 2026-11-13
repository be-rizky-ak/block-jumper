# Block Jumper (Isometric 2.5D Showcase)

**Block Jumper** is an official showcase example for the **Aion Engine SDK**, demonstrating how to build a complete, polished 2.5D isometric arcade platformer with procedural endless course generation, dynamic multi-tier difficulty scaling, responsive camera-relative kinematics, animated 3D character controller, panoramic equirectangular skyboxes, Kenney vector UI, spatial audio integration, and persistent settings.

---

## Showcase Overview & Architecture

Block Jumper demonstrates the core subsystems and design patterns of the Aion Engine SDK:
- **Application & Lifecycle:** Managed initialization, fixed-timestep simulation loop, event dispatch, and graceful shutdown via `Aion::Application`.
- **Startup Reliability & OpenGL Validation:** Validates window creation, checks OpenGL 3.3 Core Profile compliance, and provides actionable diagnostics on startup failures.
- **Resource Discovery & Portability:** Centralized, deterministic resource discovery (`ResourcePath`) that functions seamlessly across development workspaces, packaged installs, and arbitrary working directories.
- **Scene Graph & Transforms:** Hierarchical scene management, entity transforms, and model instantiation with `Aion::Scene` and `Aion::Entity`.
- **Elevated Isometric Camera:** Fixed diagonal viewing angle (pitch ~-35.264°, yaw 45°) with smooth player-following damping (`FollowCamera`).
- **Rendering & PBR Shading:** 3-tone directional sunlight balancing direct and ambient lighting, stabilized shadow maps, and 360°×180° equirectangular environment mapping.
- **Procedural Endless Progression:** Segment-based procedural level generation chaining modular courses, difficulty-based pattern selection (Stages 1–5), and bounded dynamic recycling (`LevelManager`).
- **Physics & Kinematics:** Custom predictive AABB collision response supporting varying platform elevations, one-way ledges, solid obstacles, and precise jump trajectories (`PlayerController`, `PlayerCollider`).
- **3D Skeletal Animation:** Blend-tree animation transitions (Idle, Run, Jump, Fall, Land) powered by `PlayerAnimationController`.
- **Audio Subsystem:** Multi-category volume mixing (Master, Music, SFX), background music streaming, and pre-decoded low-latency sound effects (`Aion::AudioSystem`).
- **Vector UI & High DPI:** Direct SVG rendering with Kenney UI Pack, Ken-Future vector font glyph atlas, HUD overlays, and responsive modal dialogs (`GameOverUI`, `OptionsUI`).
- **State Persistence:** Safe JSON serialization of best distance records and audio preferences (`SaveManager`).

---

## Gameplay & Controls

Traverse an endless series of floating platforms that dynamically scale in difficulty. Misjudging a jump or stepping off an edge results in a fall and Game Over, allowing instant retry to beat your best distance.

| Key | Action |
| --- | --- |
| `W` / `Up Arrow` | Move forward relative to camera (diagonal up-right) |
| `S` / `Down Arrow` | Move backward relative to camera (diagonal down-left) |
| `A` / `Left Arrow` | Move left relative to camera (diagonal up-left) |
| `D` / `Right Arrow` | Move right relative to camera (diagonal down-right) |
| `Space` | Jump (grounded only) |
| `Escape` | Open / Close Options window |

---

## Project Structure

```text
BlockJumper/
├── CMakeLists.txt              # Standard CMake build script consuming public Aion SDK
├── README.md                   # Project documentation, build and run guide
├── ROADMAP.md                  # Milestone and feature roadmap
├── LICENSE                     # MIT License
├── THIRD-PARTY-NOTICES.md      # Third-party asset credits and CC0 notices
├── assets/                     # Packaged runtime assets
│   ├── audio/                  # Background music (MP3) and sound effects (OGG)
│   ├── models/                 # 3D models (GLB) for character, platforms, decorations
│   ├── shaders/                # Standard PBR shaders (copied during build)
│   └── textures/               # Skybox panoramas (PNG) and UI vector assets (SVG/PNG)
└── src/                        # Production game source code
    ├── main.cpp                # Entry point, CLI parser, context validation
    ├── camera/                 # FollowCamera system
    ├── game/                   # App lifecycle, GameScene, LevelManager, ResourcePath, Config
    ├── player/                 # PlayerController, Collider, AnimationController
    ├── ui/                     # GameOverUI, OptionsUI, LoadingScreen
    └── vfx/                    # VisualEffectsSystem
```

---

## Prerequisites

### Native Desktop Development (Windows & Linux)
- **Operating System:** Windows 10/11 (64-bit) or Linux (Ubuntu 20.04+, Debian 11+, Fedora 36+, Arch).
- **C++ Compiler:** Standard C++20 compliant compiler:
  - MSVC from Visual Studio 2019 / 2022 (`/std:c++20`) on Windows.
  - GCC 11 or newer (GCC 13 tested) on Linux.
  - Clang 14 or newer on Linux.
- **Build System:** CMake 3.16 or newer.
- **Graphics:** GPU and driver supporting **OpenGL 3.3 Core Profile** (or higher).
- **Aion Engine SDK:** Pre-built native Aion Engine SDK 1.2.0 package (`AionSDK`).
- *Neither Emscripten nor Ninja is required for native desktop development.*

### Optional Web Build Development Tools (WebAssembly & WebGL 2)
Required **only** when explicitly building or exporting the Web target:
- **Emscripten SDK (`emsdk`):** Supported versions **3.1.50 through 3.1.64+**.
  - Windows: Clone to `$env:USERPROFILE\emsdk`, run `.\emsdk.bat install 3.1.64` and `.\emsdk.bat activate 3.1.64`.
  - Linux/macOS: Clone to `~/emsdk`, run `./emsdk install 3.1.64` and `./emsdk activate 3.1.64`.
- **Ninja:** High-performance build generator required for Emscripten builds (`-G Ninja`).
  - Windows: `winget install Ninja-multiconfig.Ninja`, `choco install ninja`, or Visual Studio CMake tools.
  - Linux: `sudo apt-get install ninja-build` (Debian/Ubuntu) or `sudo dnf install ninja-build` (Fedora).
  - macOS: `brew install ninja`.

---

## Player Requirements (Web Build)

To play the browser-based WebGL 2 export, players only need:
- **A modern web browser with WebGL 2 support** (e.g. Google Chrome, Microsoft Edge, Mozilla Firefox, or Safari 15+).
- **An active internet connection** (or local network connection) to download the hosted game.

**Players do NOT need:**
- Emscripten or Ninja
- CMake or build tools
- A C++ compiler
- The Aion Engine SDK
- The source code repository

---

## Aion SDK Setup

Block Jumper consumes the public Aion Engine SDK via standard CMake `find_package(AionEngine CONFIG REQUIRED)`.

### Specifying the SDK Location

You can provide the path to your Aion SDK installation using any of the following standard methods:

1. **CMake Prefix Path (Recommended):**
   Pass `-DCMAKE_PREFIX_PATH=<path_to_sdk>` when configuring:
   ```bash
   cmake -S . -B build -DCMAKE_PREFIX_PATH="/opt/AionSDK"
   ```

2. **Environment Variable:**
   Set `AION_SDK_DIR` in your environment:
   ```bash
   export AION_SDK_DIR="/opt/AionSDK"
   # On Windows PowerShell:
   # $env:AION_SDK_DIR = "C:\AionSDK"
   ```

---

## Building Block Jumper

### 1. Native Windows Build (MSVC)

Native Windows development does not require Emscripten, Ninja, or any Web tools.

```cmd
cmake -S . -B build -A x64 -DCMAKE_PREFIX_PATH="C:/AionSDK"
cmake --build build --config Release
```

Or run the provided PowerShell / batch build entry points:
```powershell
# PowerShell
.\build.ps1 -BuildConfig Release -SdkPath C:\AionSDK

# Command Prompt
build.bat Release --sdk-path C:\AionSDK
```

The resulting executable will be placed in `build\Release\BlockJumper.exe` (or `build\BlockJumper.exe`).

### 2. Native Linux Build (GCC or Clang)

```bash
# Using GCC
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="/path/to/AionSDK"
cmake --build build -j$(nproc)

# Using Clang
CC=clang CXX=clang++ cmake -S . -B build-clang -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="/path/to/AionSDK"
cmake --build build-clang -j$(nproc)
```

### 3. WebAssembly & WebGL 2 Web Build

Block Jumper can be compiled to a standalone WebAssembly/WebGL 2 web application using the official Aion SDK toolchain. Both native and Web builds consume the same **unified Aion SDK root** (`AionSDK`), with target platform selection handled internally:
- Native builds automatically select the `Windows` distribution.
- WebAssembly builds automatically select the `Web` distribution.

#### On Windows (PowerShell)

Ensure the Emscripten SDK and Ninja are installed, then run:

```powershell
# Build in Release mode (auto-discovers unified Aion SDK root):
.\build_web.ps1

# Specify explicit unified SDK root (same path used for native build.bat / build.ps1):
.\build_web.ps1 -SdkPath "C:\Development\AionSDK"

# Clean rebuild:
.\build_web.ps1 -Clean

# Export to custom directory:
.\build_web.ps1 -ExportDir "dist\web"
```

#### On Linux / macOS (Bash)

```bash
# Activate Emscripten SDK:
source ~/emsdk/emsdk_env.sh

# Build Release web target (auto-discovers unified Aion SDK root):
./build_web.sh

# Build with explicit unified SDK root (same path used for native CMake):
./build_web.sh -SdkPath /path/to/aion/dist/sdk/AionSDK

# Clean build:
./build_web.sh --clean
```

#### Web Export Directory Structure

The ready-to-host web application is isolated in `build_web/export/` (or your custom `-ExportDir`):

```text
build_web/export/
├── index.html            # Responsive HTML5 entry point with 16:9 canvas and audio unlock
├── BlockJumper.html      # Direct named entry point
├── BlockJumper.js        # Emscripten JavaScript runtime glue and loader
├── BlockJumper.wasm      # Compiled WebAssembly binary
├── BlockJumper.data      # Preloaded Virtual File System assets (models, textures, audio, shaders)
└── export_manifest.json  # Provenance manifest with SHA-256 artifact checksums
```

#### Testing the Web Build Locally

Due to browser security and Cross-Origin policies, WebAssembly cannot run directly from the local file system (`file://`). Serve the export folder with a local HTTP server:

```bash
# Using Python 3:
python3 -m http.server 8080 --directory build_web/export

# Or on Windows PowerShell:
python -m http.server 8080 --directory "build_web\export"
```

Then open `http://localhost:8080/` in any modern web browser.

#### Deploying to Static Hosting

- **Static Hosting Providers:** Upload the entire contents of `build_web/export/` to any static web hosting provider (such as GitHub Pages, Netlify, Vercel, AWS S3 / CloudFront, Cloudflare Pages, Apache, or Nginx).
- **Subdirectory Hosting:** All scripts and asset references use relative paths. Hosting from subdirectories (e.g., `https://example.com/games/block-jumper/`) works out of the box without URL rewriting.
- **Server MIME Types:** Ensure the web server serves `.wasm` with `application/wasm` and `.data` with `application/octet-stream`. Modern static hosting providers handle this automatically.
- **No Special Headers Required:** The game runs in single-threaded WebAssembly mode without `SharedArrayBuffer` or pthreads, so no `Cross-Origin-Opener-Policy` (COOP) or `Cross-Origin-Embedder-Policy` (COEP) isolation headers are needed.

### Windows Subsystem & Console Logging Behavior

Block Jumper configures its Windows production executable subsystem and console diagnostic logging based on the build configuration:

| Configuration | Windows Subsystem | Console Window | Aion Engine Diagnostic Logging |
|---|---|---|---|
| `Debug` | Console (`/SUBSYSTEM:CONSOLE`) | Available for development | Enabled (`AION_ENABLE_CONSOLE_LOG=ON`) |
| `Release` | Windows GUI (`/SUBSYSTEM:WINDOWS`) | No standalone console window | Disabled (`AION_ENABLE_CONSOLE_LOG=OFF`) |
| `RelWithDebInfo` | Windows GUI (`/SUBSYSTEM:WINDOWS`) | No standalone console window | Disabled (`AION_ENABLE_CONSOLE_LOG=OFF`) |
| `MinSizeRel` | Windows GUI (`/SUBSYSTEM:WINDOWS`) | No standalone console window | Disabled (`AION_ENABLE_CONSOLE_LOG=OFF`) |

- **Standalone GUI Game Launch:** Launching a Windows Release build from Windows Explorer runs cleanly without displaying an unwanted command prompt window.
- **Terminal Execution:** The application can still be launched from an existing terminal (`cmd.exe`, PowerShell, or bash), accepting all command-line arguments without spawning an extra console window.
- **Diagnostic Logging Override:** You can explicitly override engine console logging at CMake configure time via `-DAION_ENABLE_CONSOLE_LOG=ON` or `-DAION_ENABLE_CONSOLE_LOG=OFF`:
  ```cmd
  cmake -S . -B build -A x64 -DCMAKE_BUILD_TYPE=Release -DAION_ENABLE_CONSOLE_LOG=ON
  ```
- **Test Executable Subsystem:** The test executable (`BlockJumperTest`) always retains the standard console subsystem across all configurations.

---

## Running the Game

The build process automatically stages all required assets (`models/`, `textures/`, `audio/`, `shaders/`) directly alongside the generated executable.

### Standard Launch

Run the executable from any directory:

```bash
# On Linux:
./build/BlockJumper

# On Windows:
.\build\Release\BlockJumper.exe
```

### Working Directory Independence

Block Jumper includes centralized resource discovery and will automatically locate its assets whether launched from the repository root, the build directory, or an arbitrary working directory (e.g., `/tmp`).

If running from an isolated custom layout, you can override the asset discovery directory via CLI or environment variable:
```bash
# Command line flag
./build/BlockJumper --assets /path/to/assets

# Environment variable override
BLOCK_JUMPER_ASSETS_DIR=/path/to/assets ./build/BlockJumper
```

---

## Command-Line Options

Block Jumper provides command-line options for testing, automation, and asset discovery:

| Flag | Argument | Description |
| --- | --- | --- |
| `--help` | None | Displays command-line usage information and exits. |
| `--test` | None | Runs in automated test mode, executing initialization, headless simulation, and verification checks before exiting. |
| `--duration` | `<seconds>` | Sets the run duration in seconds when combined with `--test` (e.g. `--duration 30.0`). Defaults to 0.0s (single-tick verification). |
| `--assets` | `<path>` | Explicitly sets the runtime asset directory root, bypassing auto-discovery. |

### Examples

Run automated self-tests:
```bash
./build/BlockJumper --test
```

Run headless stress test under Xvfb for 15 seconds:
```bash
xvfb-run -a ./build/BlockJumper --test --duration 15.0
```

Run test suite:
```bash
xvfb-run -a ./build/BlockJumperTest --test
```

---

## Troubleshooting & Diagnostics

- **OpenGL Context Failure:** If your environment lacks an active display or hardware acceleration (e.g., CI containers or remote servers), run the application using a virtual framebuffer such as Xvfb (`xvfb-run -a ./build/BlockJumper`).
- **Missing Shaders or Assets:** If assets fail to load, ensure the build completed cleanly so that CMake asset copy targets finished. Check console output for `[ResourcePath]` diagnostic logs showing which resolution tier was evaluated.
- **Corrupt Save File:** If `block_jumper_save.json` becomes corrupted, Block Jumper detects the JSON syntax error, logs a clear warning, and safely restores factory default preferences without crashing.
