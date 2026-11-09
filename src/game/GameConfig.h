#pragma once

#include <cmath>
#include <vector>
#include "Aion/Math/Vector3.h"
#include "Aion/Math/Vector4.h"

namespace BlockJumper
{
    /**
     * @brief Centralized player kinematics and jump tuning parameters.
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

        // Dimensions
        Aion::Vector3 Dimensions{1.0f, 1.0f, 1.0f}; // 1x1x1 cube
        Aion::Vector3 HalfExtents{0.5f, 0.5f, 0.5f};

        // Fall limit below which player respawns
        float VoidFallLimit = -12.0f;
        // Spawn at the most bottom-right portion of starting platform from isometric viewpoint
        // Screen-right is (+X, -Z), screen-bottom is (+X, +Z) -> (+X) places player at bottom-right
        Aion::Vector3 SpawnPosition{2.2f, 0.5f, 0.4f};
    };

    /**
     * @brief Platform and obstacle geometry and styling definition.
     */
    struct PlatformBox
    {
        Aion::Vector3 Position{0.0f};
        Aion::Vector3 Dimensions{1.0f};
        Aion::Vector4 Color{0.25f, 0.52f, 0.50f, 1.0f};
        float Metallic = 0.15f;
        float Roughness = 0.75f;
        bool IsSolidObstacle = false; // When true, blocks horizontal traversal
        const char* Name = "Platform";

        float GetTopSurfaceY() const { return Position.y + Dimensions.y * 0.5f; }
        Aion::Vector3 GetMin() const { return Position - Dimensions * 0.5f; }
        Aion::Vector3 GetMax() const { return Position + Dimensions * 0.5f; }
    };

    /**
     * @brief Elevated isometric camera configuration.
     * Fixed diagonal viewing angle (Pitch ~-35.264°, Yaw 45°) exposing top and visible sides.
     */
    struct CameraConfig
    {
        // Elevated diagonal offset in (+X, +Y, +Z) looking toward player
        Aion::Vector3 Offset{14.0f, 16.0f, 14.0f};

        // Fixed isometric viewing angles
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
     * @brief Complete game configuration.
     */
    struct GameConfig
    {
        PlayerConfig Player;
        CameraConfig Camera;

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
