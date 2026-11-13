#pragma once

#include <cmath>
#include <vector>
#include "Aion/Math/Vector3.h"
#include "Aion/Math/Vector4.h"

namespace BlockJumper
{
    /**
     * @brief Window and display configuration settings.
     */
    struct WindowConfig
    {
        const char* Title = "Block Jumper - Isometric 2.5D";
        uint32_t Width = 1280;
        uint32_t Height = 720;
        bool VSync = true;
    };

    /**
     * @brief Player kinematics, movement tuning, and collision volume settings.
     */
    struct PlayerConfig
    {
        // Omnidirectional movement (units/s)
        float MoveSpeed = 6.0f;
        float Acceleration = 24.0f;  // units/s²
        float Deceleration = 28.0f;  // units/s²

        // Jump & Vertical physics
        float JumpHeight = 1.5f;     // units
        float Gravity = 25.0f;       // units/s²
        float JumpVelocity = std::sqrt(2.0f * 25.0f * 1.5f); // ~8.66 units/s

        // Collision volume dimensions (1x1x1 cube)
        Aion::Vector3 Dimensions{1.0f, 1.0f, 1.0f};
        Aion::Vector3 HalfExtents{0.5f, 0.5f, 0.5f};

        // Fall limit below which player simulation terminates / resets
        float VoidFallLimit = -12.0f;

        // World-space fall death threshold below lowest platform (top is -1.0f, drop is 2.8 units)
        float FallDeathThreshold = -5.0f;

        // Intentional duration (seconds) player falls naturally before transitioning to GameOver
        float FallTransitionDuration = 0.8f;

        // Spawn position: bottom-right corner of starting platform framing forward course ahead
        Aion::Vector3 SpawnPosition{2.2f, 0.5f, 0.4f};
    };

    /**
     * @brief Platform box geometry and visual material definition.
     */
    struct PlatformBox
    {
        Aion::Vector3 Position{0.0f};
        Aion::Vector3 Dimensions{1.0f};
        Aion::Vector4 Color{0.25f, 0.52f, 0.50f, 1.0f};
        float Metallic = 0.15f;
        float Roughness = 0.75f;
        bool IsSolidObstacle = false; // When true, blocks horizontal player traversal
        const char* Name = "Platform";

        // Moving platform & dynamic obstacle attributes
        bool IsMoving = false;
        Aion::Vector3 MoveAxis{1.0f, 0.0f, 0.0f};
        float MoveDistance = 0.0f;
        float MoveSpeed = 0.0f;
        float MovePhase = 0.0f;
        Aion::Vector3 BasePosition{0.0f};
        Aion::Vector3 Velocity{0.0f};

        bool IsPeriodicHazard = false;
        float HazardPeriod = 2.0f;
        float HazardActiveTime = 1.0f;
        float HazardTimer = 0.0f;
        bool HazardIsActive = true;

        float GetTopSurfaceY() const { return Position.y + Dimensions.y * 0.5f; }
        Aion::Vector3 GetMin() const { return Position - Dimensions * 0.5f; }
        Aion::Vector3 GetMax() const { return Position + Dimensions * 0.5f; }
    };

    /**
     * @brief Elevated isometric camera configuration.
     * Fixed diagonal viewing angle (Pitch ~-35.264°, Yaw 45.0°) exposing platform tops and camera-facing faces.
     */
    struct CameraConfig
    {
        // Elevated diagonal offset in (+X, +Y, +Z) looking toward player
        Aion::Vector3 Offset{14.0f, 16.0f, 14.0f};

        // Fixed isometric viewing angles in degrees
        float PitchDegrees = -35.264f;
        float YawDegrees = 45.0f;

        // Smooth follow responsiveness
        float FollowSmoothRateHorizontal = 6.0f;
        float FollowSmoothRateVertical = 4.0f;

        // Perspective projection settings (telephoto FOV enhances isometric appearance)
        float Fov = 42.0f;
        float NearPlane = 0.1f;
        float FarPlane = 600.0f;
    };

    /**
     * @brief Endless level progression and segment lifecycle settings.
     */
    struct EndlessConfig
    {
        // Number of upcoming segments to maintain ahead of the player
        int UpcomingSegmentsAhead = 3;

        // Number of completed segments to retain behind the player before recycling
        int SegmentsRetainedBehind = 2;

        // Distance threshold ahead of current segment exit to trigger upcoming generation
        float GenerationDistanceThreshold = 18.0f;

        // Connection jump gap between consecutive segments (units)
        float ConnectionGap = 1.2f;
    };

    /**
     * @brief Difficulty progression and segment pattern selection parameters.
     */
    struct DifficultyConfig
    {
        // Initial difficulty level when starting a new run
        int InitialDifficulty = 1;

        // Configurable maximum difficulty level
        int MaxDifficulty = 10;

        // Number of completed segments required to advance to the next difficulty level
        int SegmentsPerDifficultyLevel = 3;

        // Frequency (every N generated segments) at which an easier recovery segment appears
        int EasierSegmentInterval = 4;

        // Number of recently spawned segment patterns to remember and avoid repeating
        int RepetitionAvoidanceWindow = 3;

        // Verified maximum allowable horizontal jump gap between platforms (units)
        float MaxJumpGapLimit = 2.8f;

        // Verified maximum allowable vertical step ascent climb (units, JumpHeight is 1.5f)
        float MaxAscentHeight = 1.2f;

        // Verified maximum allowable vertical descent drop (units)
        float MaxDescentHeight = 2.8f;

        // Minimum platform horizontal dimension (width/depth) for safe landing area
        float MinPlatformDimension = 1.3f;
    };

    /**
     * @brief Rendering and environment lighting parameters.
     */
    struct RenderingConfig
    {
        // Directional sunlight vector (normalized)
        Aion::Vector3 LightDirection{0.55f, 0.80f, 0.25f};

        // Directional light color and intensity
        Aion::Vector3 LightColor{2.2f, 2.15f, 2.05f};

        // Ambient sky illumination color
        Aion::Vector3 AmbientColor{0.28f, 0.29f, 0.34f};

        // Shadow mapping configuration
        bool ShadowsEnabled = true;
        float ShadowBiasMax = 0.0025f;
        float ShadowBiasMin = 0.0005f;
        int ShadowMapResolution = 2048;

        // Skybox asset paths
        const char* GrassSkyboxPath = "assets/textures/kenney_skyboxes/skyboxes/skybox-day.png";
        const char* SnowSkyboxPath = "assets/textures/kenney_skyboxes/skyboxes/skybox-morning.png";

        // Character model asset path
        const char* CharacterModelPath = "assets/models/kenney_mini-characters/models/character-male-a.glb";
    };

    /**
     * @brief Centralized audio parameters and sound asset paths.
     */
    struct AudioConfig
    {
        // Default category volume levels [0.0f, 1.0f]
        float MasterVolume = 1.0f;
        float MusicVolume = 0.5f;
        float SFXVolume = 0.8f;

        // Supplied audio asset paths
        const char* JumpSFXPath = "assets/audio/kenney_digital-audio/audio/phaserUp1.ogg";
        const char* GameOverSFXPath = "assets/audio/kenney_digital-audio/audio/threeTone1.ogg";
        const char* RetrySFXPath = "assets/audio/kenney_digital-audio/audio/tone1.ogg";
        const char* BackgroundMusicPath = "assets/audio/cotton_toys_by_soundroll/cotton-toys-soundroll-main-version-16753-01-17.mp3";
    };

    /**
     * @brief UI presentation and asset paths.
     */
    struct UIConfig
    {
        const char* FontAtlasPath = "assets/textures/kenney_ui-pack/png/kenney_future_atlas.png";
        const char* VectorUIBasePath = "assets/textures/kenney_ui-pack/vector/";
        float LoadingFadeDuration = 0.45f;
        float GameOverFadeDuration = 0.30f;
    };

    /**
     * @brief Local persistence configuration.
     */
    struct PersistenceConfig
    {
        const char* SaveFileName = "block_jumper_save.json";
        float DefaultBestDistance = 0.0f;
    };

    /**
     * @brief Performance limits and bounds.
     */
    struct PerformanceConfig
    {
        bool EnableFrustumCulling = true;
        int TargetFrameRate = 60;
        float FixedPhysicsTimestep = 1.0f / 60.0f;
    };

    /**
     * @brief Consolidated game configuration combining all subsystem settings.
     */
    struct GameConfig
    {
        WindowConfig Window;
        PlayerConfig Player;
        CameraConfig Camera;
        EndlessConfig Endless;
        DifficultyConfig Difficulty;
        RenderingConfig Rendering;
        AudioConfig Audio;
        UIConfig UI;
        PersistenceConfig Persistence;
        PerformanceConfig Performance;

        static inline const WindowConfig DefaultWindow{};

        static std::vector<PlatformBox> CreateDefaultLevel()
        {
            std::vector<PlatformBox> platforms;
            platforms.reserve(32);

            // ---------------------------------------------------------
            // Stage 1: Starting Platform (Progression Origin)
            // Top surface at Y = 0.0f; player starts at bottom-right (2.2, 0.5, 0.4)
            // ---------------------------------------------------------
            platforms.push_back(PlatformBox{
                .Position = {0.0f, -0.4f, 0.0f},
                .Dimensions = {7.0f, 0.8f, 7.0f},
                .Color = {0.25f, 0.52f, 0.50f, 1.0f}, // Slate Teal
                .Metallic = 0.15f,
                .Roughness = 0.70f,
                .IsSolidObstacle = false,
                .Name = "StartPlatform"
            });

            // Rear guidance curbs marking boundary safely behind player
            platforms.push_back(PlatformBox{
                .Position = {3.1f, 0.15f, 3.1f},
                .Dimensions = {0.6f, 0.3f, 0.6f},
                .Color = {0.85f, 0.78f, 0.65f, 1.0f}, // Sand Stone
                .Metallic = 0.10f,
                .Roughness = 0.85f,
                .IsSolidObstacle = false,
                .Name = "StartCornerSE"
            });
            platforms.push_back(PlatformBox{
                .Position = {0.0f, 0.15f, 3.2f},
                .Dimensions = {4.0f, 0.3f, 0.4f},
                .Color = {0.85f, 0.78f, 0.65f, 1.0f},
                .Metallic = 0.10f,
                .Roughness = 0.85f,
                .IsSolidObstacle = false,
                .Name = "StartBorderS"
            });

            // ---------------------------------------------------------
            // Stage 2: Initial Stepping Stones & First Elevation
            // ---------------------------------------------------------
            // Stepping Stone 1 (Top Y = 0.0f)
            platforms.push_back(PlatformBox{
                .Position = {-5.5f, -0.4f, -3.2f},
                .Dimensions = {2.4f, 0.8f, 2.4f},
                .Color = {0.32f, 0.68f, 0.55f, 1.0f}, // Mint Teal
                .Metallic = 0.10f,
                .Roughness = 0.75f,
                .IsSolidObstacle = false,
                .Name = "Step1_Mint"
            });

            // Stepping Stone 2 - Ascending (Top Y = 0.4f)
            platforms.push_back(PlatformBox{
                .Position = {-8.4f, -0.2f, -5.4f},
                .Dimensions = {2.4f, 1.2f, 2.4f},
                .Color = {0.28f, 0.60f, 0.65f, 1.0f}, // Turquoise
                .Metallic = 0.12f,
                .Roughness = 0.70f,
                .IsSolidObstacle = false,
                .Name = "Step2_Turquoise"
            });

            // Elevated Ledge 1 (Top Y = 1.2f, requires jump to climb)
            platforms.push_back(PlatformBox{
                .Position = {-11.8f, 0.2f, -8.0f},
                .Dimensions = {3.4f, 2.0f, 3.4f},
                .Color = {0.88f, 0.48f, 0.35f, 1.0f}, // Terracotta Coral
                .Metallic = 0.15f,
                .Roughness = 0.65f,
                .IsSolidObstacle = false,
                .Name = "Ledge1_Coral"
            });

            // Marker Accent Pillar on Ledge 1
            platforms.push_back(PlatformBox{
                .Position = {-11.8f, 1.6f, -6.9f},
                .Dimensions = {0.8f, 0.8f, 0.8f},
                .Color = {0.95f, 0.80f, 0.20f, 1.0f}, // Golden Accent
                .Metallic = 0.35f,
                .Roughness = 0.35f,
                .IsSolidObstacle = true,
                .Name = "Ledge1_Marker"
            });

            // ---------------------------------------------------------
            // Stage 3: First One-Way Drop (Backtracking Prevented!)
            // Drop from Ledge1 (Y=1.2) to DropLanding1 (Y=-0.6) is 1.8 units.
            // Exceeds max JumpHeight (1.5 units) -> physically impossible to backtrack!
            // ---------------------------------------------------------
            platforms.push_back(PlatformBox{
                .Position = {-16.0f, -1.0f, -10.5f},
                .Dimensions = {3.6f, 0.8f, 3.6f},
                .Color = {0.20f, 0.45f, 0.58f, 1.0f}, // Deep Cyan
                .Metallic = 0.15f,
                .Roughness = 0.70f,
                .IsSolidObstacle = false,
                .Name = "DropLanding1"
            });

            // ---------------------------------------------------------
            // Stage 4: Lateral Navigation (Zig-Zag Jumping Columns)
            // ---------------------------------------------------------
            // Lateral Column 1 (Steering Left relative to camera) (Top Y = -0.2f)
            platforms.push_back(PlatformBox{
                .Position = {-19.4f, -0.8f, -8.4f},
                .Dimensions = {2.2f, 1.2f, 2.2f},
                .Color = {0.90f, 0.42f, 0.48f, 1.0f}, // Rose Coral
                .Metallic = 0.12f,
                .Roughness = 0.70f,
                .IsSolidObstacle = false,
                .Name = "LateralCol1"
            });

            // Lateral Column 2 (Steering Right relative to camera) (Top Y = 0.2f)
            platforms.push_back(PlatformBox{
                .Position = {-22.5f, -0.6f, -11.5f},
                .Dimensions = {2.2f, 1.6f, 2.2f},
                .Color = {0.92f, 0.70f, 0.22f, 1.0f}, // Golden Amber
                .Metallic = 0.15f,
                .Roughness = 0.65f,
                .IsSolidObstacle = false,
                .Name = "LateralCol2"
            });

            // Mid-Terrace Island (Top Y = 0.8f)
            platforms.push_back(PlatformBox{
                .Position = {-26.2f, -0.2f, -14.2f},
                .Dimensions = {4.0f, 2.0f, 4.0f},
                .Color = {0.28f, 0.65f, 0.48f, 1.0f}, // Emerald Mint
                .Metallic = 0.15f,
                .Roughness = 0.70f,
                .IsSolidObstacle = false,
                .Name = "MidTerrace"
            });

            // Mid-Terrace Beacon Pedestal
            platforms.push_back(PlatformBox{
                .Position = {-26.2f, 1.3f, -12.8f},
                .Dimensions = {0.8f, 1.0f, 0.8f},
                .Color = {0.95f, 0.85f, 0.20f, 1.0f},
                .Metallic = 0.35f,
                .Roughness = 0.40f,
                .IsSolidObstacle = true,
                .Name = "TerraceBeacon"
            });

            // ---------------------------------------------------------
            // Stage 5: Second One-Way Drop (Backtracking Prevented!)
            // Drop from MidTerrace (Y=0.8) to DropLanding2 (Y=-1.0) is 1.8 units.
            // Exceeds max JumpHeight (1.5 units) -> physically impossible to backtrack!
            // ---------------------------------------------------------
            platforms.push_back(PlatformBox{
                .Position = {-30.8f, -1.4f, -16.8f},
                .Dimensions = {3.8f, 0.8f, 3.8f},
                .Color = {0.35f, 0.40f, 0.65f, 1.0f}, // Slate Indigo
                .Metallic = 0.15f,
                .Roughness = 0.70f,
                .IsSolidObstacle = false,
                .Name = "DropLanding2"
            });

            // ---------------------------------------------------------
            // Stage 6: High Ascent Stairway & Citadel Plateau
            // ---------------------------------------------------------
            // Ascending Column 1 (Top Y = -0.2f)
            platforms.push_back(PlatformBox{
                .Position = {-34.4f, -1.0f, -19.4f},
                .Dimensions = {2.2f, 1.6f, 2.2f},
                .Color = {0.45f, 0.38f, 0.75f, 1.0f}, // Royal Indigo
                .Metallic = 0.15f,
                .Roughness = 0.65f,
                .IsSolidObstacle = false,
                .Name = "AscentCol1"
            });

            // Ascending Column 2 (Top Y = 0.8f)
            platforms.push_back(PlatformBox{
                .Position = {-37.5f, -0.5f, -22.2f},
                .Dimensions = {2.4f, 2.6f, 2.4f},
                .Color = {0.58f, 0.35f, 0.82f, 1.0f}, // Amethyst Violet
                .Metallic = 0.15f,
                .Roughness = 0.60f,
                .IsSolidObstacle = false,
                .Name = "AscentCol2"
            });

            // High Citadel Plateau (Top Y = 1.9f)
            platforms.push_back(PlatformBox{
                .Position = {-41.5f, 0.2f, -25.8f},
                .Dimensions = {4.6f, 3.4f, 4.6f},
                .Color = {0.95f, 0.75f, 0.25f, 1.0f}, // Golden Citadel
                .Metallic = 0.20f,
                .Roughness = 0.55f,
                .IsSolidObstacle = false,
                .Name = "CitadelPlateau"
            });

            // Obstacle Hurdle on Citadel (requires jumping over)
            platforms.push_back(PlatformBox{
                .Position = {-41.5f, 2.45f, -25.8f},
                .Dimensions = {0.8f, 1.1f, 3.2f},
                .Color = {0.18f, 0.20f, 0.28f, 1.0f}, // Obsidian Obstacle
                .Metallic = 0.25f,
                .Roughness = 0.50f,
                .IsSolidObstacle = true,
                .Name = "CitadelHurdle"
            });

            // ---------------------------------------------------------
            // Stage 7: Third One-Way Drop (Backtracking Prevented!)
            // Drop from Citadel (Y=1.9) to DropLanding3 (Y=-0.8) is 2.7 units.
            // Exceeds max JumpHeight (1.5 units) -> physically impossible to backtrack!
            // ---------------------------------------------------------
            platforms.push_back(PlatformBox{
                .Position = {-46.5f, -1.3f, -29.2f},
                .Dimensions = {4.0f, 1.0f, 4.0f},
                .Color = {0.22f, 0.58f, 0.72f, 1.0f}, // Sky Cyan
                .Metallic = 0.15f,
                .Roughness = 0.65f,
                .IsSolidObstacle = false,
                .Name = "DropLanding3"
            });

            // ---------------------------------------------------------
            // Stage 8: Apex Crest & Beacon Trophy
            // ---------------------------------------------------------
            platforms.push_back(PlatformBox{
                .Position = {-51.0f, -0.9f, -32.5f},
                .Dimensions = {3.6f, 1.8f, 3.6f},
                .Color = {0.30f, 0.80f, 0.55f, 1.0f}, // Radiant Emerald
                .Metallic = 0.15f,
                .Roughness = 0.65f,
                .IsSolidObstacle = false,
                .Name = "ApexSanctuary"
            });

            // Luminous Beacon Trophy
            platforms.push_back(PlatformBox{
                .Position = {-51.0f, 1.0f, -32.5f},
                .Dimensions = {1.2f, 1.0f, 1.2f},
                .Color = {1.0f, 0.88f, 0.20f, 1.0f}, // Luminous Gold
                .Metallic = 0.50f,
                .Roughness = 0.25f,
                .IsSolidObstacle = true,
                .Name = "ApexTrophy"
            });

            return platforms;
        }
    };
} // namespace BlockJumper
