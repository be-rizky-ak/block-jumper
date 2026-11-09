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
    src/player/PlayerCollider.cpp
    src/player/PlayerController.cpp
    src/camera/FollowCamera.cpp
)

set(BLOCK_JUMPER_HEADERS
    src/game/GameConfig.h
    src/game/BlockJumperApp.h
    src/game/GameScene.h
    src/game/LevelSegment.h
    src/game/LevelManager.h
    src/player/PlayerCollider.h
    src/player/PlayerController.h
    src/camera/FollowCamera.h
)

add_executable(BlockJumper
    ${BLOCK_JUMPER_SOURCES}
    ${BLOCK_JUMPER_HEADERS}
)

target_include_directories(BlockJumper PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Link against public AionEngine target
target_link_libraries(BlockJumper PRIVATE
    Aion::AionEngine
)

# Copy runtime shaders to executable assets directory if available
set(AION_RUNTIME_SHADER_VERT "")
set(AION_RUNTIME_SHADER_FRAG "")
if(EXISTS "${AionEngine_DIR}/../../../share/Aion/shaders/standard.vert")
    set(AION_RUNTIME_SHADER_VERT "${AionEngine_DIR}/../../../share/Aion/shaders/standard.vert")
    set(AION_RUNTIME_SHADER_FRAG "${AionEngine_DIR}/../../../share/Aion/shaders/standard.frag")
elseif(DEFINED ENV{AION_SDK_DIR} AND EXISTS "$ENV{AION_SDK_DIR}/share/Aion/shaders/standard.vert")
    set(AION_RUNTIME_SHADER_VERT "$ENV{AION_SDK_DIR}/share/Aion/shaders/standard.vert")
    set(AION_RUNTIME_SHADER_FRAG "$ENV{AION_SDK_DIR}/share/Aion/shaders/standard.frag")
endif()

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
