cmake_minimum_required(VERSION 3.16)
project(BlockJumper CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Consume Aion through its public CMake package
find_package(AionEngine CONFIG REQUIRED)

# Source files
set(BLOCK_JUMPER_SOURCES
    src/main.cpp
    src/game/BlockJumperApp.cpp
    src/game/GameScene.cpp
    src/game/LevelManager.cpp
    src/game/SaveManager.cpp
    src/player/PlayerCollider.cpp
    src/player/PlayerController.cpp
    src/player/PlayerAnimationController.cpp
    src/camera/FollowCamera.cpp
    src/ui/GameOverUI.cpp
    src/ui/LoadingScreen.cpp
    src/vfx/VisualEffectsSystem.cpp
    src/game/ResourcePath.cpp
    src/ui/OptionsUI.cpp
    src/ui/TouchControlsUI.cpp
)

set(BLOCK_JUMPER_HEADERS
    src/game/GameConfig.h
    src/game/GameState.h
    src/game/BlockJumperApp.h
    src/game/GameScene.h
    src/game/LevelSegment.h
    src/game/LevelManager.h
    src/game/SaveManager.h
    src/player/PlayerCollider.h
    src/player/PlayerController.h
    src/player/PlayerAnimationController.h
    src/camera/FollowCamera.h
    src/ui/GameOverUI.h
    src/ui/LoadingScreen.h
    src/ui/KenneyFontAtlas.h
    src/vfx/VisualEffectsSystem.h
    src/game/ResourcePath.h
    src/ui/OptionsUI.h
    src/ui/TouchControlsUI.h
)

add_executable(BlockJumper
    ${BLOCK_JUMPER_SOURCES}
    ${BLOCK_JUMPER_HEADERS}
)

target_include_directories(BlockJumper PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Compiler warning settings
if(MSVC)
    target_compile_options(BlockJumper PRIVATE /W4 /permissive-)
else()
    target_compile_options(BlockJumper PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Windows executable subsystem:
# - Debug: Console subsystem to provide developer diagnostics during development
# - Release / RelWithDebInfo / MinSizeRel: Windows GUI subsystem (suppresses standalone console window)
if(WIN32 AND NOT EMSCRIPTEN)
    set_target_properties(BlockJumper PROPERTIES
        WIN32_EXECUTABLE $<NOT:$<CONFIG:Debug>>
    )
    if(MSVC)
        target_link_options(BlockJumper PRIVATE
            $<$<NOT:$<CONFIG:Debug>>:/ENTRY:mainCRTStartup>
        )
    endif()
endif()

# Console logging sink configuration:
# - Debug: Enabled by default (AION_ENABLE_CONSOLE_LOG=ON)
# - Release / RelWithDebInfo / MinSizeRel: Disabled by default (AION_ENABLE_CONSOLE_LOG=OFF)
# - Explicit override: -DAION_ENABLE_CONSOLE_LOG=ON or -DAION_ENABLE_CONSOLE_LOG=OFF
set(AION_ENABLE_CONSOLE_LOG "AUTO" CACHE STRING "Enable engine console logging (AUTO, ON, or OFF). Defaults to ON for Debug, OFF for Release/RelWithDebInfo/MinSizeRel.")
set_property(CACHE AION_ENABLE_CONSOLE_LOG PROPERTY STRINGS "AUTO" "ON" "OFF")

# Link against public AionEngine target
target_link_libraries(BlockJumper PRIVATE
    Aion::AionEngine
)

if(NOT AION_ENABLE_CONSOLE_LOG STREQUAL "AUTO" AND NOT AION_ENABLE_CONSOLE_LOG STREQUAL "")
    if(AION_ENABLE_CONSOLE_LOG)
        set(_AION_CONSOLE_LOG_DEF "1")
    else()
        set(_AION_CONSOLE_LOG_DEF "0")
    endif()
    get_target_property(_aion_defs Aion::AionEngine INTERFACE_COMPILE_DEFINITIONS)
    if(_aion_defs)
        string(REGEX REPLACE "AION_ENABLE_CONSOLE_LOG=[^;]+" "AION_ENABLE_CONSOLE_LOG=${_AION_CONSOLE_LOG_DEF}" _aion_defs "${_aion_defs}")
        set_target_properties(Aion::AionEngine PROPERTIES INTERFACE_COMPILE_DEFINITIONS "${_aion_defs}")
    else()
        set_target_properties(Aion::AionEngine PROPERTIES INTERFACE_COMPILE_DEFINITIONS "AION_ENABLE_CONSOLE_LOG=${_AION_CONSOLE_LOG_DEF}")
    endif()
endif()

# Discover runtime shaders from Aion SDK (if not already defined by AionEngineConfig)
if(NOT AION_RUNTIME_SHADER_VERT OR NOT AION_RUNTIME_SHADER_FRAG)
    set(AION_RUNTIME_SHADER_VERT "")
    set(AION_RUNTIME_SHADER_FRAG "")
    if(EXISTS "${AionEngine_DIR}/Common/shaders/standard.vert")
        set(AION_RUNTIME_SHADER_VERT "${AionEngine_DIR}/Common/shaders/standard.vert")
        set(AION_RUNTIME_SHADER_FRAG "${AionEngine_DIR}/Common/shaders/standard.frag")
    elseif(EXISTS "${AionEngine_DIR}/../Common/shaders/standard.vert")
        set(AION_RUNTIME_SHADER_VERT "${AionEngine_DIR}/../Common/shaders/standard.vert")
        set(AION_RUNTIME_SHADER_FRAG "${AionEngine_DIR}/../Common/shaders/standard.frag")
    elseif(EXISTS "${AionEngine_DIR}/../../Common/shaders/standard.vert")
        set(AION_RUNTIME_SHADER_VERT "${AionEngine_DIR}/../../Common/shaders/standard.vert")
        set(AION_RUNTIME_SHADER_FRAG "${AionEngine_DIR}/../../Common/shaders/standard.frag")
    elseif(EXISTS "${AionEngine_DIR}/../../../Common/shaders/standard.vert")
        set(AION_RUNTIME_SHADER_VERT "${AionEngine_DIR}/../../../Common/shaders/standard.vert")
        set(AION_RUNTIME_SHADER_FRAG "${AionEngine_DIR}/../../../Common/shaders/standard.frag")
    elseif(EXISTS "${AionEngine_DIR}/../../../../Common/shaders/standard.vert")
        set(AION_RUNTIME_SHADER_VERT "${AionEngine_DIR}/../../../../Common/shaders/standard.vert")
        set(AION_RUNTIME_SHADER_FRAG "${AionEngine_DIR}/../../../../Common/shaders/standard.frag")
    elseif(EXISTS "${AionEngine_DIR}/../../../share/Aion/shaders/standard.vert")
        set(AION_RUNTIME_SHADER_VERT "${AionEngine_DIR}/../../../share/Aion/shaders/standard.vert")
        set(AION_RUNTIME_SHADER_FRAG "${AionEngine_DIR}/../../../share/Aion/shaders/standard.frag")
    elseif(EXISTS "${AionEngine_DIR}/../../share/Aion/shaders/standard.vert")
        set(AION_RUNTIME_SHADER_VERT "${AionEngine_DIR}/../../share/Aion/shaders/standard.vert")
        set(AION_RUNTIME_SHADER_FRAG "${AionEngine_DIR}/../../share/Aion/shaders/standard.frag")
    elseif(DEFINED ENV{AION_SDK_DIR} AND EXISTS "$ENV{AION_SDK_DIR}/Common/shaders/standard.vert")
        set(AION_RUNTIME_SHADER_VERT "$ENV{AION_SDK_DIR}/Common/shaders/standard.vert")
        set(AION_RUNTIME_SHADER_FRAG "$ENV{AION_SDK_DIR}/Common/shaders/standard.frag")
    elseif(DEFINED ENV{AION_SDK_DIR} AND EXISTS "$ENV{AION_SDK_DIR}/share/Aion/shaders/standard.vert")
        set(AION_RUNTIME_SHADER_VERT "$ENV{AION_SDK_DIR}/share/Aion/shaders/standard.vert")
        set(AION_RUNTIME_SHADER_FRAG "$ENV{AION_SDK_DIR}/share/Aion/shaders/standard.frag")
    endif()
endif()
if(EMSCRIPTEN)
    # Configure Web target using Aion SDK platform configuration
    set(_extra_preloads "")
    if(AION_RUNTIME_SHADER_VERT AND AION_RUNTIME_SHADER_FRAG)
        list(APPEND _extra_preloads
            "${AION_RUNTIME_SHADER_VERT}@/assets/shaders/standard.vert"
            "${AION_RUNTIME_SHADER_FRAG}@/assets/shaders/standard.frag"
        )
    endif()

    aion_configure_web_executable(BlockJumper
        SHELL_FILE "${CMAKE_CURRENT_SOURCE_DIR}/web/shell.html"
        ASSETS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/assets"
        EXTRA_PRELOAD_FILES ${_extra_preloads}
    )
else()
    if(AION_RUNTIME_SHADER_VERT)
        add_custom_command(
            TARGET BlockJumper POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:BlockJumper>/assets/shaders"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${AION_RUNTIME_SHADER_VERT}"
                "${AION_RUNTIME_SHADER_FRAG}"
                "$<TARGET_FILE_DIR:BlockJumper>/assets/shaders/"
            COMMENT "Copying Aion runtime standard shaders to output assets directory..."
        )
    endif()

    # Copy 3D asset models to executable assets directory if present
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/assets/models")
        add_custom_command(
            TARGET BlockJumper POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${CMAKE_CURRENT_SOURCE_DIR}/assets/models"
                "$<TARGET_FILE_DIR:BlockJumper>/assets/models"
            COMMENT "Copying 3D asset models to output assets directory..."
        )
    endif()

    # Copy texture assets to executable assets directory if present
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/assets/textures")
        add_custom_command(
            TARGET BlockJumper POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${CMAKE_CURRENT_SOURCE_DIR}/assets/textures"
                "$<TARGET_FILE_DIR:BlockJumper>/assets/textures"
            COMMENT "Copying texture assets to output assets directory..."
        )
    endif()

    # Copy audio assets to executable assets directory if present
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/assets/audio")
        add_custom_command(
            TARGET BlockJumper POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${CMAKE_CURRENT_SOURCE_DIR}/assets/audio"
                "$<TARGET_FILE_DIR:BlockJumper>/assets/audio"
            COMMENT "Copying audio assets to output assets directory..."
        )
    endif()
endif()
