# Block Jumper (Isometric 2.5D)

Initial playable prototype of **Block Jumper**, an isometric 2.5D platformer built using the public **Aion SDK**, inspired by the visual presentation of isometric block-adventure design showcases.

## Controls

| Key | Action |
| --- | --- |
| `W` / `Up Arrow` | Move forward relative to camera |
| `S` / `Down Arrow` | Move backward relative to camera |
| `A` / `Left Arrow` | Move left relative to camera |
| `D` / `Right Arrow` | Move right relative to camera |
| `Space` | Jump (when grounded) |

## Project Structure

```text
BlockJumper/
├── CMakeLists.txt
├── build.bat
├── build.ps1
├── README.md
└── src/
    ├── main.cpp
    ├── game/
    ├── player/
    ├── camera/
    └── ui/
```

## Building and Running

### Prerequisites

- **Operating System:** Windows 10 or 11 (64-bit)
- **Compiler:** Microsoft Visual C++ (MSVC) from Visual Studio 2019 or Visual Studio 2022 with C++20 support (`/std:c++20`)
- **Build System:** CMake 3.16 or newer
- **Graphics:** OpenGL 3.3 Core Profile compatible GPU and graphics drivers
- **SDK:** Aion Engine SDK 1.0.0 (Windows x64)

### Aion SDK Setup

Block Jumper consumes the public Aion Engine SDK via CMake `find_package(AionEngine)`.

1. Download and extract the unified Windows release archive `AionSDK-1.0.0-windows-x86_64.zip` (e.g. to `C:\AionSDK`).
2. Set the `AION_SDK_DIR` environment variable to your extracted SDK directory:
   ```cmd
   setx AION_SDK_DIR "C:\AionSDK"
   ```
   Alternatively, you can pass the SDK path directly to the build scripts using `--sdk-path` or `-SdkPath`.

### Building on Windows

#### Option 1: Using `build.bat` (Command Prompt / Explorer)

Run `build.bat` from the project root (defaults to Release configuration):

```cmd
build.bat
```

To build Debug configuration:
```cmd
build.bat Debug
```

To specify the SDK path explicitly:
```cmd
build.bat Release --sdk-path C:\AionSDK
```

#### Option 2: Using `build.ps1` (PowerShell)

Run `.\build.ps1` from the project root:

```powershell
.\build.ps1
```

To build Debug configuration:
```powershell
.\build.ps1 -BuildConfig Debug
```

To specify the SDK path and perform a clean build:
```powershell
.\build.ps1 -BuildConfig Release -SdkPath C:\AionSDK -Clean
```

#### Option 3: Manual CMake Workflow

```cmd
cmake -S . -B build -A x64 -DCMAKE_PREFIX_PATH=C:/AionSDK
cmake --build build --config Release
```

### Generated Executable Location

Upon successful compilation, the executable and all required runtime assets (3D models, textures, shaders) are automatically staged:
- **Release:** `build\Release\BlockJumper.exe` (with assets in `build\Release\assets\`)
- **Debug:** `build\Debug\BlockJumper.exe` (with assets in `build\Debug\assets\`)

*(Or `build\BlockJumper.exe` if using a single-configuration generator such as Ninja).*

### Running the Game

Launch the generated executable:

```cmd
.\build\Release\BlockJumper.exe
```

Or from PowerShell:
```powershell
.\build\Release\BlockJumper.exe
```

### Known Limitations

- Block Jumper requires a graphics device supporting OpenGL 3.3 Core Profile. Running under virtual machines or environments without hardware acceleration may fail context initialization.
- Game progress is saved automatically to `block_jumper_save.json` in the working directory.
