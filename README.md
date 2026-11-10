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
├── LICENSE                     # MIT License
├── THIRD-PARTY-NOTICES.md      # Third-party asset credits and CC0 notices
├── assets/                     # Packaged runtime assets
│   ├── audio/                  # Background music (MP3) and sound effects (OGG)
│   ├── models/                 # 3D models (GLB) for character, platforms, decorations
│   ├── shaders/                # Standard PBR shaders (copied during build)
│   └── textures/               # Skybox panoramas (PNG) and UI vector assets (SVG/PNG)
├── src/                        # Production game source code
│   ├── main.cpp                # Entry point, CLI parser, context validation
│   ├── camera/                 # FollowCamera system
│   ├── game/                   # App lifecycle, GameScene, LevelManager, ResourcePath, Config
│   ├── player/                 # PlayerController, Collider, AnimationController
│   ├── ui/                     # GameOverUI, OptionsUI, LoadingScreen
│   └── vfx/                    # VisualEffectsSystem
```

---

## Prerequisites

- **Operating System:** Windows 10/11 (64-bit).
- **C++ Compiler:** C++20 compliant compiler:
  - GCC 11 or newer (GCC 13 tested)
  - Clang 14 or newer (Clang 18 tested)
  - MSVC from Visual Studio 2019 / 2022 (`/std:c++20`)
- **Build System:** CMake 3.16 or newer.
- **Graphics:** GPU and driver supporting **OpenGL 3.3 Core Profile** (or higher).
- **Aion Engine SDK:** Pre-built public Aion Engine SDK package (`AionSDK`).

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
   # On Windows: setx AION_SDK_DIR "C:\AionSDK"
   ```

---

## Building Block Jumper

### Windows (MSVC)

```cmd
cmake -S . -B build -A x64 -DCMAKE_PREFIX_PATH="C:/AionSDK"
cmake --build build --config Release
```

Or run the provided `build.bat` / `build.ps1` helper scripts:
```cmd
build.bat Release --sdk-path C:\AionSDK
```

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
