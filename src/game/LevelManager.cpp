#include "LevelManager.h"
#include "ResourcePath.h"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <iostream>

#include "Aion/Assets/AssetManager.h"
#include "Aion/Assets/Model.h"
#include "Aion/Importers/ModelImporter.h"
#include "Aion/Math/Math.h"
#include "Aion/Math/Quaternion.h"
#include "Aion/Renderer/Material.h"
#include "Aion/Scene/MeshRenderer.h"

namespace BlockJumper
{
    void LevelManager::Initialize(
        Aion::Scene* scene,
        const GameConfig& config,
        std::shared_ptr<Aion::Mesh> sharedCubeMesh)
    {
        m_scene = scene;
        m_config = config;
        m_cubeMesh = std::move(sharedCubeMesh);

        m_activeSegments.clear();
        m_activePlatforms.clear();
        m_currentSegmentIndex = 0;
        m_forwardProgress = 0.0f;
        m_segmentLengths.clear();
        m_segmentCumulativeStarts.clear();
        m_segmentsGenerated = 0;
        m_lastPatternIndex = -1;
        m_lastDecorCompositionIndex = -1;
        m_recentPatternsHistory.clear();
        m_lastSelectedModelName.clear();
        m_lastReportedDifficulty = m_config.Difficulty.InitialDifficulty;

        InitializePatterns();
        LoadPlatformModels();
        LoadEnvironmentModels();
        SpawnStartingPlatform();

        // Pre-generate configured upcoming segments ahead of starting platform
        for (int i = 0; i < m_config.Endless.UpcomingSegmentsAhead; ++i)
        {
            SpawnNextSegment();
        }

        RebuildActivePlatforms();

        std::cout << "[LevelManager] Initialized with " << m_activeSegments.size()
                  << " active segments (" << m_activePlatforms.size()
                  << " platforms total, " << m_patterns.size() << " segment patterns across "
                  << m_config.Difficulty.MaxDifficulty << " difficulty stages)." << std::endl;
    }

    void LevelManager::Reset()
    {
        for (const auto& seg : m_activeSegments)
        {
            RecycleSegment(seg);
        }
        m_activeSegments.clear();
        m_activePlatforms.clear();
        m_currentSegmentIndex = 0;
        m_forwardProgress = 0.0f;
        m_segmentLengths.clear();
        m_segmentCumulativeStarts.clear();
        m_segmentsGenerated = 0;
        m_lastPatternIndex = -1;
        m_lastDecorCompositionIndex = -1;
        m_recentPatternsHistory.clear();
        m_lastSelectedModelName.clear();
        m_lastReportedDifficulty = m_config.Difficulty.InitialDifficulty;
        m_rng = std::mt19937{1337};

        SpawnStartingPlatform();
        for (int i = 0; i < m_config.Endless.UpcomingSegmentsAhead; ++i)
        {
            SpawnNextSegment();
        }
        RebuildActivePlatforms();

        std::cout << "[LevelManager] Reset run: difficulty reset to Level "
                  << m_config.Difficulty.InitialDifficulty
                  << " (" << GetDifficultyStageName(m_config.Difficulty.InitialDifficulty) << ")." << std::endl;
    }

    int LevelManager::CalculateCurrentDifficulty() const
    {
        int baseDiff = m_config.Difficulty.InitialDifficulty;
        int segmentsPerLevel = std::max(1, m_config.Difficulty.SegmentsPerDifficultyLevel);
        int calculated = baseDiff + (m_currentSegmentIndex / segmentsPerLevel);
        return std::clamp(calculated, 1, m_config.Difficulty.MaxDifficulty);
    }

    int LevelManager::GetCurrentDifficulty() const
    {
        return CalculateCurrentDifficulty();
    }

    const char* LevelManager::GetDifficultyStageName(int difficulty) const
    {
        switch (difficulty)
        {
            case 1: return "Easy";
            case 2: return "Normal";
            case 3: return "Challenging";
            case 4: return "Hard";
            case 5: return "Expert";
            case 6: return "Moving Platforms";
            case 7: return "Complex Motion";
            case 8: return "Dynamic Hazards";
            case 9: return "Combined Trial";
            case 10: return "Mastery";
            default: return "Custom";
        }
    }

    void LevelManager::InitializePatterns()
    {
        m_patterns.clear();
        m_patterns.reserve(28);

        // Pattern 0: GentleRightArc (Difficulty 1)
        {
            SegmentPattern p;
            p.Name = "GentleRightArc";
            p.MinDifficulty = 1;
            p.MaxDifficulty = 1;
            p.Weight = 1.0f;
            p.LocalEntryPoint = {0.0f, 0.0f, 0.0f};

            p.LocalPlatforms.push_back(PlatformBox{
                .Position = {0.00f, -0.40f, -0.00f},
                .Dimensions = {4.40f, 0.80f, 4.20f},
                .Color = {0.25f, 0.52f, 0.50f, 1.0f},
                .Metallic = 0.15f,
                .Roughness = 0.70f,
                .IsSolidObstacle = false,
                .Name = "RightArc_Start"
            });

            p.LocalPlatforms.push_back(PlatformBox{
                .Position = {-3.39f, -0.85f, -5.37f},
                .Dimensions = {4.20f, 0.80f, 4.00f},
                .Color = {0.20f, 0.45f, 0.58f, 1.0f},
                .Metallic = 0.15f,
                .Roughness = 0.70f,
                .IsSolidObstacle = false,
                .Name = "RightArc_Step1"
            });

            p.LocalPlatforms.push_back(PlatformBox{
                .Position = {-7.14f, -0.45f, -10.68f},
                .Dimensions = {4.40f, 1.00f, 4.20f},
                .Color = {0.28f, 0.65f, 0.48f, 1.0f},
                .Metallic = 0.15f,
                .Roughness = 0.70f,
                .IsSolidObstacle = false,
                .Name = "RightArc_Step2"
            });

            p.LocalPlatforms.push_back(PlatformBox{
                .Position = {-11.60f, -0.40f, -16.12f},
                .Dimensions = {4.80f, 0.80f, 4.40f},
                .Color = {0.25f, 0.52f, 0.50f, 1.0f},
                .Metallic = 0.15f,
                .Roughness = 0.70f,
                .IsSolidObstacle = false,
                .Name = "RightArc_End"
            });

            p.LocalExitPoint = {-16.69f, 0.00f, -21.50f};
            m_patterns.push_back(std::move(p));
        }

        // Pattern 1: GentleLeftArc (Difficulty 1)
        {
            SegmentPattern p;
            p.Name = "GentleLeftArc";
            p.MinDifficulty = 1;
            p.MaxDifficulty = 1;
            p.Weight = 1.0f;
            p.LocalEntryPoint = {0.0f, 0.0f, 0.0f};

            p.LocalPlatforms.push_back(PlatformBox{
                .Position = {0.00f, -0.40f, -0.00f},
                .Dimensions = {4.40f, 0.80f, 4.20f},
                .Color = {0.32f, 0.68f, 0.55f, 1.0f},
                .Metallic = 0.15f,
                .Roughness = 0.70f,
                .IsSolidObstacle = false,
                .Name = "LeftArc_Start"
            });

            p.LocalPlatforms.push_back(PlatformBox{
                .Position = {-5.59f, -0.10f, -3.61f},
                .Dimensions = {4.20f, 1.30f, 4.40f},
                .Color = {0.28f, 0.60f, 0.65f, 1.0f},
                .Metallic = 0.15f,
                .Roughness = 0.70f,
                .IsSolidObstacle = false,
                .Name = "LeftArc_Step1"
            });

            p.LocalPlatforms.push_back(PlatformBox{
                .Position = {-11.10f, -0.35f, -7.57f},
                .Dimensions = {4.40f, 1.00f, 4.20f},
                .Color = {0.20f, 0.45f, 0.58f, 1.0f},
                .Metallic = 0.15f,
                .Roughness = 0.70f,
                .IsSolidObstacle = false,
                .Name = "LeftArc_Step2"
            });

            p.LocalPlatforms.push_back(PlatformBox{
                .Position = {-16.83f, -0.80f, -12.30f},
                .Dimensions = {4.80f, 0.80f, 4.40f},
                .Color = {0.28f, 0.65f, 0.48f, 1.0f},
                .Metallic = 0.15f,
                .Roughness = 0.70f,
                .IsSolidObstacle = false,
                .Name = "LeftArc_End"
            });

            p.LocalExitPoint = {-22.42f, 0.00f, -17.61f};
            m_patterns.push_back(std::move(p));
        }

        // Pattern 2: WideZigZag (Difficulty 1)
        {
            SegmentPattern p;
            p.Name = "WideZigZag";
            p.MinDifficulty = 1;
            p.MaxDifficulty = 1;
            p.Weight = 1.0f;
            p.LocalEntryPoint = {0.0f, 0.0f, 0.0f};

            p.LocalPlatforms.push_back(PlatformBox{
                .Position = {0.00f, -0.40f, -0.00f},
                .Dimensions = {4.40f, 0.80f, 4.20f},
                .Color = {0.32f, 0.68f, 0.55f, 1.0f},
                .Metallic = 0.15f,
                .Roughness = 0.70f,
                .IsSolidObstacle = false,
                .Name = "WideZZ_Start"
            });

            p.LocalPlatforms.push_back(PlatformBox{
                .Position = {-2.97f, -0.15f, -5.52f},
                .Dimensions = {4.00f, 1.30f, 4.20f},
                .Color = {0.88f, 0.48f, 0.35f, 1.0f},
                .Metallic = 0.15f,
                .Roughness = 0.70f,
                .IsSolidObstacle = false,
                .Name = "WideZZ_Right"
            });

            p.LocalPlatforms.push_back(PlatformBox{
                .Position = {-8.56f, -0.60f, -6.58f},
                .Dimensions = {4.20f, 0.80f, 4.00f},
                .Color = {0.95f, 0.75f, 0.25f, 1.0f},
                .Metallic = 0.15f,
                .Roughness = 0.70f,
                .IsSolidObstacle = false,
                .Name = "WideZZ_Left"
            });

            p.LocalPlatforms.push_back(PlatformBox{
                .Position = {-11.38f, -0.30f, -12.09f},
                .Dimensions = {4.60f, 1.00f, 4.40f},
                .Color = {0.30f, 0.80f, 0.55f, 1.0f},
                .Metallic = 0.15f,
                .Roughness = 0.70f,
                .IsSolidObstacle = false,
                .Name = "WideZZ_End"
            });

            p.LocalExitPoint = {-17.04f, 0.00f, -17.04f};
            m_patterns.push_back(std::move(p));
        }

        // Pattern 3: GentleDipMeander (Difficulty 1)
        {
            SegmentPattern p;
            p.Name = "GentleDipMeander";
            p.MinDifficulty = 1;
            p.MaxDifficulty = 1;
            p.Weight = 1.0f;
            p.LocalEntryPoint = {0.0f, 0.0f, 0.0f};

            p.LocalPlatforms.push_back(PlatformBox{
                .Position = {0.00f, -0.40f, -0.00f},
                .Dimensions = {4.60f, 0.80f, 4.40f},
                .Color = {0.25f, 0.52f, 0.50f, 1.0f},
                .Metallic = 0.15f,
                .Roughness = 0.70f,
                .IsSolidObstacle = false,
                .Name = "Dip_Start"
            });

            p.LocalPlatforms.push_back(PlatformBox{
                .Position = {-5.59f, -0.90f, -3.04f},
                .Dimensions = {4.00f, 0.80f, 4.00f},
                .Color = {0.28f, 0.60f, 0.65f, 1.0f},
                .Metallic = 0.15f,
                .Roughness = 0.70f,
                .IsSolidObstacle = false,
                .Name = "Dip_Low"
            });

            p.LocalPlatforms.push_back(PlatformBox{
                .Position = {-6.65f, -0.45f, -8.63f},
                .Dimensions = {4.00f, 1.00f, 4.20f},
                .Color = {0.32f, 0.68f, 0.55f, 1.0f},
                .Metallic = 0.15f,
                .Roughness = 0.70f,
                .IsSolidObstacle = false,
                .Name = "Dip_Rise"
            });

            p.LocalPlatforms.push_back(PlatformBox{
                .Position = {-12.30f, -0.40f, -12.30f},
                .Dimensions = {4.80f, 0.80f, 4.60f},
                .Color = {0.28f, 0.65f, 0.48f, 1.0f},
                .Metallic = 0.15f,
                .Roughness = 0.70f,
                .IsSolidObstacle = false,
                .Name = "Dip_End"
            });

            p.LocalExitPoint = {-17.75f, 0.00f, -17.75f};
            m_patterns.push_back(std::move(p));
        }

        // Pattern 4: ModerateZigZag (Difficulty 2)
        {
            SegmentPattern p;
            p.Name = "ModerateZigZag";
            p.MinDifficulty = 2;
            p.MaxDifficulty = 2;
            p.Weight = 1.0f;
            p.LocalEntryPoint = {0.0f, 0.0f, 0.0f};

            p.LocalPlatforms.push_back(PlatformBox{
                .Position = {0.00f, -0.40f, -0.00f},
                .Dimensions = {3.80f, 0.80f, 3.60f},
                .Color = {0.32f, 0.68f, 0.55f, 1.0f},
                .Metallic = 0.15f,
                .Roughness = 0.70f,
                .IsSolidObstacle = false,
                .Name = "ModZZ_Start"
            });

            p.LocalPlatforms.push_back(PlatformBox{
                .Position = {-2.12f, 0.00f, -5.23f},
                .Dimensions = {3.40f, 1.50f, 3.60f},
                .Color = {0.90f, 0.42f, 0.48f, 1.0f},
                .Metallic = 0.15f,
                .Roughness = 0.70f,
                .IsSolidObstacle = false,
                .Name = "ModZZ_Right1"
            });

            p.LocalPlatforms.push_back(PlatformBox{
                .Position = {-7.07f, -0.95f, -5.37f},
                .Dimensions = {3.00f, 0.80f, 3.20f},
                .Color = {0.92f, 0.70f, 0.22f, 1.0f},
                .Metallic = 0.15f,
                .Roughness = 0.70f,
                .IsSolidObstacle = false,
                .Name = "ModZZ_Left"
            });

            p.LocalPlatforms.push_back(PlatformBox{
                .Position = {-7.85f, -0.10f, -10.39f},
                .Dimensions = {3.20f, 1.30f, 3.40f},
                .Color = {0.28f, 0.65f, 0.48f, 1.0f},
                .Metallic = 0.15f,
                .Roughness = 0.70f,
                .IsSolidObstacle = false,
                .Name = "ModZZ_Right2"
            });

            p.LocalPlatforms.push_back(PlatformBox{
                .Position = {-13.01f, -0.40f, -13.01f},
                .Dimensions = {4.00f, 0.80f, 3.80f},
                .Color = {0.20f, 0.45f, 0.58f, 1.0f},
                .Metallic = 0.15f,
                .Roughness = 0.70f,
                .IsSolidObstacle = false,
                .Name = "ModZZ_End"
            });

            p.LocalExitPoint = {-17.96f, 0.00f, -17.96f};
            m_patterns.push_back(std::move(p));
        }

        // Pattern 5: AlternatingStepsDynamic (Difficulty 2)
        {
            SegmentPattern p;
            p.Name = "AlternatingStepsDynamic";
            p.MinDifficulty = 2;
            p.MaxDifficulty = 2;
            p.Weight = 1.0f;
            p.LocalEntryPoint = {0.0f, 0.0f, 0.0f};

            p.LocalPlatforms.push_back(PlatformBox{
                .Position = {0.00f, -0.40f, -0.00f},
                .Dimensions = {3.80f, 0.80f, 3.40f},
                .Color = {0.32f, 0.68f, 0.55f, 1.0f},
                .Metallic = 0.15f,
                .Roughness = 0.70f,
                .IsSolidObstacle = false,
                .Name = "AltStep_Start"
            });

            p.LocalPlatforms.push_back(PlatformBox{
                .Position = {-5.09f, -0.05f, -1.98f},
                .Dimensions = {3.20f, 1.50f, 3.40f},
                .Color = {0.92f, 0.70f, 0.22f, 1.0f},
                .Metallic = 0.15f,
                .Roughness = 0.70f,
                .IsSolidObstacle = false,
                .Name = "AltStep_Left"
            });

            p.LocalPlatforms.push_back(PlatformBox{
                .Position = {-4.74f, -1.00f, -7.00f},
                .Dimensions = {3.00f, 0.80f, 3.20f},
                .Color = {0.28f, 0.60f, 0.65f, 1.0f},
                .Metallic = 0.15f,
                .Roughness = 0.70f,
                .IsSolidObstacle = false,
                .Name = "AltStep_Right"
            });

            p.LocalPlatforms.push_back(PlatformBox{
                .Position = {-9.55f, -0.15f, -8.13f},
                .Dimensions = {3.20f, 1.30f, 3.20f},
                .Color = {0.35f, 0.40f, 0.65f, 1.0f},
                .Metallic = 0.15f,
                .Roughness = 0.70f,
                .IsSolidObstacle = false,
                .Name = "AltStep_Mid"
            });

            p.LocalPlatforms.push_back(PlatformBox{
                .Position = {-13.08f, -0.40f, -13.08f},
                .Dimensions = {4.00f, 0.80f, 3.80f},
                .Color = {0.28f, 0.65f, 0.48f, 1.0f},
                .Metallic = 0.15f,
                .Roughness = 0.70f,
                .IsSolidObstacle = false,
                .Name = "AltStep_End"
            });

            p.LocalExitPoint = {-17.89f, 0.00f, -17.89f};
            m_patterns.push_back(std::move(p));
        }

        // Pattern 6: RightDriftAscent (Difficulty 2)
        {
            SegmentPattern p;
            p.Name = "RightDriftAscent";
            p.MinDifficulty = 2;
            p.MaxDifficulty = 2;
            p.Weight = 1.0f;
            p.LocalEntryPoint = {0.0f, 0.0f, 0.0f};

            p.LocalPlatforms.push_back(PlatformBox{
                .Position = {0.00f, -0.40f, -0.00f},
                .Dimensions = {3.80f, 0.80f, 3.60f},
                .Color = {0.20f, 0.45f, 0.58f, 1.0f},
                .Metallic = 0.15f,
                .Roughness = 0.70f,
                .IsSolidObstacle = false,
                .Name = "DriftAsc_Start"
            });

            p.LocalPlatforms.push_back(PlatformBox{
                .Position = {-2.62f, -0.10f, -5.16f},
                .Dimensions = {3.40f, 1.40f, 3.60f},
                .Color = {0.35f, 0.40f, 0.65f, 1.0f},
                .Metallic = 0.15f,
                .Roughness = 0.70f,
                .IsSolidObstacle = false,
                .Name = "DriftAsc_Step1"
            });

            p.LocalPlatforms.push_back(PlatformBox{
                .Position = {-5.66f, 0.10f, -10.18f},
                .Dimensions = {3.20f, 1.80f, 3.40f},
                .Color = {0.45f, 0.38f, 0.75f, 1.0f},
                .Metallic = 0.15f,
                .Roughness = 0.70f,
                .IsSolidObstacle = false,
                .Name = "DriftAsc_Step2"
            });

            p.LocalPlatforms.push_back(PlatformBox{
                .Position = {-9.69f, -0.20f, -15.06f},
                .Dimensions = {4.00f, 1.10f, 3.60f},
                .Color = {0.30f, 0.80f, 0.55f, 1.0f},
                .Metallic = 0.15f,
                .Roughness = 0.70f,
                .IsSolidObstacle = false,
                .Name = "DriftAsc_End"
            });

            p.LocalExitPoint = {-14.28f, 0.35f, -19.94f};
            m_patterns.push_back(std::move(p));
        }

        // Pattern 7: OutwardReturnLoop (Difficulty 2)
        {
            SegmentPattern p;
            p.Name = "OutwardReturnLoop";
            p.MinDifficulty = 2;
            p.MaxDifficulty = 2;
            p.Weight = 1.0f;
            p.LocalEntryPoint = {0.0f, 0.0f, 0.0f};

            p.LocalPlatforms.push_back(PlatformBox{
                .Position = {0.00f, -0.40f, -0.00f},
                .Dimensions = {3.80f, 0.80f, 3.40f},
                .Color = {0.25f, 0.52f, 0.50f, 1.0f},
                .Metallic = 0.15f,
                .Roughness = 0.70f,
                .IsSolidObstacle = false,
                .Name = "Loop_Start"
            });

            p.LocalPlatforms.push_back(PlatformBox{
                .Position = {-5.23f, -0.05f, -2.12f},
                .Dimensions = {3.40f, 1.50f, 3.20f},
                .Color = {0.28f, 0.60f, 0.65f, 1.0f},
                .Metallic = 0.15f,
                .Roughness = 0.70f,
                .IsSolidObstacle = false,
                .Name = "Loop_Out1"
            });

            p.LocalPlatforms.push_back(PlatformBox{
                .Position = {-9.83f, -1.05f, -5.30f},
                .Dimensions = {3.00f, 0.80f, 3.40f},
                .Color = {0.32f, 0.68f, 0.55f, 1.0f},
                .Metallic = 0.15f,
                .Roughness = 0.70f,
                .IsSolidObstacle = false,
                .Name = "Loop_Out2"
            });

            p.LocalPlatforms.push_back(PlatformBox{
                .Position = {-12.37f, -0.15f, -10.39f},
                .Dimensions = {3.20f, 1.20f, 3.60f},
                .Color = {0.88f, 0.48f, 0.35f, 1.0f},
                .Metallic = 0.15f,
                .Roughness = 0.70f,
                .IsSolidObstacle = false,
                .Name = "Loop_In"
            });

            p.LocalPlatforms.push_back(PlatformBox{
                .Position = {-15.56f, -0.40f, -15.56f},
                .Dimensions = {4.20f, 0.80f, 3.80f},
                .Color = {0.28f, 0.65f, 0.48f, 1.0f},
                .Metallic = 0.15f,
                .Roughness = 0.70f,
                .IsSolidObstacle = false,
                .Name = "Loop_End"
            });

            p.LocalExitPoint = {-20.44f, 0.00f, -20.44f};
            m_patterns.push_back(std::move(p));
        }

        // Pattern 8: ElevatedPlateauSwerve (Difficulty 3)
        {
            SegmentPattern p;
            p.Name = "ElevatedPlateauSwerve";
            p.MinDifficulty = 3;
            p.MaxDifficulty = 3;
            p.Weight = 1.0f;
            p.LocalEntryPoint = {0.0f, 0.0f, 0.0f};

            p.LocalPlatforms.push_back(PlatformBox{
                .Position = {0.00f, -0.40f, -0.00f},
                .Dimensions = {3.40f, 0.80f, 3.20f},
                .Color = {0.20f, 0.45f, 0.58f, 1.0f},
                .Metallic = 0.15f,
                .Roughness = 0.70f,
                .IsSolidObstacle = false,
                .Name = "PlatSwerve_Start"
            });

            p.LocalPlatforms.push_back(PlatformBox{
                .Position = {-2.26f, 0.00f, -5.37f},
                .Dimensions = {4.40f, 1.60f, 3.80f},
                .Color = {0.88f, 0.48f, 0.35f, 1.0f},
                .Metallic = 0.15f,
                .Roughness = 0.70f,
                .IsSolidObstacle = false,
                .Name = "PlatSwerve_Terrace"
            });

            p.LocalPlatforms.push_back(PlatformBox{
                .Position = {-2.26f, 1.10f, -5.37f},
                .Dimensions = {0.60f, 0.60f, 3.00f},
                .Color = {0.15f, 0.15f, 0.18f, 1.0f},
                .Metallic = 0.90f,
                .Roughness = 0.20f,
                .IsSolidObstacle = true,
                .Name = "PlatSwerve_Hurdle"
            });

            p.LocalPlatforms.push_back(PlatformBox{
                .Position = {-7.71f, -1.00f, -7.71f},
                .Dimensions = {2.80f, 0.80f, 3.00f},
                .Color = {0.28f, 0.60f, 0.65f, 1.0f},
                .Metallic = 0.15f,
                .Roughness = 0.70f,
                .IsSolidObstacle = false,
                .Name = "PlatSwerve_Step"
            });

            p.LocalPlatforms.push_back(PlatformBox{
                .Position = {-12.23f, -1.80f, -9.69f},
                .Dimensions = {2.60f, 0.80f, 2.80f},
                .Color = {0.20f, 0.45f, 0.58f, 1.0f},
                .Metallic = 0.15f,
                .Roughness = 0.70f,
                .IsSolidObstacle = false,
                .Name = "PlatSwerve_Drop"
            });

            p.LocalPlatforms.push_back(PlatformBox{
                .Position = {-14.57f, -0.90f, -14.57f},
                .Dimensions = {3.80f, 1.00f, 3.40f},
                .Color = {0.30f, 0.80f, 0.55f, 1.0f},
                .Metallic = 0.15f,
                .Roughness = 0.70f,
                .IsSolidObstacle = false,
                .Name = "PlatSwerve_End"
            });

            p.LocalExitPoint = {-19.30f, -0.40f, -19.30f};
            m_patterns.push_back(std::move(p));
        }

        // Pattern 9: DynamicZigZagChallenging (Difficulty 3)
        {
            SegmentPattern p;
            p.Name = "DynamicZigZagChallenging";
            p.MinDifficulty = 3;
            p.MaxDifficulty = 3;
            p.Weight = 1.0f;
            p.LocalEntryPoint = {0.0f, 0.0f, 0.0f};

            p.LocalPlatforms.push_back(PlatformBox{
                .Position = {0.00f, -0.40f, -0.00f},
                .Dimensions = {3.40f, 0.80f, 3.20f},
                .Color = {0.32f, 0.68f, 0.55f, 1.0f},
                .Metallic = 0.15f,
                .Roughness = 0.70f,
                .IsSolidObstacle = false,
                .Name = "DynZZ_Start"
            });

            p.LocalPlatforms.push_back(PlatformBox{
                .Position = {-5.02f, 0.00f, -1.63f},
                .Dimensions = {2.80f, 1.50f, 3.00f},
                .Color = {0.90f, 0.42f, 0.48f, 1.0f},
                .Metallic = 0.15f,
                .Roughness = 0.70f,
                .IsSolidObstacle = false,
                .Name = "DynZZ_Left1"
            });

            p.LocalPlatforms.push_back(PlatformBox{
                .Position = {-3.61f, -0.85f, -6.43f},
                .Dimensions = {2.60f, 0.80f, 2.80f},
                .Color = {0.92f, 0.70f, 0.22f, 1.0f},
                .Metallic = 0.15f,
                .Roughness = 0.70f,
                .IsSolidObstacle = false,
                .Name = "DynZZ_Right"
            });

            p.LocalPlatforms.push_back(PlatformBox{
                .Position = {-8.27f, -0.05f, -5.73f},
                .Dimensions = {2.80f, 1.40f, 2.80f},
                .Color = {0.28f, 0.65f, 0.48f, 1.0f},
                .Metallic = 0.15f,
                .Roughness = 0.70f,
                .IsSolidObstacle = false,
                .Name = "DynZZ_Left2"
            });

            p.LocalPlatforms.push_back(PlatformBox{
                .Position = {-10.75f, -0.40f, -10.75f},
                .Dimensions = {4.00f, 0.80f, 3.60f},
                .Color = {0.25f, 0.52f, 0.50f, 1.0f},
                .Metallic = 0.15f,
                .Roughness = 0.70f,
                .IsSolidObstacle = false,
                .Name = "DynZZ_End"
            });

            p.LocalExitPoint = {-15.56f, 0.00f, -15.56f};
            m_patterns.push_back(std::move(p));
        }

        // Pattern 10: DescendingDropCurve (Difficulty 3)
        {
            SegmentPattern p;
            p.Name = "DescendingDropCurve";
            p.MinDifficulty = 3;
            p.MaxDifficulty = 3;
            p.Weight = 1.0f;
            p.LocalEntryPoint = {0.0f, 0.0f, 0.0f};

            p.LocalPlatforms.push_back(PlatformBox{
                .Position = {0.00f, -0.40f, -0.00f},
                .Dimensions = {3.60f, 0.80f, 3.20f},
                .Color = {0.25f, 0.52f, 0.50f, 1.0f},
                .Metallic = 0.15f,
                .Roughness = 0.70f,
                .IsSolidObstacle = false,
                .Name = "DropCurve_Launch"
            });

            p.LocalPlatforms.push_back(PlatformBox{
                .Position = {-1.98f, -2.20f, -5.09f},
                .Dimensions = {3.00f, 0.80f, 3.20f},
                .Color = {0.20f, 0.45f, 0.58f, 1.0f},
                .Metallic = 0.15f,
                .Roughness = 0.70f,
                .IsSolidObstacle = false,
                .Name = "DropCurve_Landing"
            });

            p.LocalPlatforms.push_back(PlatformBox{
                .Position = {-5.52f, -1.60f, -9.76f},
                .Dimensions = {2.80f, 1.40f, 3.00f},
                .Color = {0.28f, 0.60f, 0.65f, 1.0f},
                .Metallic = 0.15f,
                .Roughness = 0.70f,
                .IsSolidObstacle = false,
                .Name = "DropCurve_Climb"
            });

            p.LocalPlatforms.push_back(PlatformBox{
                .Position = {-10.18f, -0.80f, -12.16f},
                .Dimensions = {3.00f, 1.40f, 2.80f},
                .Color = {0.32f, 0.68f, 0.55f, 1.0f},
                .Metallic = 0.15f,
                .Roughness = 0.70f,
                .IsSolidObstacle = false,
                .Name = "DropCurve_Step"
            });

            p.LocalPlatforms.push_back(PlatformBox{
                .Position = {-15.41f, -0.15f, -15.41f},
                .Dimensions = {4.00f, 1.10f, 3.60f},
                .Color = {0.30f, 0.80f, 0.55f, 1.0f},
                .Metallic = 0.15f,
                .Roughness = 0.70f,
                .IsSolidObstacle = false,
                .Name = "DropCurve_End"
            });

            p.LocalExitPoint = {-20.22f, 0.40f, -20.22f};
            m_patterns.push_back(std::move(p));
        }

        // Pattern 11: UndulatingRidgeMeander (Difficulty 3)
        {
            SegmentPattern p;
            p.Name = "UndulatingRidgeMeander";
            p.MinDifficulty = 3;
            p.MaxDifficulty = 3;
            p.Weight = 1.0f;
            p.LocalEntryPoint = {0.0f, 0.0f, 0.0f};

            p.LocalPlatforms.push_back(PlatformBox{
                .Position = {0.00f, -0.40f, -0.00f},
                .Dimensions = {3.60f, 0.80f, 3.20f},
                .Color = {0.28f, 0.65f, 0.48f, 1.0f},
                .Metallic = 0.15f,
                .Roughness = 0.70f,
                .IsSolidObstacle = false,
                .Name = "Ridge_Start"
            });

            p.LocalPlatforms.push_back(PlatformBox{
                .Position = {-2.19f, 0.00f, -5.02f},
                .Dimensions = {3.00f, 1.60f, 3.20f},
                .Color = {0.88f, 0.48f, 0.35f, 1.0f},
                .Metallic = 0.15f,
                .Roughness = 0.70f,
                .IsSolidObstacle = false,
                .Name = "Ridge_Right1"
            });

            p.LocalPlatforms.push_back(PlatformBox{
                .Position = {-6.65f, -0.85f, -8.06f},
                .Dimensions = {2.60f, 0.80f, 2.80f},
                .Color = {0.92f, 0.70f, 0.22f, 1.0f},
                .Metallic = 0.15f,
                .Roughness = 0.70f,
                .IsSolidObstacle = false,
                .Name = "Ridge_Right2"
            });

            p.LocalPlatforms.push_back(PlatformBox{
                .Position = {-11.31f, -0.05f, -8.77f},
                .Dimensions = {2.80f, 1.50f, 3.00f},
                .Color = {0.45f, 0.38f, 0.75f, 1.0f},
                .Metallic = 0.15f,
                .Roughness = 0.70f,
                .IsSolidObstacle = false,
                .Name = "Ridge_Left"
            });

            p.LocalPlatforms.push_back(PlatformBox{
                .Position = {-13.86f, -0.40f, -13.86f},
                .Dimensions = {4.20f, 0.80f, 3.60f},
                .Color = {0.25f, 0.52f, 0.50f, 1.0f},
                .Metallic = 0.15f,
                .Roughness = 0.70f,
                .IsSolidObstacle = false,
                .Name = "Ridge_End"
            });

            p.LocalExitPoint = {-18.67f, 0.00f, -18.67f};
            m_patterns.push_back(std::move(p));
        }

        // Pattern 12: ZigZagAscent (Difficulty 4)
        {
            SegmentPattern p;
            p.Name = "ZigZagAscent";
            p.MinDifficulty = 4;
            p.MaxDifficulty = 4;
            p.Weight = 1.0f;
            p.LocalEntryPoint = {0.0f, 0.0f, 0.0f};

            p.LocalPlatforms.push_back(PlatformBox{
                .Position = {0.00f, -0.40f, -0.00f},
                .Dimensions = {2.80f, 0.80f, 2.40f},
                .Color = {0.20f, 0.45f, 0.58f, 1.0f},
                .Metallic = 0.15f,
                .Roughness = 0.70f,
                .IsSolidObstacle = false,
                .Name = "ZZAsc_Start"
            });

            p.LocalPlatforms.push_back(PlatformBox{
                .Position = {-1.41f, 0.05f, -4.53f},
                .Dimensions = {2.20f, 1.60f, 2.40f},
                .Color = {0.90f, 0.42f, 0.48f, 1.0f},
                .Metallic = 0.15f,
                .Roughness = 0.70f,
                .IsSolidObstacle = false,
                .Name = "ZZAsc_Right1"
            });

            p.LocalPlatforms.push_back(PlatformBox{
                .Position = {-4.60f, -0.30f, -8.56f},
                .Dimensions = {2.20f, 1.10f, 2.20f},
                .Color = {0.45f, 0.38f, 0.75f, 1.0f},
                .Metallic = 0.15f,
                .Roughness = 0.70f,
                .IsSolidObstacle = false,
                .Name = "ZZAsc_Right2"
            });

            p.LocalPlatforms.push_back(PlatformBox{
                .Position = {-8.84f, 0.25f, -6.86f},
                .Dimensions = {2.20f, 1.80f, 2.00f},
                .Color = {0.95f, 0.75f, 0.25f, 1.0f},
                .Metallic = 0.15f,
                .Roughness = 0.70f,
                .IsSolidObstacle = false,
                .Name = "ZZAsc_Left"
            });

            p.LocalPlatforms.push_back(PlatformBox{
                .Position = {-11.46f, -0.15f, -11.46f},
                .Dimensions = {3.80f, 1.10f, 3.40f},
                .Color = {0.25f, 0.52f, 0.50f, 1.0f},
                .Metallic = 0.15f,
                .Roughness = 0.70f,
                .IsSolidObstacle = false,
                .Name = "ZZAsc_End"
            });

            p.LocalExitPoint = {-15.91f, 0.40f, -15.91f};
            m_patterns.push_back(std::move(p));
        }

        // Pattern 13: HurdleRunwaySwerve (Difficulty 4)
        {
            SegmentPattern p;
            p.Name = "HurdleRunwaySwerve";
            p.MinDifficulty = 4;
            p.MaxDifficulty = 4;
            p.Weight = 1.0f;
            p.LocalEntryPoint = {0.0f, 0.0f, 0.0f};

            p.LocalPlatforms.push_back(PlatformBox{
                .Position = {0.00f, -0.40f, -0.00f},
                .Dimensions = {2.80f, 0.80f, 2.60f},
                .Color = {0.32f, 0.68f, 0.55f, 1.0f},
                .Metallic = 0.15f,
                .Roughness = 0.70f,
                .IsSolidObstacle = false,
                .Name = "Runway_Start"
            });

            p.LocalPlatforms.push_back(PlatformBox{
                .Position = {-5.44f, 0.10f, -2.33f},
                .Dimensions = {4.00f, 1.60f, 2.40f},
                .Color = {0.88f, 0.48f, 0.35f, 1.0f},
                .Metallic = 0.15f,
                .Roughness = 0.70f,
                .IsSolidObstacle = false,
                .Name = "Runway_Terrace"
            });

            p.LocalPlatforms.push_back(PlatformBox{
                .Position = {-5.44f, 1.20f, -2.33f},
                .Dimensions = {3.60f, 0.60f, 0.50f},
                .Color = {0.15f, 0.15f, 0.18f, 1.0f},
                .Metallic = 0.90f,
                .Roughness = 0.20f,
                .IsSolidObstacle = true,
                .Name = "Runway_Hurdle"
            });

            p.LocalPlatforms.push_back(PlatformBox{
                .Position = {-4.31f, -2.00f, -6.86f},
                .Dimensions = {2.20f, 0.80f, 2.40f},
                .Color = {0.20f, 0.45f, 0.58f, 1.0f},
                .Metallic = 0.15f,
                .Roughness = 0.70f,
                .IsSolidObstacle = false,
                .Name = "Runway_Drop"
            });

            p.LocalPlatforms.push_back(PlatformBox{
                .Position = {-9.19f, -1.10f, -9.19f},
                .Dimensions = {3.60f, 1.00f, 3.20f},
                .Color = {0.30f, 0.80f, 0.55f, 1.0f},
                .Metallic = 0.15f,
                .Roughness = 0.70f,
                .IsSolidObstacle = false,
                .Name = "Runway_End"
            });

            p.LocalExitPoint = {-13.65f, -0.60f, -13.65f};
            m_patterns.push_back(std::move(p));
        }

        // Pattern 14: DropAndLateralAscent (Difficulty 4)
        {
            SegmentPattern p;
            p.Name = "DropAndLateralAscent";
            p.MinDifficulty = 4;
            p.MaxDifficulty = 4;
            p.Weight = 1.0f;
            p.LocalEntryPoint = {0.0f, 0.0f, 0.0f};

            p.LocalPlatforms.push_back(PlatformBox{
                .Position = {0.00f, -0.40f, -0.00f},
                .Dimensions = {2.80f, 0.80f, 2.40f},
                .Color = {0.25f, 0.52f, 0.50f, 1.0f},
                .Metallic = 0.15f,
                .Roughness = 0.70f,
                .IsSolidObstacle = false,
                .Name = "DropAsc_Start"
            });

            p.LocalPlatforms.push_back(PlatformBox{
                .Position = {-1.48f, -2.20f, -4.60f},
                .Dimensions = {2.40f, 0.80f, 2.60f},
                .Color = {0.20f, 0.45f, 0.58f, 1.0f},
                .Metallic = 0.15f,
                .Roughness = 0.70f,
                .IsSolidObstacle = false,
                .Name = "DropAsc_Landing"
            });

            p.LocalPlatforms.push_back(PlatformBox{
                .Position = {-5.94f, -1.55f, -3.68f},
                .Dimensions = {2.20f, 1.40f, 2.20f},
                .Color = {0.28f, 0.60f, 0.65f, 1.0f},
                .Metallic = 0.15f,
                .Roughness = 0.70f,
                .IsSolidObstacle = false,
                .Name = "DropAsc_Climb1"
            });

            p.LocalPlatforms.push_back(PlatformBox{
                .Position = {-5.87f, -0.60f, -8.13f},
                .Dimensions = {2.20f, 1.40f, 2.40f},
                .Color = {0.92f, 0.70f, 0.22f, 1.0f},
                .Metallic = 0.15f,
                .Roughness = 0.70f,
                .IsSolidObstacle = false,
                .Name = "DropAsc_Climb2"
            });

            p.LocalPlatforms.push_back(PlatformBox{
                .Position = {-10.82f, 0.05f, -10.82f},
                .Dimensions = {3.80f, 1.10f, 3.20f},
                .Color = {0.30f, 0.80f, 0.55f, 1.0f},
                .Metallic = 0.15f,
                .Roughness = 0.70f,
                .IsSolidObstacle = false,
                .Name = "DropAsc_End"
            });

            p.LocalExitPoint = {-15.20f, 0.60f, -15.20f};
            m_patterns.push_back(std::move(p));
        }

        // Pattern 15: ApexPillars (Difficulty 5)
        {
            SegmentPattern p;
            p.Name = "ApexPillars";
            p.MinDifficulty = 5;
            p.MaxDifficulty = 5;
            p.Weight = 1.0f;
            p.LocalEntryPoint = {0.0f, 0.0f, 0.0f};

            p.LocalPlatforms.push_back(PlatformBox{
                .Position = {0.00f, -0.40f, -0.00f},
                .Dimensions = {2.40f, 0.80f, 2.00f},
                .Color = {0.25f, 0.52f, 0.50f, 1.0f},
                .Metallic = 0.15f,
                .Roughness = 0.70f,
                .IsSolidObstacle = false,
                .Name = "Apex_Start"
            });

            p.LocalPlatforms.push_back(PlatformBox{
                .Position = {-1.27f, 0.05f, -4.10f},
                .Dimensions = {1.50f, 1.60f, 1.60f},
                .Color = {0.95f, 0.75f, 0.25f, 1.0f},
                .Metallic = 0.15f,
                .Roughness = 0.70f,
                .IsSolidObstacle = false,
                .Name = "Apex_Right1"
            });

            p.LocalPlatforms.push_back(PlatformBox{
                .Position = {-5.02f, -0.50f, -2.47f},
                .Dimensions = {1.50f, 0.80f, 1.50f},
                .Color = {0.45f, 0.38f, 0.75f, 1.0f},
                .Metallic = 0.15f,
                .Roughness = 0.70f,
                .IsSolidObstacle = false,
                .Name = "Apex_Left"
            });

            p.LocalPlatforms.push_back(PlatformBox{
                .Position = {-3.60f, -0.05f, -5.90f},
                .Dimensions = {1.60f, 1.60f, 1.60f},
                .Color = {0.90f, 0.42f, 0.48f, 1.0f},
                .Metallic = 0.15f,
                .Roughness = 0.70f,
                .IsSolidObstacle = false,
                .Name = "Apex_Right2"
            });

            p.LocalPlatforms.push_back(PlatformBox{
                .Position = {-8.06f, -0.50f, -8.06f},
                .Dimensions = {3.40f, 1.00f, 3.20f},
                .Color = {0.25f, 0.52f, 0.50f, 1.0f},
                .Metallic = 0.15f,
                .Roughness = 0.70f,
                .IsSolidObstacle = false,
                .Name = "Apex_End"
            });

            p.LocalExitPoint = {-12.37f, 0.00f, -12.37f};
            m_patterns.push_back(std::move(p));
        }

        // Pattern 16: CitadelGauntlet (Difficulty 5)
        {
            SegmentPattern p;
            p.Name = "CitadelGauntlet";
            p.MinDifficulty = 5;
            p.MaxDifficulty = 5;
            p.Weight = 1.0f;
            p.LocalEntryPoint = {0.0f, 0.0f, 0.0f};

            p.LocalPlatforms.push_back(PlatformBox{
                .Position = {0.00f, -0.40f, -0.00f},
                .Dimensions = {2.40f, 0.80f, 2.20f},
                .Color = {0.20f, 0.45f, 0.58f, 1.0f},
                .Metallic = 0.15f,
                .Roughness = 0.70f,
                .IsSolidObstacle = false,
                .Name = "Citadel_Start"
            });

            p.LocalPlatforms.push_back(PlatformBox{
                .Position = {-5.16f, 0.25f, -2.05f},
                .Dimensions = {3.40f, 1.80f, 3.00f},
                .Color = {0.88f, 0.48f, 0.35f, 1.0f},
                .Metallic = 0.15f,
                .Roughness = 0.70f,
                .IsSolidObstacle = false,
                .Name = "Citadel_Terrace"
            });

            p.LocalPlatforms.push_back(PlatformBox{
                .Position = {-5.16f, 1.45f, -2.05f},
                .Dimensions = {3.00f, 0.60f, 0.50f},
                .Color = {0.15f, 0.15f, 0.18f, 1.0f},
                .Metallic = 0.90f,
                .Roughness = 0.20f,
                .IsSolidObstacle = true,
                .Name = "Citadel_Hurdle"
            });

            p.LocalPlatforms.push_back(PlatformBox{
                .Position = {-4.10f, -1.90f, -6.65f},
                .Dimensions = {1.60f, 0.80f, 1.60f},
                .Color = {0.45f, 0.38f, 0.75f, 1.0f},
                .Metallic = 0.15f,
                .Roughness = 0.70f,
                .IsSolidObstacle = false,
                .Name = "Citadel_DropPillar"
            });

            p.LocalPlatforms.push_back(PlatformBox{
                .Position = {-8.77f, -0.90f, -8.77f},
                .Dimensions = {3.40f, 1.00f, 3.20f},
                .Color = {0.30f, 0.80f, 0.55f, 1.0f},
                .Metallic = 0.15f,
                .Roughness = 0.70f,
                .IsSolidObstacle = false,
                .Name = "Citadel_End"
            });

            p.LocalExitPoint = {-13.15f, -0.40f, -13.15f};
            m_patterns.push_back(std::move(p));
        }

        // Pattern 17: PrecisionPinnacles (Difficulty 5)
        {
            SegmentPattern p;
            p.Name = "PrecisionPinnacles";
            p.MinDifficulty = 5;
            p.MaxDifficulty = 5;
            p.Weight = 1.0f;
            p.LocalEntryPoint = {0.0f, 0.0f, 0.0f};

            p.LocalPlatforms.push_back(PlatformBox{
                .Position = {0.00f, -0.40f, -0.00f},
                .Dimensions = {2.40f, 0.80f, 2.20f},
                .Color = {0.32f, 0.68f, 0.55f, 1.0f},
                .Metallic = 0.15f,
                .Roughness = 0.70f,
                .IsSolidObstacle = false,
                .Name = "Pinnacle_Start"
            });

            p.LocalPlatforms.push_back(PlatformBox{
                .Position = {-4.24f, 0.10f, -1.41f},
                .Dimensions = {1.60f, 1.60f, 1.50f},
                .Color = {0.95f, 0.75f, 0.25f, 1.0f},
                .Metallic = 0.15f,
                .Roughness = 0.70f,
                .IsSolidObstacle = false,
                .Name = "Pinnacle_1"
            });

            p.LocalPlatforms.push_back(PlatformBox{
                .Position = {-2.47f, -0.60f, -5.30f},
                .Dimensions = {1.50f, 0.80f, 1.60f},
                .Color = {0.92f, 0.70f, 0.22f, 1.0f},
                .Metallic = 0.15f,
                .Roughness = 0.70f,
                .IsSolidObstacle = false,
                .Name = "Pinnacle_2"
            });

            p.LocalPlatforms.push_back(PlatformBox{
                .Position = {-6.22f, 0.05f, -3.96f},
                .Dimensions = {1.50f, 1.50f, 1.50f},
                .Color = {0.90f, 0.42f, 0.48f, 1.0f},
                .Metallic = 0.15f,
                .Roughness = 0.70f,
                .IsSolidObstacle = false,
                .Name = "Pinnacle_3"
            });

            p.LocalPlatforms.push_back(PlatformBox{
                .Position = {-8.49f, -0.50f, -8.49f},
                .Dimensions = {3.40f, 1.00f, 3.20f},
                .Color = {0.25f, 0.52f, 0.50f, 1.0f},
                .Metallic = 0.15f,
                .Roughness = 0.70f,
                .IsSolidObstacle = false,
                .Name = "Pinnacle_End"
            });

            p.LocalExitPoint = {-12.87f, 0.00f, -12.87f};
            m_patterns.push_back(std::move(p));
        }

        // =========================================================================
        // LEVEL 6: Simple Moving Platforms (Difficulty 6)
        // =========================================================================

        // Pattern 18: MovingPlatformLateral (Difficulty 6)
        {
            SegmentPattern p;
            p.Name = "MovingPlatformLateral";
            p.MinDifficulty = 6;
            p.MaxDifficulty = 6;
            p.Weight = 1.0f;
            p.LocalEntryPoint = {0.0f, 0.0f, 0.0f};

            p.LocalPlatforms.push_back(PlatformBox{
                .Position = {0.00f, -0.40f, 0.00f},
                .Dimensions = {4.00f, 0.80f, 4.00f},
                .Color = {0.25f, 0.52f, 0.50f, 1.0f},
                .Metallic = 0.15f,
                .Roughness = 0.70f,
                .IsSolidObstacle = false,
                .Name = "MoveLat_Start"
            });

            p.LocalPlatforms.push_back(PlatformBox{
                .Position = {-3.50f, -0.40f, -4.50f},
                .Dimensions = {2.60f, 0.80f, 2.60f},
                .Color = {0.18f, 0.42f, 0.65f, 1.0f},
                .Metallic = 0.20f,
                .Roughness = 0.65f,
                .IsSolidObstacle = false,
                .Name = "MoveLat_Platform",
                .IsMoving = true,
                .MoveAxis = {1.0f, 0.0f, 0.0f},
                .MoveDistance = 1.00f,
                .MoveSpeed = 1.80f,
                .MovePhase = 0.0f,
                .BasePosition = {-3.50f, -0.40f, -4.50f}
            });

            p.LocalPlatforms.push_back(PlatformBox{
                .Position = {-7.00f, -0.40f, -9.00f},
                .Dimensions = {4.00f, 0.80f, 4.00f},
                .Color = {0.28f, 0.65f, 0.48f, 1.0f},
                .Metallic = 0.15f,
                .Roughness = 0.70f,
                .IsSolidObstacle = false,
                .Name = "MoveLat_Exit"
            });

            p.LocalExitPoint = {-7.00f, 0.00f, -9.00f};
            m_patterns.push_back(std::move(p));
        }

        // Pattern 19: MovingPlatformLongitudinal (Difficulty 6)
        {
            SegmentPattern p;
            p.Name = "MovingPlatformLongitudinal";
            p.MinDifficulty = 6;
            p.MaxDifficulty = 6;
            p.Weight = 1.0f;
            p.LocalEntryPoint = {0.0f, 0.0f, 0.0f};

            p.LocalPlatforms.push_back(PlatformBox{
                .Position = {0.00f, -0.40f, 0.00f},
                .Dimensions = {4.00f, 0.80f, 4.00f},
                .Color = {0.25f, 0.52f, 0.50f, 1.0f},
                .Metallic = 0.15f,
                .Roughness = 0.70f,
                .IsSolidObstacle = false,
                .Name = "MoveLong_Start"
            });

            p.LocalPlatforms.push_back(PlatformBox{
                .Position = {-4.50f, -0.20f, -3.50f},
                .Dimensions = {2.60f, 0.80f, 2.60f},
                .Color = {0.18f, 0.42f, 0.65f, 1.0f},
                .Metallic = 0.20f,
                .Roughness = 0.65f,
                .IsSolidObstacle = false,
                .Name = "MoveLong_Platform",
                .IsMoving = true,
                .MoveAxis = {0.0f, 0.0f, 1.0f},
                .MoveDistance = 1.00f,
                .MoveSpeed = 2.00f,
                .MovePhase = 0.0f,
                .BasePosition = {-4.50f, -0.20f, -3.50f}
            });

            p.LocalPlatforms.push_back(PlatformBox{
                .Position = {-9.00f, 0.00f, -7.00f},
                .Dimensions = {4.00f, 0.80f, 4.00f},
                .Color = {0.28f, 0.65f, 0.48f, 1.0f},
                .Metallic = 0.15f,
                .Roughness = 0.70f,
                .IsSolidObstacle = false,
                .Name = "MoveLong_Exit"
            });

            p.LocalExitPoint = {-9.00f, 0.40f, -7.00f};
            m_patterns.push_back(std::move(p));
        }

        // =========================================================================
        // LEVEL 7: Complex Moving Platforms (Difficulty 7)
        // =========================================================================

        // Pattern 20: DualMovingCrossway (Difficulty 7)
        {
            SegmentPattern p;
            p.Name = "DualMovingCrossway";
            p.MinDifficulty = 7;
            p.MaxDifficulty = 7;
            p.Weight = 1.0f;
            p.LocalEntryPoint = {0.0f, 0.0f, 0.0f};

            p.LocalPlatforms.push_back(PlatformBox{
                .Position = {0.00f, -0.40f, 0.00f},
                .Dimensions = {3.80f, 0.80f, 3.80f},
                .Color = {0.25f, 0.52f, 0.50f, 1.0f},
                .Metallic = 0.15f,
                .Roughness = 0.70f,
                .IsSolidObstacle = false,
                .Name = "DualMove_Start"
            });

            p.LocalPlatforms.push_back(PlatformBox{
                .Position = {-3.50f, -0.20f, -4.20f},
                .Dimensions = {2.40f, 0.80f, 2.40f},
                .Color = {0.18f, 0.42f, 0.65f, 1.0f},
                .Metallic = 0.20f,
                .Roughness = 0.65f,
                .IsSolidObstacle = false,
                .Name = "DualMove_PlatX",
                .IsMoving = true,
                .MoveAxis = {1.0f, 0.0f, 0.0f},
                .MoveDistance = 1.10f,
                .MoveSpeed = 2.20f,
                .MovePhase = 0.0f,
                .BasePosition = {-3.50f, -0.20f, -4.20f}
            });

            p.LocalPlatforms.push_back(PlatformBox{
                .Position = {-7.00f, 0.00f, -8.40f},
                .Dimensions = {2.40f, 0.80f, 2.40f},
                .Color = {0.20f, 0.48f, 0.70f, 1.0f},
                .Metallic = 0.20f,
                .Roughness = 0.65f,
                .IsSolidObstacle = false,
                .Name = "DualMove_PlatZ",
                .IsMoving = true,
                .MoveAxis = {0.0f, 0.0f, 1.0f},
                .MoveDistance = 1.10f,
                .MoveSpeed = 2.40f,
                .MovePhase = 1.57f,
                .BasePosition = {-7.00f, 0.00f, -8.40f}
            });

            p.LocalPlatforms.push_back(PlatformBox{
                .Position = {-10.50f, 0.20f, -12.60f},
                .Dimensions = {3.80f, 0.80f, 3.80f},
                .Color = {0.28f, 0.65f, 0.48f, 1.0f},
                .Metallic = 0.15f,
                .Roughness = 0.70f,
                .IsSolidObstacle = false,
                .Name = "DualMove_Exit"
            });

            p.LocalExitPoint = {-10.50f, 0.60f, -12.60f};
            m_patterns.push_back(std::move(p));
        }

        // Pattern 21: AscendingMovingSteps (Difficulty 7)
        {
            SegmentPattern p;
            p.Name = "AscendingMovingSteps";
            p.MinDifficulty = 7;
            p.MaxDifficulty = 7;
            p.Weight = 1.0f;
            p.LocalEntryPoint = {0.0f, 0.0f, 0.0f};

            p.LocalPlatforms.push_back(PlatformBox{
                .Position = {0.00f, -0.40f, 0.00f},
                .Dimensions = {3.80f, 0.80f, 3.80f},
                .Color = {0.25f, 0.52f, 0.50f, 1.0f},
                .Metallic = 0.15f,
                .Roughness = 0.70f,
                .IsSolidObstacle = false,
                .Name = "AscStep_Start"
            });

            p.LocalPlatforms.push_back(PlatformBox{
                .Position = {-3.40f, 0.10f, -4.00f},
                .Dimensions = {2.40f, 0.80f, 2.40f},
                .Color = {0.18f, 0.42f, 0.65f, 1.0f},
                .Metallic = 0.20f,
                .Roughness = 0.65f,
                .IsSolidObstacle = false,
                .Name = "AscStep_MoveA",
                .IsMoving = true,
                .MoveAxis = {1.0f, 0.0f, 0.0f},
                .MoveDistance = 1.00f,
                .MoveSpeed = 2.60f,
                .MovePhase = 0.0f,
                .BasePosition = {-3.40f, 0.10f, -4.00f}
            });

            p.LocalPlatforms.push_back(PlatformBox{
                .Position = {-6.80f, 0.50f, -8.00f},
                .Dimensions = {2.40f, 0.80f, 2.40f},
                .Color = {0.20f, 0.48f, 0.70f, 1.0f},
                .Metallic = 0.20f,
                .Roughness = 0.65f,
                .IsSolidObstacle = false,
                .Name = "AscStep_MoveB",
                .IsMoving = true,
                .MoveAxis = {1.0f, 0.0f, 0.0f},
                .MoveDistance = 1.00f,
                .MoveSpeed = 2.60f,
                .MovePhase = 3.14f,
                .BasePosition = {-6.80f, 0.50f, -8.00f}
            });

            p.LocalPlatforms.push_back(PlatformBox{
                .Position = {-10.20f, 0.80f, -12.00f},
                .Dimensions = {3.80f, 0.80f, 3.80f},
                .Color = {0.28f, 0.65f, 0.48f, 1.0f},
                .Metallic = 0.15f,
                .Roughness = 0.70f,
                .IsSolidObstacle = false,
                .Name = "AscStep_Exit"
            });

            p.LocalExitPoint = {-10.20f, 1.20f, -12.00f};
            m_patterns.push_back(std::move(p));
        }

        // =========================================================================
        // LEVEL 8: Dynamic Obstacles & Hazards (Difficulty 8)
        // =========================================================================

        // Pattern 22: PeriodicSpikeRunway (Difficulty 8)
        {
            SegmentPattern p;
            p.Name = "PeriodicSpikeRunway";
            p.MinDifficulty = 8;
            p.MaxDifficulty = 8;
            p.Weight = 1.0f;
            p.LocalEntryPoint = {0.0f, 0.0f, 0.0f};

            p.LocalPlatforms.push_back(PlatformBox{
                .Position = {0.00f, -0.40f, 0.00f},
                .Dimensions = {3.80f, 0.80f, 3.80f},
                .Color = {0.25f, 0.52f, 0.50f, 1.0f},
                .Metallic = 0.15f,
                .Roughness = 0.70f,
                .IsSolidObstacle = false,
                .Name = "SpikeRun_Start"
            });

            p.LocalPlatforms.push_back(PlatformBox{
                .Position = {-3.80f, -0.40f, -4.80f},
                .Dimensions = {3.80f, 0.80f, 4.80f},
                .Color = {0.22f, 0.45f, 0.52f, 1.0f},
                .Metallic = 0.15f,
                .Roughness = 0.70f,
                .IsSolidObstacle = false,
                .Name = "SpikeRun_Runway"
            });

            p.LocalPlatforms.push_back(PlatformBox{
                .Position = {-3.80f, 0.40f, -4.80f},
                .Dimensions = {1.80f, 0.80f, 1.40f},
                .Color = {0.80f, 0.20f, 0.20f, 1.0f},
                .Metallic = 0.30f,
                .Roughness = 0.50f,
                .IsSolidObstacle = true,
                .Name = "SpikeRun_Hazard",
                .BasePosition = {-3.80f, 0.40f, -4.80f},
                .IsPeriodicHazard = true,
                .HazardPeriod = 2.40f,
                .HazardActiveTime = 1.20f,
                .HazardTimer = 0.0f,
                .HazardIsActive = true
            });

            p.LocalPlatforms.push_back(PlatformBox{
                .Position = {-7.60f, -0.40f, -9.60f},
                .Dimensions = {3.80f, 0.80f, 3.80f},
                .Color = {0.28f, 0.65f, 0.48f, 1.0f},
                .Metallic = 0.15f,
                .Roughness = 0.70f,
                .IsSolidObstacle = false,
                .Name = "SpikeRun_Exit"
            });

            p.LocalExitPoint = {-7.60f, 0.00f, -9.60f};
            m_patterns.push_back(std::move(p));
        }

        // Pattern 23: PatrollingHazardLedge (Difficulty 8)
        {
            SegmentPattern p;
            p.Name = "PatrollingHazardLedge";
            p.MinDifficulty = 8;
            p.MaxDifficulty = 8;
            p.Weight = 1.0f;
            p.LocalEntryPoint = {0.0f, 0.0f, 0.0f};

            p.LocalPlatforms.push_back(PlatformBox{
                .Position = {0.00f, -0.40f, 0.00f},
                .Dimensions = {3.80f, 0.80f, 3.80f},
                .Color = {0.25f, 0.52f, 0.50f, 1.0f},
                .Metallic = 0.15f,
                .Roughness = 0.70f,
                .IsSolidObstacle = false,
                .Name = "Patrol_Start"
            });

            p.LocalPlatforms.push_back(PlatformBox{
                .Position = {-3.80f, -0.20f, -4.80f},
                .Dimensions = {4.00f, 0.80f, 4.00f},
                .Color = {0.20f, 0.45f, 0.58f, 1.0f},
                .Metallic = 0.15f,
                .Roughness = 0.70f,
                .IsSolidObstacle = false,
                .Name = "Patrol_Platform"
            });

            p.LocalPlatforms.push_back(PlatformBox{
                .Position = {-3.80f, 0.60f, -4.80f},
                .Dimensions = {1.00f, 0.80f, 1.00f},
                .Color = {0.85f, 0.25f, 0.20f, 1.0f},
                .Metallic = 0.35f,
                .Roughness = 0.50f,
                .IsSolidObstacle = true,
                .Name = "Patrol_Obstacle",
                .IsMoving = true,
                .MoveAxis = {1.0f, 0.0f, 0.0f},
                .MoveDistance = 1.10f,
                .MoveSpeed = 2.20f,
                .MovePhase = 0.0f,
                .BasePosition = {-3.80f, 0.60f, -4.80f}
            });

            p.LocalPlatforms.push_back(PlatformBox{
                .Position = {-7.60f, 0.00f, -9.60f},
                .Dimensions = {3.80f, 0.80f, 3.80f},
                .Color = {0.28f, 0.65f, 0.48f, 1.0f},
                .Metallic = 0.15f,
                .Roughness = 0.70f,
                .IsSolidObstacle = false,
                .Name = "Patrol_Exit"
            });

            p.LocalExitPoint = {-7.60f, 0.40f, -9.60f};
            m_patterns.push_back(std::move(p));
        }

        // =========================================================================
        // LEVEL 9: Combined Moving Platforms & Hazards (Difficulty 9)
        // =========================================================================

        // Pattern 24: MovingPlatformHazardWeave (Difficulty 9)
        {
            SegmentPattern p;
            p.Name = "MovingPlatformHazardWeave";
            p.MinDifficulty = 9;
            p.MaxDifficulty = 9;
            p.Weight = 1.0f;
            p.LocalEntryPoint = {0.0f, 0.0f, 0.0f};

            p.LocalPlatforms.push_back(PlatformBox{
                .Position = {0.00f, -0.40f, 0.00f},
                .Dimensions = {3.60f, 0.80f, 3.60f},
                .Color = {0.25f, 0.52f, 0.50f, 1.0f},
                .Metallic = 0.15f,
                .Roughness = 0.70f,
                .IsSolidObstacle = false,
                .Name = "Weave_Start"
            });

            p.LocalPlatforms.push_back(PlatformBox{
                .Position = {-3.50f, 0.00f, -4.20f},
                .Dimensions = {2.40f, 0.80f, 2.40f},
                .Color = {0.18f, 0.42f, 0.65f, 1.0f},
                .Metallic = 0.20f,
                .Roughness = 0.65f,
                .IsSolidObstacle = false,
                .Name = "Weave_MovePlat",
                .IsMoving = true,
                .MoveAxis = {1.0f, 0.0f, 0.0f},
                .MoveDistance = 1.10f,
                .MoveSpeed = 2.20f,
                .MovePhase = 0.0f,
                .BasePosition = {-3.50f, 0.00f, -4.20f}
            });

            p.LocalPlatforms.push_back(PlatformBox{
                .Position = {-7.00f, 0.20f, -8.60f},
                .Dimensions = {3.80f, 0.80f, 3.80f},
                .Color = {0.22f, 0.45f, 0.52f, 1.0f},
                .Metallic = 0.15f,
                .Roughness = 0.70f,
                .IsSolidObstacle = false,
                .Name = "Weave_MidPlat"
            });

            p.LocalPlatforms.push_back(PlatformBox{
                .Position = {-7.00f, 1.10f, -8.60f},
                .Dimensions = {1.80f, 0.80f, 1.20f},
                .Color = {0.80f, 0.20f, 0.20f, 1.0f},
                .Metallic = 0.30f,
                .Roughness = 0.50f,
                .IsSolidObstacle = true,
                .Name = "Weave_Hazard",
                .BasePosition = {-7.00f, 1.10f, -8.60f},
                .IsPeriodicHazard = true,
                .HazardPeriod = 2.20f,
                .HazardActiveTime = 1.00f,
                .HazardTimer = 0.0f,
                .HazardIsActive = true
            });

            p.LocalPlatforms.push_back(PlatformBox{
                .Position = {-10.60f, 0.40f, -13.00f},
                .Dimensions = {3.80f, 0.80f, 3.80f},
                .Color = {0.28f, 0.65f, 0.48f, 1.0f},
                .Metallic = 0.15f,
                .Roughness = 0.70f,
                .IsSolidObstacle = false,
                .Name = "Weave_Exit"
            });

            p.LocalExitPoint = {-10.60f, 0.80f, -13.00f};
            m_patterns.push_back(std::move(p));
        }

        // Pattern 25: OscillatingSpikeGauntlet (Difficulty 9)
        {
            SegmentPattern p;
            p.Name = "OscillatingSpikeGauntlet";
            p.MinDifficulty = 9;
            p.MaxDifficulty = 9;
            p.Weight = 1.0f;
            p.LocalEntryPoint = {0.0f, 0.0f, 0.0f};

            p.LocalPlatforms.push_back(PlatformBox{
                .Position = {0.00f, -0.40f, 0.00f},
                .Dimensions = {3.60f, 0.80f, 3.60f},
                .Color = {0.25f, 0.52f, 0.50f, 1.0f},
                .Metallic = 0.15f,
                .Roughness = 0.70f,
                .IsSolidObstacle = false,
                .Name = "Gaunt_Start"
            });

            p.LocalPlatforms.push_back(PlatformBox{
                .Position = {-3.60f, -0.20f, -4.40f},
                .Dimensions = {2.50f, 0.80f, 2.50f},
                .Color = {0.18f, 0.42f, 0.65f, 1.0f},
                .Metallic = 0.20f,
                .Roughness = 0.65f,
                .IsSolidObstacle = false,
                .Name = "Gaunt_MovePlat",
                .IsMoving = true,
                .MoveAxis = {0.0f, 0.0f, 1.0f},
                .MoveDistance = 1.00f,
                .MoveSpeed = 2.40f,
                .MovePhase = 0.0f,
                .BasePosition = {-3.60f, -0.20f, -4.40f}
            });

            p.LocalPlatforms.push_back(PlatformBox{
                .Position = {-7.40f, 0.00f, -9.00f},
                .Dimensions = {4.00f, 0.80f, 4.00f},
                .Color = {0.20f, 0.45f, 0.58f, 1.0f},
                .Metallic = 0.15f,
                .Roughness = 0.70f,
                .IsSolidObstacle = false,
                .Name = "Gaunt_Platform2"
            });

            p.LocalPlatforms.push_back(PlatformBox{
                .Position = {-7.40f, 0.80f, -9.00f},
                .Dimensions = {1.00f, 0.80f, 1.00f},
                .Color = {0.85f, 0.25f, 0.20f, 1.0f},
                .Metallic = 0.35f,
                .Roughness = 0.50f,
                .IsSolidObstacle = true,
                .Name = "Gaunt_Hazard2",
                .IsMoving = true,
                .MoveAxis = {1.0f, 0.0f, 0.0f},
                .MoveDistance = 1.10f,
                .MoveSpeed = 2.50f,
                .MovePhase = 0.0f,
                .BasePosition = {-7.40f, 0.80f, -9.00f}
            });

            p.LocalPlatforms.push_back(PlatformBox{
                .Position = {-11.20f, 0.20f, -13.60f},
                .Dimensions = {3.60f, 0.80f, 3.60f},
                .Color = {0.28f, 0.65f, 0.48f, 1.0f},
                .Metallic = 0.15f,
                .Roughness = 0.70f,
                .IsSolidObstacle = false,
                .Name = "Gaunt_Exit"
            });

            p.LocalExitPoint = {-11.20f, 0.60f, -13.60f};
            m_patterns.push_back(std::move(p));
        }

        // =========================================================================
        // LEVEL 10: Mastery Combinations (Difficulty 10)
        // =========================================================================

        // Pattern 26: MasteryMovingAscent (Difficulty 10)
        {
            SegmentPattern p;
            p.Name = "MasteryMovingAscent";
            p.MinDifficulty = 10;
            p.MaxDifficulty = 10;
            p.Weight = 1.0f;
            p.LocalEntryPoint = {0.0f, 0.0f, 0.0f};

            p.LocalPlatforms.push_back(PlatformBox{
                .Position = {0.00f, -0.40f, 0.00f},
                .Dimensions = {3.60f, 0.80f, 3.60f},
                .Color = {0.25f, 0.52f, 0.50f, 1.0f},
                .Metallic = 0.15f,
                .Roughness = 0.70f,
                .IsSolidObstacle = false,
                .Name = "MasteryAsc_Start"
            });

            p.LocalPlatforms.push_back(PlatformBox{
                .Position = {-3.40f, 0.20f, -4.20f},
                .Dimensions = {2.40f, 0.80f, 2.40f},
                .Color = {0.18f, 0.42f, 0.65f, 1.0f},
                .Metallic = 0.20f,
                .Roughness = 0.65f,
                .IsSolidObstacle = false,
                .Name = "MasteryAsc_PlatX",
                .IsMoving = true,
                .MoveAxis = {1.0f, 0.0f, 0.0f},
                .MoveDistance = 1.10f,
                .MoveSpeed = 2.50f,
                .MovePhase = 0.0f,
                .BasePosition = {-3.40f, 0.20f, -4.20f}
            });

            p.LocalPlatforms.push_back(PlatformBox{
                .Position = {-6.80f, 0.60f, -8.40f},
                .Dimensions = {2.40f, 0.80f, 2.40f},
                .Color = {0.20f, 0.48f, 0.70f, 1.0f},
                .Metallic = 0.20f,
                .Roughness = 0.65f,
                .IsSolidObstacle = false,
                .Name = "MasteryAsc_PlatZ",
                .IsMoving = true,
                .MoveAxis = {0.0f, 0.0f, 1.0f},
                .MoveDistance = 1.10f,
                .MoveSpeed = 2.60f,
                .MovePhase = 1.57f,
                .BasePosition = {-6.80f, 0.60f, -8.40f}
            });

            p.LocalPlatforms.push_back(PlatformBox{
                .Position = {-10.20f, 0.90f, -12.60f},
                .Dimensions = {3.80f, 0.80f, 3.80f},
                .Color = {0.22f, 0.45f, 0.52f, 1.0f},
                .Metallic = 0.15f,
                .Roughness = 0.70f,
                .IsSolidObstacle = false,
                .Name = "MasteryAsc_PlatLedge"
            });

            p.LocalPlatforms.push_back(PlatformBox{
                .Position = {-10.20f, 1.70f, -12.60f},
                .Dimensions = {1.60f, 0.80f, 1.60f},
                .Color = {0.85f, 0.20f, 0.20f, 1.0f},
                .Metallic = 0.30f,
                .Roughness = 0.50f,
                .IsSolidObstacle = true,
                .Name = "MasteryAsc_Spike",
                .BasePosition = {-10.20f, 1.70f, -12.60f},
                .IsPeriodicHazard = true,
                .HazardPeriod = 1.80f,
                .HazardActiveTime = 0.90f,
                .HazardTimer = 0.0f,
                .HazardIsActive = true
            });

            p.LocalPlatforms.push_back(PlatformBox{
                .Position = {-13.60f, 1.20f, -16.80f},
                .Dimensions = {2.40f, 0.80f, 2.40f},
                .Color = {0.18f, 0.42f, 0.65f, 1.0f},
                .Metallic = 0.20f,
                .Roughness = 0.65f,
                .IsSolidObstacle = false,
                .Name = "MasteryAsc_PlatFast",
                .IsMoving = true,
                .MoveAxis = {1.0f, 0.0f, 0.0f},
                .MoveDistance = 1.10f,
                .MoveSpeed = 2.80f,
                .MovePhase = 0.0f,
                .BasePosition = {-13.60f, 1.20f, -16.80f}
            });

            p.LocalPlatforms.push_back(PlatformBox{
                .Position = {-17.20f, 1.50f, -21.20f},
                .Dimensions = {4.00f, 0.80f, 4.00f},
                .Color = {0.28f, 0.65f, 0.48f, 1.0f},
                .Metallic = 0.15f,
                .Roughness = 0.70f,
                .IsSolidObstacle = false,
                .Name = "MasteryAsc_Exit"
            });

            p.LocalExitPoint = {-17.20f, 1.90f, -21.20f};
            m_patterns.push_back(std::move(p));
        }

        // Pattern 27: MasteryGrandGauntlet (Difficulty 10)
        {
            SegmentPattern p;
            p.Name = "MasteryGrandGauntlet";
            p.MinDifficulty = 10;
            p.MaxDifficulty = 10;
            p.Weight = 1.0f;
            p.LocalEntryPoint = {0.0f, 0.0f, 0.0f};

            p.LocalPlatforms.push_back(PlatformBox{
                .Position = {0.00f, -0.40f, 0.00f},
                .Dimensions = {3.60f, 0.80f, 3.60f},
                .Color = {0.25f, 0.52f, 0.50f, 1.0f},
                .Metallic = 0.15f,
                .Roughness = 0.70f,
                .IsSolidObstacle = false,
                .Name = "MasteryG_Start"
            });

            p.LocalPlatforms.push_back(PlatformBox{
                .Position = {-3.50f, 0.00f, -4.20f},
                .Dimensions = {2.40f, 0.80f, 2.40f},
                .Color = {0.18f, 0.42f, 0.65f, 1.0f},
                .Metallic = 0.20f,
                .Roughness = 0.65f,
                .IsSolidObstacle = false,
                .Name = "MasteryG_MoveA",
                .IsMoving = true,
                .MoveAxis = {1.0f, 0.0f, 0.0f},
                .MoveDistance = 1.10f,
                .MoveSpeed = 2.40f,
                .MovePhase = 0.0f,
                .BasePosition = {-3.50f, 0.00f, -4.20f}
            });

            p.LocalPlatforms.push_back(PlatformBox{
                .Position = {-7.00f, 0.30f, -8.40f},
                .Dimensions = {2.80f, 0.80f, 2.80f},
                .Color = {0.22f, 0.45f, 0.52f, 1.0f},
                .Metallic = 0.15f,
                .Roughness = 0.70f,
                .IsSolidObstacle = false,
                .Name = "MasteryG_Step"
            });

            p.LocalPlatforms.push_back(PlatformBox{
                .Position = {-10.50f, 0.60f, -12.60f},
                .Dimensions = {2.40f, 0.80f, 2.40f},
                .Color = {0.20f, 0.48f, 0.70f, 1.0f},
                .Metallic = 0.20f,
                .Roughness = 0.65f,
                .IsSolidObstacle = false,
                .Name = "MasteryG_MoveB",
                .IsMoving = true,
                .MoveAxis = {0.0f, 0.0f, 1.0f},
                .MoveDistance = 1.10f,
                .MoveSpeed = 2.60f,
                .MovePhase = 2.00f,
                .BasePosition = {-10.50f, 0.60f, -12.60f}
            });

            p.LocalPlatforms.push_back(PlatformBox{
                .Position = {-14.20f, 0.80f, -17.00f},
                .Dimensions = {4.00f, 0.80f, 4.00f},
                .Color = {0.20f, 0.45f, 0.58f, 1.0f},
                .Metallic = 0.15f,
                .Roughness = 0.70f,
                .IsSolidObstacle = false,
                .Name = "MasteryG_ExitPlat"
            });

            p.LocalPlatforms.push_back(PlatformBox{
                .Position = {-14.20f, 1.60f, -17.00f},
                .Dimensions = {1.00f, 0.80f, 1.00f},
                .Color = {0.85f, 0.25f, 0.20f, 1.0f},
                .Metallic = 0.35f,
                .Roughness = 0.50f,
                .IsSolidObstacle = true,
                .Name = "MasteryG_Patrol",
                .IsMoving = true,
                .MoveAxis = {1.0f, 0.0f, 0.0f},
                .MoveDistance = 1.00f,
                .MoveSpeed = 2.20f,
                .MovePhase = 0.0f,
                .BasePosition = {-14.20f, 1.60f, -17.00f}
            });

            p.LocalExitPoint = {-14.20f, 1.20f, -17.00f};
            m_patterns.push_back(std::move(p));
        }

    }

    static std::string ResolveAssetPath(const std::string& relativePath)
    {
        return ResolveAsset(relativePath);
    }

    void LevelManager::LoadPlatformModels()
    {
        std::cout << "[LevelManager] Pre-loading Kenney 3D platform and obstacle models..." << std::endl;
        m_grassLargeModel = Aion::AssetManager::LoadModel(ResolveAssetPath("assets/models/kenney_platformer-kit/models/block-grass-large.glb"));
        m_grassStandardModel = Aion::AssetManager::LoadModel(ResolveAssetPath("assets/models/kenney_platformer-kit/models/block-grass.glb"));
        m_grassNarrowModel = Aion::AssetManager::LoadModel(ResolveAssetPath("assets/models/kenney_platformer-kit/models/block-grass-narrow.glb"));
        m_snowLargeModel = Aion::AssetManager::LoadModel(ResolveAssetPath("assets/models/kenney_platformer-kit/models/block-snow-large.glb"));
        m_snowStandardModel = Aion::AssetManager::LoadModel(ResolveAssetPath("assets/models/kenney_platformer-kit/models/block-snow.glb"));
        m_snowNarrowModel = Aion::AssetManager::LoadModel(ResolveAssetPath("assets/models/kenney_platformer-kit/models/block-snow-narrow.glb"));
        m_spikeObstacleWideModel = Aion::AssetManager::LoadModel(ResolveAssetPath("assets/models/kenney_platformer-kit/models/spike-block-wide.glb"));
        m_spikeObstacleModel = Aion::AssetManager::LoadModel(ResolveAssetPath("assets/models/kenney_platformer-kit/models/spike-block.glb"));
        m_movingPlatformModel = Aion::AssetManager::LoadModel(ResolveAssetPath("assets/models/kenney_platformer-kit/models/block-moving.glb"));
        m_movingLargeModel = Aion::AssetManager::LoadModel(ResolveAssetPath("assets/models/kenney_platformer-kit/models/block-moving-large.glb"));
        m_sawModel = Aion::AssetManager::LoadModel(ResolveAssetPath("assets/models/kenney_platformer-kit/models/saw.glb"));

        auto addVariant = [](std::vector<PlatformModelVariant>& list,
                             const std::string& path,
                             float bw, float bd, float bh,
                             bool canRotate,
                             const std::string& name)
        {
            auto model = Aion::AssetManager::LoadModel(ResolveAssetPath(path));
            if (model)
            {
                list.push_back(PlatformModelVariant{
                    .Model = model,
                    .BaseWidth = bw,
                    .BaseDepth = bd,
                    .BaseHeight = bh,
                    .CanRotateY = canRotate,
                    .Name = name
                });
            }
        };

        // 1. Grass Large variants (>= 3.0m)
        m_grassLargeVariants.clear();
        addVariant(m_grassLargeVariants, "assets/models/kenney_platformer-kit/models/block-grass-large.glb", 2.0f, 2.0f, 1.0f, false, "block-grass-large");
        addVariant(m_grassLargeVariants, "assets/models/kenney_platformer-kit/models/block-grass-overhang-large.glb", 2.0f, 2.0f, 1.0f, false, "block-grass-overhang-large");
        addVariant(m_grassLargeVariants, "assets/models/kenney_platformer-kit/models/block-grass-low-large.glb", 2.0f, 2.0f, 0.5f, false, "block-grass-low-large");
        addVariant(m_grassLargeVariants, "assets/models/kenney_platformer-kit/models/block-grass-overhang-low-large.glb", 2.0f, 2.0f, 0.5f, false, "block-grass-overhang-low-large");

        // 2. Grass Standard variants (2.0m - 3.0m)
        m_grassStandardVariants.clear();
        addVariant(m_grassStandardVariants, "assets/models/kenney_platformer-kit/models/block-grass.glb", 1.0f, 1.0f, 1.0f, false, "block-grass");
        addVariant(m_grassStandardVariants, "assets/models/kenney_platformer-kit/models/block-grass-overhang-edge.glb", 1.0f, 1.0f, 1.0f, true, "block-grass-overhang-edge");
        addVariant(m_grassStandardVariants, "assets/models/kenney_platformer-kit/models/block-grass-overhang-corner.glb", 1.0f, 1.0f, 1.0f, true, "block-grass-overhang-corner");
        addVariant(m_grassStandardVariants, "assets/models/kenney_platformer-kit/models/block-grass-edge.glb", 1.0f, 1.0f, 1.0f, true, "block-grass-edge");
        addVariant(m_grassStandardVariants, "assets/models/kenney_platformer-kit/models/block-grass-corner.glb", 1.0f, 1.0f, 1.0f, true, "block-grass-corner");
        addVariant(m_grassStandardVariants, "assets/models/kenney_platformer-kit/models/block-grass-low.glb", 1.0f, 1.0f, 0.5f, false, "block-grass-low");
        addVariant(m_grassStandardVariants, "assets/models/kenney_platformer-kit/models/block-grass-overhang-low.glb", 1.0f, 1.0f, 0.5f, false, "block-grass-overhang-low");
        addVariant(m_grassStandardVariants, "assets/models/kenney_platformer-kit/models/block-grass-corner-low.glb", 1.0f, 1.0f, 0.5f, true, "block-grass-corner-low");
        addVariant(m_grassStandardVariants, "assets/models/kenney_platformer-kit/models/block-grass-corner-overhang-low.glb", 1.0f, 1.0f, 0.5f, true, "block-grass-corner-overhang-low");

        // 3. Grass Narrow variants (< 2.0m)
        m_grassNarrowVariants.clear();
        addVariant(m_grassNarrowVariants, "assets/models/kenney_platformer-kit/models/block-grass-narrow.glb", 0.7f, 0.7f, 1.0f, false, "block-grass-narrow");
        addVariant(m_grassNarrowVariants, "assets/models/kenney_platformer-kit/models/block-grass-overhang-narrow.glb", 0.7f, 0.7f, 1.0f, false, "block-grass-overhang-narrow");
        addVariant(m_grassNarrowVariants, "assets/models/kenney_platformer-kit/models/block-grass-low-narrow.glb", 0.7f, 0.7f, 0.5f, false, "block-grass-low-narrow");
        addVariant(m_grassNarrowVariants, "assets/models/kenney_platformer-kit/models/block-grass-overhang-low-narrow.glb", 0.7f, 0.7f, 0.5f, false, "block-grass-overhang-low-narrow");

        // 4. Snow Large variants (>= 3.0m)
        m_snowLargeVariants.clear();
        addVariant(m_snowLargeVariants, "assets/models/kenney_platformer-kit/models/block-snow-large.glb", 2.0f, 2.0f, 1.0f, false, "block-snow-large");
        addVariant(m_snowLargeVariants, "assets/models/kenney_platformer-kit/models/block-snow-overhang-large.glb", 2.0f, 2.0f, 1.0f, false, "block-snow-overhang-large");
        addVariant(m_snowLargeVariants, "assets/models/kenney_platformer-kit/models/block-snow-low-large.glb", 2.0f, 2.0f, 0.5f, false, "block-snow-low-large");
        addVariant(m_snowLargeVariants, "assets/models/kenney_platformer-kit/models/block-snow-overhang-low-large.glb", 2.0f, 2.0f, 0.5f, false, "block-snow-overhang-low-large");

        // 5. Snow Standard variants (2.0m - 3.0m)
        m_snowStandardVariants.clear();
        addVariant(m_snowStandardVariants, "assets/models/kenney_platformer-kit/models/block-snow.glb", 1.0f, 1.0f, 1.0f, false, "block-snow");
        addVariant(m_snowStandardVariants, "assets/models/kenney_platformer-kit/models/block-snow-overhang-edge.glb", 1.0f, 1.0f, 1.0f, true, "block-snow-overhang-edge");
        addVariant(m_snowStandardVariants, "assets/models/kenney_platformer-kit/models/block-snow-overhang-corner.glb", 1.0f, 1.0f, 1.0f, true, "block-snow-overhang-corner");
        addVariant(m_snowStandardVariants, "assets/models/kenney_platformer-kit/models/block-snow-edge.glb", 1.0f, 1.0f, 1.0f, true, "block-snow-edge");
        addVariant(m_snowStandardVariants, "assets/models/kenney_platformer-kit/models/block-snow-corner.glb", 1.0f, 1.0f, 1.0f, true, "block-snow-corner");
        addVariant(m_snowStandardVariants, "assets/models/kenney_platformer-kit/models/block-snow-low.glb", 1.0f, 1.0f, 0.5f, false, "block-snow-low");
        addVariant(m_snowStandardVariants, "assets/models/kenney_platformer-kit/models/block-snow-overhang-low.glb", 1.0f, 1.0f, 0.5f, false, "block-snow-overhang-low");
        addVariant(m_snowStandardVariants, "assets/models/kenney_platformer-kit/models/block-snow-corner-low.glb", 1.0f, 1.0f, 0.5f, true, "block-snow-corner-low");
        addVariant(m_snowStandardVariants, "assets/models/kenney_platformer-kit/models/block-snow-corner-overhang-low.glb", 1.0f, 1.0f, 0.5f, true, "block-snow-corner-overhang-low");

        // 6. Snow Narrow variants (< 2.0m)
        m_snowNarrowVariants.clear();
        addVariant(m_snowNarrowVariants, "assets/models/kenney_platformer-kit/models/block-snow-narrow.glb", 0.7f, 0.7f, 1.0f, false, "block-snow-narrow");
        addVariant(m_snowNarrowVariants, "assets/models/kenney_platformer-kit/models/block-snow-overhang-narrow.glb", 0.7f, 0.7f, 1.0f, false, "block-snow-overhang-narrow");
        addVariant(m_snowNarrowVariants, "assets/models/kenney_platformer-kit/models/block-snow-low-narrow.glb", 0.7f, 0.7f, 0.5f, false, "block-snow-low-narrow");
        addVariant(m_snowNarrowVariants, "assets/models/kenney_platformer-kit/models/block-snow-overhang-low-narrow.glb", 0.7f, 0.7f, 0.5f, false, "block-snow-overhang-low-narrow");

        std::cout << "[LevelManager] Pre-loaded "
                  << (m_grassLargeVariants.size() + m_grassStandardVariants.size() + m_grassNarrowVariants.size())
                  << " Grass platform variants and "
                  << (m_snowLargeVariants.size() + m_snowStandardVariants.size() + m_snowNarrowVariants.size())
                  << " Snow platform variants for dynamic course generation." << std::endl;
    }

    void LevelManager::LoadEnvironmentModels()
    {
        std::cout << "[LevelManager] Pre-loading Kenney Mini-Forest and Platformer-Kit decoration models..." << std::endl;

        // Mini-Forest models
        m_mfTreeModel = Aion::AssetManager::LoadModel(ResolveAssetPath("assets/models/kenney_mini-forest_1.0/models/tree.glb"));
        m_mfTreeHighModel = Aion::AssetManager::LoadModel(ResolveAssetPath("assets/models/kenney_mini-forest_1.0/models/tree-high.glb"));
        m_mfPlantModel = Aion::AssetManager::LoadModel(ResolveAssetPath("assets/models/kenney_mini-forest_1.0/models/plant.glb"));
        m_mfStonesModel = Aion::AssetManager::LoadModel(ResolveAssetPath("assets/models/kenney_mini-forest_1.0/models/stones.glb"));
        m_mfRocksLowModel = Aion::AssetManager::LoadModel(ResolveAssetPath("assets/models/kenney_mini-forest_1.0/models/rocks-low.glb"));
        m_mfRocksHighModel = Aion::AssetManager::LoadModel(ResolveAssetPath("assets/models/kenney_mini-forest_1.0/models/rocks-high.glb"));
        m_mfTentModel = Aion::AssetManager::LoadModel(ResolveAssetPath("assets/models/kenney_mini-forest_1.0/models/tent.glb"));
        m_mfFenceModel = Aion::AssetManager::LoadModel(ResolveAssetPath("assets/models/kenney_mini-forest_1.0/models/fence.glb"));
        m_mfPatchGrassModel = Aion::AssetManager::LoadModel(ResolveAssetPath("assets/models/kenney_mini-forest_1.0/models/patch-grass.glb"));
        m_mfPatchDirtModel = Aion::AssetManager::LoadModel(ResolveAssetPath("assets/models/kenney_mini-forest_1.0/models/patch-dirt.glb"));

        // Platformer-Kit models
        m_pkGrassModel = Aion::AssetManager::LoadModel(ResolveAssetPath("assets/models/kenney_platformer-kit/models/grass.glb"));
        m_pkFlowersModel = Aion::AssetManager::LoadModel(ResolveAssetPath("assets/models/kenney_platformer-kit/models/flowers.glb"));
        m_pkFlowersTallModel = Aion::AssetManager::LoadModel(ResolveAssetPath("assets/models/kenney_platformer-kit/models/flowers-tall.glb"));
        m_pkPlantModel = Aion::AssetManager::LoadModel(ResolveAssetPath("assets/models/kenney_platformer-kit/models/plant.glb"));
        m_pkRocksModel = Aion::AssetManager::LoadModel(ResolveAssetPath("assets/models/kenney_platformer-kit/models/rocks.glb"));
        m_pkStonesModel = Aion::AssetManager::LoadModel(ResolveAssetPath("assets/models/kenney_platformer-kit/models/stones.glb"));
        m_pkMushroomsModel = Aion::AssetManager::LoadModel(ResolveAssetPath("assets/models/kenney_platformer-kit/models/mushrooms.glb"));
        m_pkFenceLowModel = Aion::AssetManager::LoadModel(ResolveAssetPath("assets/models/kenney_platformer-kit/models/fence-low-straight.glb"));
        m_pkFenceBrokenModel = Aion::AssetManager::LoadModel(ResolveAssetPath("assets/models/kenney_platformer-kit/models/fence-broken.glb"));
        m_pkCrateModel = Aion::AssetManager::LoadModel(ResolveAssetPath("assets/models/kenney_platformer-kit/models/crate.glb"));
        m_pkTreePineModel = Aion::AssetManager::LoadModel(ResolveAssetPath("assets/models/kenney_platformer-kit/models/tree-pine.glb"));
        m_pkTreePineSmallModel = Aion::AssetManager::LoadModel(ResolveAssetPath("assets/models/kenney_platformer-kit/models/tree-pine-small.glb"));
        m_pkSignModel = Aion::AssetManager::LoadModel(ResolveAssetPath("assets/models/kenney_platformer-kit/models/sign.glb"));

        // Snow / Mountain nature models
        m_pkTreePineSnowModel = Aion::AssetManager::LoadModel(ResolveAssetPath("assets/models/kenney_platformer-kit/models/tree-pine-snow.glb"));
        m_pkTreePineSnowSmallModel = Aion::AssetManager::LoadModel(ResolveAssetPath("assets/models/kenney_platformer-kit/models/tree-pine-snow-small.glb"));
        m_pkTreeSnowModel = Aion::AssetManager::LoadModel(ResolveAssetPath("assets/models/kenney_platformer-kit/models/tree-snow.glb"));

        // Tune decoration material properties for balanced brightness and visual readability against platforms
        auto tuneDecorMaterials = [](const std::shared_ptr<Aion::Model>& model) {
            if (!model) return;
            for (const auto& mat : model->GetMaterials())
            {
                if (mat)
                {
                    mat->BaseColor = Aion::Vector4(1.15f, 1.15f, 1.15f, 1.0f);
                    mat->RoughnessFactor = 0.65f;
                    mat->EmissiveFactor = Aion::Vector3(0.05f, 0.05f, 0.05f);
                }
            }
        };

        const std::shared_ptr<Aion::Model> decorModels[] = {
            m_mfTreeModel, m_mfTreeHighModel, m_mfPlantModel, m_mfStonesModel,
            m_mfRocksLowModel, m_mfRocksHighModel, m_mfTentModel, m_mfFenceModel,
            m_mfPatchGrassModel, m_mfPatchDirtModel, m_pkGrassModel, m_pkFlowersModel,
            m_pkFlowersTallModel, m_pkPlantModel, m_pkRocksModel, m_pkStonesModel,
            m_pkMushroomsModel, m_pkFenceLowModel, m_pkFenceBrokenModel, m_pkCrateModel,
            m_pkTreePineModel, m_pkTreePineSmallModel, m_pkSignModel,
            m_pkTreePineSnowModel, m_pkTreePineSnowSmallModel, m_pkTreeSnowModel
        };
        for (const auto& dm : decorModels)
        {
            tuneDecorMaterials(dm);
        }

        std::cout << "[LevelManager] Pre-loaded environment and platform decoration models successfully (materials tuned for contrast)." << std::endl;
    }

    std::shared_ptr<Aion::Object3D> LevelManager::CreateDecorObject(
        const std::shared_ptr<Aion::Model>& model,
        const Aion::Vector3& position,
        float scale,
        float rotYDegrees,
        const std::string& name)
    {
        if (!model)
        {
            return nullptr;
        }

        auto obj = Aion::ModelImporter::CreateModelObject(model);
        if (!obj)
        {
            return nullptr;
        }

        obj->SetName(name);
        obj->Transform.Position = position;
        if (std::abs(rotYDegrees) > 0.001f)
        {
            obj->Transform.Rotation = Aion::Quaternion::FromEuler(0.0f, Aion::Math::Radians(rotYDegrees), 0.0f);
        }
        obj->Transform.Scale = Aion::Vector3(scale);

        if (m_scene)
        {
            m_scene->Add(obj);
        }
        return obj;
    }

    PlatformModelVariant LevelManager::SelectPlatformModelVariant(
        const PlatformBox& plat,
        int difficulty)
    {
        float maxHoriz = std::max(plat.Dimensions.x, plat.Dimensions.z);
        float minHoriz = std::min(plat.Dimensions.x, plat.Dimensions.z);

        const std::vector<PlatformModelVariant>* pool = nullptr;

        if (difficulty >= 4) // Stages 4 & 5 (Hard & Expert) use snow theme
        {
            if (maxHoriz >= 3.0f && !m_snowLargeVariants.empty())
            {
                pool = &m_snowLargeVariants;
            }
            else if (minHoriz < 2.0f && !m_snowNarrowVariants.empty())
            {
                pool = &m_snowNarrowVariants;
            }
            else if (!m_snowStandardVariants.empty())
            {
                pool = &m_snowStandardVariants;
            }
        }
        else // Stages 1, 2, 3 use grass theme
        {
            if (maxHoriz >= 3.0f && !m_grassLargeVariants.empty())
            {
                pool = &m_grassLargeVariants;
            }
            else if (minHoriz < 2.0f && !m_grassNarrowVariants.empty())
            {
                pool = &m_grassNarrowVariants;
            }
            else if (!m_grassStandardVariants.empty())
            {
                pool = &m_grassStandardVariants;
            }
        }

        if (!pool || pool->empty())
        {
            return PlatformModelVariant{nullptr, 1.0f, 1.0f, 1.0f, false, "Fallback"};
        }

        // Repetition avoidance: avoid picking the exact same visual model variant for consecutive platforms
        std::vector<const PlatformModelVariant*> validCandidates;
        validCandidates.reserve(pool->size());
        for (const auto& variant : *pool)
        {
            if (pool->size() > 1 && variant.Name == m_lastSelectedModelName)
            {
                continue;
            }
            validCandidates.push_back(&variant);
        }

        if (validCandidates.empty())
        {
            for (const auto& variant : *pool)
            {
                validCandidates.push_back(&variant);
            }
        }

        std::uniform_int_distribution<size_t> dist(0, validCandidates.size() - 1);
        const auto* chosen = validCandidates[dist(m_rng)];
        m_lastSelectedModelName = chosen->Name;
        return *chosen;
    }

    std::shared_ptr<Aion::Object3D> LevelManager::CreatePlatformObject(
        const PlatformBox& plat,
        int difficulty,
        const std::string& objName)
    {
        std::shared_ptr<Aion::Model> selectedModel = nullptr;
        float baseWidth = 1.0f;
        float baseDepth = 1.0f;
        float baseHeight = 1.0f;
        Aion::Quaternion rotation = Aion::Quaternion::Identity();

        if (plat.IsSolidObstacle)
        {
            float maxDim = std::max(plat.Dimensions.x, plat.Dimensions.z);
            if ((plat.IsMoving || plat.IsPeriodicHazard) && m_sawModel && maxDim <= 1.4f)
            {
                selectedModel = m_sawModel;
                baseWidth = 1.0f;
                baseDepth = 1.0f;
                baseHeight = 1.0f;
            }
            else if (maxDim >= 1.8f && m_spikeObstacleWideModel)
            {
                selectedModel = m_spikeObstacleWideModel;
                baseHeight = 0.9f;
                if (plat.Dimensions.x < plat.Dimensions.z)
                {
                    // Obstacle runs along Z; rotate model 90 degrees so wide X axis runs along Z
                    rotation = Aion::Quaternion::FromEuler(0.0f, Aion::Math::Radians(90.0f), 0.0f);
                    baseWidth = 0.9f;
                    baseDepth = 2.4f;
                }
                else
                {
                    baseWidth = 2.4f;
                    baseDepth = 0.9f;
                }
            }
            else if (m_spikeObstacleModel)
            {
                selectedModel = m_spikeObstacleModel;
                baseWidth = 0.9f;
                baseDepth = 0.9f;
                baseHeight = 0.9f;
            }
        }
        else
        {
            // Deterministic model for starting plaza to keep lighting stability pixel check consistent
            if (plat.Name && std::strcmp(plat.Name, "StartPlatform") == 0 && m_grassLargeModel)
            {
                selectedModel = m_grassLargeModel;
                baseWidth = 2.0f;
                baseDepth = 2.0f;
                baseHeight = 1.0f;
            }
            else if (plat.IsMoving)
            {
                float minDim = std::min(plat.Dimensions.x, plat.Dimensions.z);
                if (minDim >= 2.0f && m_movingLargeModel)
                {
                    selectedModel = m_movingLargeModel;
                    baseWidth = 2.0f;
                    baseDepth = 2.0f;
                    baseHeight = 1.0f;
                }
                else if (m_movingPlatformModel)
                {
                    selectedModel = m_movingPlatformModel;
                    baseWidth = 1.0f;
                    baseDepth = 1.0f;
                    baseHeight = 1.0f;
                }
                else
                {
                    auto variant = SelectPlatformModelVariant(plat, difficulty);
                    selectedModel = variant.Model;
                    baseWidth = variant.BaseWidth;
                    baseDepth = variant.BaseDepth;
                    baseHeight = variant.BaseHeight;
                }
            }
            else
            {
                auto variant = SelectPlatformModelVariant(plat, difficulty);
                selectedModel = variant.Model;
                baseWidth = variant.BaseWidth;
                baseDepth = variant.BaseDepth;
                baseHeight = variant.BaseHeight;

                // For square platforms that support rotation, introduce 90-degree visual variation
                if (variant.CanRotateY && std::abs(plat.Dimensions.x - plat.Dimensions.z) < 0.25f)
                {
                    std::uniform_int_distribution<int> rotDist(0, 3);
                    int rotStep = rotDist(m_rng);
                    if (rotStep > 0)
                    {
                        rotation = Aion::Quaternion::FromEuler(0.0f, Aion::Math::Radians(90.0f * rotStep), 0.0f);
                    }
                }
            }
        }

        if (selectedModel)
        {
            auto platObj = Aion::ModelImporter::CreateModelObject(selectedModel);
            if (platObj)
            {
                platObj->SetName(objName);
                // Kenney models: pivot at base (Y=0), top surface at Y=baseHeight.
                // Align base to (plat.Position.y - 0.5f * plat.Dimensions.y) so that
                // the rendered top surface precisely matches plat.GetTopSurfaceY().
                platObj->Transform.Position = Aion::Vector3(
                    plat.Position.x,
                    plat.Position.y - 0.5f * plat.Dimensions.y,
                    plat.Position.z);
                platObj->Transform.Rotation = rotation;
                platObj->Transform.Scale = Aion::Vector3(
                    plat.Dimensions.x / baseWidth,
                    plat.Dimensions.y / baseHeight,
                    plat.Dimensions.z / baseDepth);

                return platObj;
            }
        }

        // Safe fallback to primitive cube mesh if model loading failed
        auto platObj = std::make_shared<Aion::Object3D>(objName);
        auto mat = std::make_shared<Aion::Material>();
        mat->BaseColor = plat.Color;
        mat->MetallicFactor = plat.Metallic;
        mat->RoughnessFactor = plat.Roughness;

        platObj->AddComponent<Aion::MeshRenderer>(m_cubeMesh, mat);
        platObj->Transform.Position = plat.Position;
        platObj->Transform.Scale = plat.Dimensions;

        return platObj;
    }

    void LevelManager::SpawnStartingPlatform()
    {
        std::vector<PlatformBox> startPlatforms;
        startPlatforms.reserve(3);

        startPlatforms.push_back(PlatformBox{
            .Position = {0.0f, -0.4f, 0.0f},
            .Dimensions = {7.0f, 0.8f, 7.0f},
            .Color = {0.25f, 0.52f, 0.50f, 1.0f}, // Slate Teal
            .Metallic = 0.15f,
            .Roughness = 0.70f,
            .IsSolidObstacle = false,
            .Name = "StartPlatform"
        });

        startPlatforms.push_back(PlatformBox{
            .Position = {3.1f, 0.15f, 3.1f},
            .Dimensions = {0.6f, 0.3f, 0.6f},
            .Color = {0.85f, 0.78f, 0.65f, 1.0f},
            .Metallic = 0.10f,
            .Roughness = 0.85f,
            .IsSolidObstacle = false,
            .Name = "StartCornerSE"
        });

        startPlatforms.push_back(PlatformBox{
            .Position = {0.0f, 0.15f, 3.2f},
            .Dimensions = {4.0f, 0.3f, 0.4f},
            .Color = {0.85f, 0.78f, 0.65f, 1.0f},
            .Metallic = 0.10f,
            .Roughness = 0.85f,
            .IsSolidObstacle = false,
            .Name = "StartBorderS"
        });

        std::vector<std::shared_ptr<Aion::Object3D>> sceneObjs;
        sceneObjs.reserve(startPlatforms.size());

        for (const auto& plat : startPlatforms)
        {
            auto platObj = CreatePlatformObject(plat, 1, plat.Name);
            if (m_scene)
            {
                m_scene->Add(platObj);
            }
            sceneObjs.push_back(platObj);
        }

        std::vector<std::shared_ptr<Aion::Object3D>> decorObjs;

        auto addDecor = [&](const std::shared_ptr<Aion::Model>& model, const Aion::Vector3& pos, float scale, float rotY, const std::string& name) {
            auto decor = CreateDecorObject(model, pos, scale, rotY, name);
            if (decor)
            {
                decorObjs.push_back(decor);
            }
        };

        // Distinctive Adventurer Basecamp on North-West corner (-X, +Z)
        addDecor(m_mfTentModel, Aion::Vector3(-2.2f, 0.0f, 2.0f), 1.0f, 45.0f, "Start_Tent");
        addDecor(m_mfTreeHighModel, Aion::Vector3(-2.8f, 0.0f, 2.7f), 1.1f, -25.0f, "Start_TreeHighNW");
        addDecor(m_mfStonesModel, Aion::Vector3(-1.3f, 0.0f, 2.5f), 0.85f, 15.0f, "Start_StonesNW");
        addDecor(m_mfFenceModel, Aion::Vector3(-2.4f, 0.0f, 3.2f), 1.0f, 0.0f, "Start_FenceNW1");
        addDecor(m_mfFenceModel, Aion::Vector3(-1.3f, 0.0f, 3.2f), 1.0f, 0.0f, "Start_FenceNW2");
        addDecor(m_pkFlowersModel, Aion::Vector3(-2.0f, 0.0f, 1.0f), 0.9f, 60.0f, "Start_FlowersNW");

        // Wooden direction signpost guiding player towards the course
        addDecor(m_pkSignModel, Aion::Vector3(2.5f, 0.0f, -0.8f), 1.1f, -45.0f, "Start_Signpost");

        // Nature accents on South and West perimeters
        addDecor(m_mfTreeModel, Aion::Vector3(2.8f, 0.0f, -2.6f), 0.95f, 110.0f, "Start_TreeS");
        addDecor(m_pkPlantModel, Aion::Vector3(2.2f, 0.0f, -2.8f), 0.9f, 0.0f, "Start_PlantS");
        addDecor(m_pkFlowersTallModel, Aion::Vector3(-2.6f, 0.0f, -1.8f), 0.85f, -70.0f, "Start_FlowersW");
        addDecor(m_pkGrassModel, Aion::Vector3(1.4f, 0.0f, 2.8f), 1.0f, 30.0f, "Start_GrassNE");

        Aion::Vector3 entryPoint{m_config.Player.SpawnPosition.x, 0.0f, m_config.Player.SpawnPosition.z};
        Aion::Vector3 exitPoint{-6.72f, 0.0f, -6.72f}; // Visible 1.45 unit gap, no overlap with StartPlatform
        m_lastExitPoint = exitPoint;

        auto startSegment = std::make_shared<LevelSegment>(
            0,
            "StartPlaza",
            1, // Starting plaza difficulty level 1
            entryPoint,
            exitPoint,
            std::move(startPlatforms),
            std::move(sceneObjs),
            std::move(decorObjs));

        m_activeSegments.push_back(startSegment);
        m_segmentsGenerated = 1;

        m_segmentLengths.clear();
        m_segmentCumulativeStarts.clear();
        m_segmentLengths.push_back(startSegment->GetLengthXZ());
        m_segmentCumulativeStarts.push_back(0.0f);
    }

    bool LevelManager::ValidatePatternTraversal(
        const SegmentPattern& pattern,
        const Aion::Vector3& prevExitPoint) const
    {
        if (pattern.LocalPlatforms.empty())
        {
            return false;
        }

        Aion::Vector3 worldOffset = prevExitPoint - pattern.LocalEntryPoint;

        // Platform extent along a 2D horizontal direction vector (accounting for independent width & depth)
        auto getPlatformExtentAlongDir = [](const PlatformBox& plat, float dirX, float dirZ) -> float {
            float absX = std::abs(dirX);
            float absZ = std::abs(dirZ);
            if (absX < 0.0001f && absZ < 0.0001f)
            {
                return 0.5f * std::min(plat.Dimensions.x, plat.Dimensions.z);
            }
            float extentX = (absX > 0.0001f) ? (0.5f * plat.Dimensions.x / absX) : 100.0f;
            float extentZ = (absZ > 0.0001f) ? (0.5f * plat.Dimensions.z / absZ) : 100.0f;
            return std::min(extentX, extentZ);
        };

        // 1. Separate traversable platforms vs intentional solid obstacles on platforms
        std::vector<const PlatformBox*> traversablePlatforms;
        traversablePlatforms.reserve(pattern.LocalPlatforms.size());

        for (const auto& plat : pattern.LocalPlatforms)
        {
            if (!plat.IsSolidObstacle)
            {
                if (plat.Dimensions.x < m_config.Difficulty.MinPlatformDimension ||
                    plat.Dimensions.z < m_config.Difficulty.MinPlatformDimension)
                {
                    return false;
                }
                traversablePlatforms.push_back(&plat);
            }
        }

        if (traversablePlatforms.empty())
        {
            return false;
        }

        // 2. Footprint separation check: prevent unintended overlap or touching between any two platforms
        for (size_t i = 0; i < traversablePlatforms.size(); ++i)
        {
            const auto* platA = traversablePlatforms[i];
            for (size_t j = i + 1; j < traversablePlatforms.size(); ++j)
            {
                const auto* platB = traversablePlatforms[j];
                float gapX = std::abs(platB->Position.x - platA->Position.x) - 0.5f * (platA->Dimensions.x + platB->Dimensions.x);
                float gapZ = std::abs(platB->Position.z - platA->Position.z) - 0.5f * (platA->Dimensions.z + platB->Dimensions.z);

                // If both gaps are less than 0.05f, footprints overlap or touch edge-to-edge
                if (gapX < 0.05f && gapZ < 0.05f)
                {
                    return false;
                }
            }
        }

        // 3. Entry connection check: jump from prevExitPoint to pattern's first platform
        const auto& firstPlat = pattern.LocalPlatforms.front();
        Aion::Vector3 firstCenter = firstPlat.Position + worldOffset;
        float firstTopY = firstPlat.GetTopSurfaceY() + worldOffset.y;
        float entryAscent = firstTopY - prevExitPoint.y;

        if (entryAscent > m_config.Difficulty.MaxAscentHeight + 0.01f ||
            entryAscent < -m_config.Difficulty.MaxDescentHeight - 0.01f)
        {
            return false;
        }

        float entryDx = firstCenter.x - prevExitPoint.x;
        float entryDz = firstCenter.z - prevExitPoint.z;
        float entryCenterDist = std::sqrt(entryDx * entryDx + entryDz * entryDz);
        if (entryCenterDist > 0.001f)
        {
            float dirX = entryDx / entryCenterDist;
            float dirZ = entryDz / entryCenterDist;
            float firstExtent = getPlatformExtentAlongDir(firstPlat, dirX, dirZ);
            float entryGap = std::max(0.0f, entryCenterDist - firstExtent);
            if (entryGap > m_config.Difficulty.MaxJumpGapLimit)
            {
                return false;
            }
        }

        // 4. Validate intra-segment jumps between consecutive traversable platforms
        for (size_t i = 0; i + 1 < traversablePlatforms.size(); ++i)
        {
            const auto* platA = traversablePlatforms[i];
            const auto* platB = traversablePlatforms[i + 1];

            float dx = platB->Position.x - platA->Position.x;
            float dz = platB->Position.z - platA->Position.z;
            float horizontalDist = std::sqrt(dx * dx + dz * dz);

            if (horizontalDist < 0.001f)
            {
                return false;
            }

            float dirX = dx / horizontalDist;
            float dirZ = dz / horizontalDist;

            // Directional half-extents along travel vector (accounting for actual platform dimensions)
            float extentA = getPlatformExtentAlongDir(*platA, dirX, dirZ);
            float extentB = getPlatformExtentAlongDir(*platB, dirX, dirZ);
            float actualGap = horizontalDist - (extentA + extentB);

            // Must have visible horizontal separation (actualGap >= 0.50f) and be reachable (<= MaxJumpGapLimit)
            if (actualGap < 0.50f || actualGap > m_config.Difficulty.MaxJumpGapLimit)
            {
                return false;
            }

            // Vertical climb / descent limits based on player physics
            float stepAscent = platB->GetTopSurfaceY() - platA->GetTopSurfaceY();
            if (stepAscent > m_config.Difficulty.MaxAscentHeight + 0.01f ||
                stepAscent < -m_config.Difficulty.MaxDescentHeight - 0.01f)
            {
                return false;
            }

            // Kinematic reach validation: air time on climb reduces horizontal jump reach
            if (stepAscent > 0.0f)
            {
                float v0 = m_config.Player.JumpVelocity;
                float g = m_config.Player.Gravity;
                float disc = v0 * v0 - 2.0f * g * stepAscent;
                if (disc < 0.0f)
                {
                    return false;
                }
                float tLand = (v0 + std::sqrt(disc)) / g;
                float maxReach = m_config.Player.MoveSpeed * tLand;
                if (actualGap > maxReach - 0.20f)
                {
                    return false;
                }
            }

            // 3D distance between landing surfaces
            float verticalDist = std::abs(stepAscent);
            float gap3D = std::sqrt(actualGap * actualGap + verticalDist * verticalDist);
            if (gap3D > m_config.Difficulty.MaxJumpGapLimit + 1.2f)
            {
                return false;
            }
        }

        // 5. Exit point connection check: jump from last platform to next segment's entry platform
        // Account for actual platform dimensions when calculating jump distances
        const auto* lastPlat = traversablePlatforms.back();
        float exitDx = pattern.LocalExitPoint.x - lastPlat->Position.x;
        float exitDz = pattern.LocalExitPoint.z - lastPlat->Position.z;
        float exitDist = std::sqrt(exitDx * exitDx + exitDz * exitDz);

        float dirExitX = (exitDist > 0.001f) ? (exitDx / exitDist) : 0.0f;
        float dirExitZ = (exitDist > 0.001f) ? (exitDz / exitDist) : 0.0f;

        float extentLast = getPlatformExtentAlongDir(*lastPlat, dirExitX, dirExitZ);
        
        // Stage-appropriate expected landing platform dimension
        float landingDim = m_config.Difficulty.MinPlatformDimension;
        if (pattern.MinDifficulty == 1)
        {
            landingDim = 4.2f;
        }
        else if (pattern.MinDifficulty == 2)
        {
            landingDim = 3.4f;
        }
        else if (pattern.MinDifficulty == 3)
        {
            landingDim = 3.0f;
        }
        else if (pattern.MinDifficulty == 4)
        {
            landingDim = 2.4f;
        }
        else
        {
            landingDim = 2.0f;
        }

        float extentLanding = 0.5f * landingDim;
        if (std::abs(dirExitX) > 0.0001f || std::abs(dirExitZ) > 0.0001f)
        {
            float elX = (std::abs(dirExitX) > 0.0001f) ? (0.5f * landingDim / std::abs(dirExitX)) : 100.0f;
            float elZ = (std::abs(dirExitZ) > 0.0001f) ? (0.5f * landingDim / std::abs(dirExitZ)) : 100.0f;
            extentLanding = std::min(elX, elZ);
        }

        float exitGap = exitDist - (extentLast + extentLanding);

        if (exitGap < 0.50f || exitGap > m_config.Difficulty.MaxJumpGapLimit)
        {
            return false;
        }

        float exitAscent = pattern.LocalExitPoint.y - lastPlat->GetTopSurfaceY();
        if (exitAscent > m_config.Difficulty.MaxAscentHeight + 0.01f ||
            exitAscent < -m_config.Difficulty.MaxDescentHeight - 0.01f)
        {
            return false;
        }

        return true;
    }

    int LevelManager::SelectNextPatternIndex(int targetDifficulty)
    {
        if (m_patterns.empty())
        {
            return 0;
        }

        // 1. Filter patterns that support targetDifficulty
        std::vector<int> matching;
        for (size_t i = 0; i < m_patterns.size(); ++i)
        {
            if (targetDifficulty >= m_patterns[i].MinDifficulty &&
                targetDifficulty <= m_patterns[i].MaxDifficulty)
            {
                matching.push_back(static_cast<int>(i));
            }
        }

        if (matching.empty())
        {
            for (size_t i = 0; i < m_patterns.size(); ++i)
            {
                matching.push_back(static_cast<int>(i));
            }
        }

        // 2. Lateral drift balancing across the course:
        // Camera-space lateral coordinate of current exit: L = (X - Z) / sqrt(2)
        float currentLateral = (m_lastExitPoint.x - m_lastExitPoint.z) * 0.70710678f;
        std::vector<int> laterallyBalanced;
        for (int idx : matching)
        {
            const auto& p = m_patterns[idx];
            float netL = (p.LocalExitPoint.x - p.LocalExitPoint.z) * 0.70710678f;
            if (currentLateral > 2.5f && netL > 0.5f)
            {
                continue; // Avoid excessive right drift
            }
            if (currentLateral < -2.5f && netL < -0.5f)
            {
                continue; // Avoid excessive left drift
            }
            laterallyBalanced.push_back(idx);
        }
        if (laterallyBalanced.empty())
        {
            laterallyBalanced = matching;
        }

        // 3. Separate into fresh candidates (not recently used)
        std::vector<int> freshCandidates;
        for (int idx : laterallyBalanced)
        {
            bool inHistory = false;
            for (int hist : m_recentPatternsHistory)
            {
                if (hist == idx)
                {
                    inHistory = true;
                    break;
                }
            }
            if (!inHistory)
            {
                freshCandidates.push_back(idx);
            }
        }

        int chosenIdx = -1;

        if (!freshCandidates.empty())
        {
            std::uniform_int_distribution<size_t> dist(0, freshCandidates.size() - 1);
            chosenIdx = freshCandidates[dist(m_rng)];
        }
        else
        {
            // If all matching candidates are in recent history, pick the least recently used
            int leastRecent = laterallyBalanced.front();
            size_t oldestPos = m_recentPatternsHistory.size();
            for (int cand : laterallyBalanced)
            {
                for (size_t h = 0; h < m_recentPatternsHistory.size(); ++h)
                {
                    if (m_recentPatternsHistory[h] == cand)
                    {
                        if (h < oldestPos)
                        {
                            oldestPos = h;
                            leastRecent = cand;
                        }
                        break;
                    }
                }
            }
            chosenIdx = leastRecent;
        }

        // 4. Validate traversal fairness
        if (!ValidatePatternTraversal(m_patterns[chosenIdx], m_lastExitPoint))
        {
            bool foundValid = false;
            for (int altIdx : laterallyBalanced)
            {
                if (altIdx != chosenIdx && ValidatePatternTraversal(m_patterns[altIdx], m_lastExitPoint))
                {
                    chosenIdx = altIdx;
                    foundValid = true;
                    break;
                }
            }

            if (!foundValid)
            {
                for (int altIdx : matching)
                {
                    if (altIdx != chosenIdx && ValidatePatternTraversal(m_patterns[altIdx], m_lastExitPoint))
                    {
                        chosenIdx = altIdx;
                        foundValid = true;
                        break;
                    }
                }
            }

            if (!foundValid)
            {
                chosenIdx = matching.empty() ? 0 : matching.front();
            }
        }

        // 5. Update repetition avoidance history
        m_recentPatternsHistory.push_back(chosenIdx);
        while (m_recentPatternsHistory.size() > static_cast<size_t>(m_config.Difficulty.RepetitionAvoidanceWindow))
        {
            m_recentPatternsHistory.pop_front();
        }

        m_lastPatternIndex = chosenIdx;
        return chosenIdx;
    }

    void LevelManager::CreatePlatformDecorations(
        const PlatformBox& plat,
        int difficulty,
        int segmentIndex,
        size_t platformIndex,
        std::vector<std::shared_ptr<Aion::Object3D>>& decorObjs)
    {
        if (plat.IsSolidObstacle || plat.IsMoving || plat.IsPeriodicHazard || std::string_view(plat.Name).find("Start") != std::string_view::npos)
        {
            return;
        }

        float minDim = std::min(plat.Dimensions.x, plat.Dimensions.z);
        // Narrow precision platforms: strictly unobstructed for gameplay readability
        if (minDim < 2.2f)
        {
            return;
        }

        float topY = plat.GetTopSurfaceY();
        uint32_t seed = static_cast<uint32_t>(segmentIndex * 7919 + platformIndex * 104729 + 42);
        std::mt19937 platRng(seed);
        std::uniform_real_distribution<float> rotDist(0.0f, 360.0f);
        std::uniform_real_distribution<float> scaleJitter(0.92f, 1.08f);

        auto addPlatDecor = [&](const std::shared_ptr<Aion::Model>& model,
                                const Aion::Vector3& pos,
                                float scale,
                                float rotY,
                                const std::string& name) {
            auto d = CreateDecorObject(model, pos, scale, rotY, name);
            if (d) decorObjs.push_back(d);
        };

        // Determine corner flank offset (leaving landing center clear)
        // 4 corner flanks: NW, NE, SW, SE
        const float offX = 0.36f * plat.Dimensions.x;
        const float offZ = 0.36f * plat.Dimensions.z;
        const Aion::Vector3 cornerOffsets[4] = {
            Aion::Vector3(-offX, 0.0f,  offZ), // NW
            Aion::Vector3( offX, 0.0f,  offZ), // NE
            Aion::Vector3(-offX, 0.0f, -offZ), // SW
            Aion::Vector3( offX, 0.0f, -offZ)  // SE
        };

        int chosenCorner = static_cast<int>((seed + platformIndex) % 4);
        Aion::Vector3 primaryPos = plat.Position + cornerOffsets[chosenCorner];
        primaryPos.y = topY;

        // Large platforms (>= 3.2m): 6 distinct multi-object compositions
        if (minDim >= 3.2f)
        {
            const int numLargeCompositions = 6;
            std::vector<int> candidates;
            candidates.reserve(numLargeCompositions);
            for (int c = 0; c < numLargeCompositions; ++c)
            {
                if (c != m_lastDecorCompositionIndex)
                {
                    candidates.push_back(c);
                }
            }
            if (candidates.empty())
            {
                for (int c = 0; c < numLargeCompositions; ++c) candidates.push_back(c);
            }

            std::uniform_int_distribution<size_t> compDist(0, candidates.size() - 1);
            int chosenComp = candidates[compDist(platRng)];
            m_lastDecorCompositionIndex = chosenComp;

            float baseRot = rotDist(platRng);

            switch (chosenComp)
            {
                case 0: // Composition 0: Mushroom Grove
                {
                    if (difficulty <= 3) // Grassland
                    {
                        addPlatDecor(m_pkMushroomsModel, primaryPos, 0.95f * scaleJitter(platRng), baseRot, "Decor_Mushrooms");
                        Aion::Vector3 subPos = primaryPos + Aion::Vector3(0.32f, 0.0f, -0.28f);
                        addPlatDecor(m_pkPlantModel, subPos, 0.82f * scaleJitter(platRng), baseRot + 45.0f, "Decor_MushroomPlant");
                    }
                    else // Snow
                    {
                        addPlatDecor(m_pkRocksModel, primaryPos, 0.95f * scaleJitter(platRng), baseRot, "Decor_SnowBoulders");
                        Aion::Vector3 subPos = primaryPos + Aion::Vector3(0.32f, 0.0f, -0.28f);
                        addPlatDecor(m_mfStonesModel, subPos, 0.85f * scaleJitter(platRng), baseRot + 60.0f, "Decor_SnowStones");
                    }
                    break;
                }
                case 1: // Composition 1: Adventurer Supply Cache
                {
                    if (difficulty <= 3) // Grassland
                    {
                        addPlatDecor(m_pkCrateModel, primaryPos, 0.85f * scaleJitter(platRng), baseRot, "Decor_Crate");
                        Aion::Vector3 subPos = primaryPos + Aion::Vector3(-0.30f, 0.0f, 0.28f);
                        addPlatDecor(m_pkFenceBrokenModel, subPos, 0.80f * scaleJitter(platRng), baseRot + 90.0f, "Decor_BrokenFence");
                    }
                    else // Snow
                    {
                        addPlatDecor(m_pkCrateModel, primaryPos, 0.85f * scaleJitter(platRng), baseRot, "Decor_SnowCrate");
                        Aion::Vector3 subPos = primaryPos + Aion::Vector3(-0.30f, 0.0f, 0.28f);
                        addPlatDecor(m_pkRocksModel, subPos, 0.78f * scaleJitter(platRng), baseRot + 45.0f, "Decor_SnowRocks");
                    }
                    break;
                }
                case 2: // Composition 2: Shrubbery & Wildflowers
                {
                    if (difficulty <= 3) // Grassland
                    {
                        addPlatDecor(m_pkPlantModel, primaryPos, 1.05f * scaleJitter(platRng), baseRot, "Decor_Bush");
                        Aion::Vector3 subPos = primaryPos + Aion::Vector3(0.28f, 0.0f, 0.32f);
                        addPlatDecor(m_pkFlowersTallModel, subPos, 0.90f * scaleJitter(platRng), baseRot + 120.0f, "Decor_TallFlowers");
                    }
                    else // Snow
                    {
                        addPlatDecor(m_pkTreePineSnowSmallModel, primaryPos, 0.72f * scaleJitter(platRng), baseRot, "Decor_SnowPineSmall");
                        Aion::Vector3 subPos = primaryPos + Aion::Vector3(0.28f, 0.0f, 0.32f);
                        addPlatDecor(m_mfStonesModel, subPos, 0.82f * scaleJitter(platRng), baseRot + 45.0f, "Decor_SnowPineStones");
                    }
                    break;
                }
                case 3: // Composition 3: Rustic Trail Fence Border
                {
                    float fenceRot = (chosenCorner % 2 == 0) ? 0.0f : 90.0f;
                    if (difficulty <= 3) // Grassland
                    {
                        addPlatDecor(m_pkFenceLowModel, primaryPos, 0.85f * scaleJitter(platRng), fenceRot, "Decor_FenceLow");
                        Aion::Vector3 subPos = primaryPos + Aion::Vector3(-0.32f, 0.0f, -0.22f);
                        addPlatDecor(m_pkGrassModel, subPos, 0.90f * scaleJitter(platRng), baseRot, "Decor_FenceGrass");
                    }
                    else // Snow
                    {
                        addPlatDecor(m_pkFenceBrokenModel, primaryPos, 0.85f * scaleJitter(platRng), fenceRot, "Decor_SnowFence");
                        Aion::Vector3 subPos = primaryPos + Aion::Vector3(-0.32f, 0.0f, -0.22f);
                        addPlatDecor(m_pkRocksModel, subPos, 0.75f * scaleJitter(platRng), baseRot, "Decor_SnowFenceRock");
                    }
                    break;
                }
                case 4: // Composition 4: Boulder & Fungi Outcrop
                {
                    if (difficulty <= 3) // Grassland
                    {
                        addPlatDecor(m_pkRocksModel, primaryPos, 0.95f * scaleJitter(platRng), baseRot, "Decor_Boulder");
                        Aion::Vector3 subPos = primaryPos + Aion::Vector3(0.32f, 0.0f, -0.28f);
                        addPlatDecor(m_pkMushroomsModel, subPos, 0.82f * scaleJitter(platRng), baseRot + 75.0f, "Decor_BoulderMushrooms");
                    }
                    else // Snow
                    {
                        addPlatDecor(m_pkRocksModel, primaryPos, 1.0f * scaleJitter(platRng), baseRot, "Decor_SnowBoulder");
                        Aion::Vector3 subPos = primaryPos + Aion::Vector3(0.32f, 0.0f, -0.28f);
                        addPlatDecor(m_mfRocksLowModel, subPos, 0.80f * scaleJitter(platRng), baseRot + 50.0f, "Decor_SnowBoulderLow");
                    }
                    break;
                }
                case 5: // Composition 5: Sentinel Mini-Pine
                default:
                {
                    if (difficulty <= 3) // Grassland
                    {
                        addPlatDecor(m_pkTreePineSmallModel, primaryPos, 0.70f * scaleJitter(platRng), baseRot, "Decor_MiniPine");
                        Aion::Vector3 subPos = primaryPos + Aion::Vector3(-0.28f, 0.0f, 0.30f);
                        addPlatDecor(m_mfStonesModel, subPos, 0.80f * scaleJitter(platRng), baseRot + 90.0f, "Decor_MiniPineStones");
                    }
                    else // Snow
                    {
                        addPlatDecor(m_pkTreePineSnowSmallModel, primaryPos, 0.70f * scaleJitter(platRng), baseRot, "Decor_SnowMiniPine");
                        Aion::Vector3 subPos = primaryPos + Aion::Vector3(-0.28f, 0.0f, 0.30f);
                        addPlatDecor(m_mfStonesModel, subPos, 0.80f * scaleJitter(platRng), baseRot + 90.0f, "Decor_SnowMiniPineStones");
                    }
                    break;
                }
            }

            // For extra-large platforms (>= 3.8m), optionally add a subtle opposite corner accent
            if (minDim >= 3.8f && (seed % 3) != 0)
            {
                int oppCorner = (chosenCorner + 2) % 4;
                Aion::Vector3 oppPos = plat.Position + cornerOffsets[oppCorner];
                oppPos.y = topY;
                float oppRot = rotDist(platRng);

                if (difficulty <= 3)
                {
                    std::shared_ptr<Aion::Model> oppModel = ((seed % 2) == 0) ? m_pkFlowersModel : m_pkGrassModel;
                    addPlatDecor(oppModel, oppPos, 0.85f * scaleJitter(platRng), oppRot, "Decor_PlatOppCorner");
                }
                else
                {
                    addPlatDecor(m_pkStonesModel, oppPos, 0.80f * scaleJitter(platRng), oppRot, "Decor_PlatSnowOppCorner");
                }
            }
        }
        else // Standard platforms (2.2m <= minDim < 3.2m): 5 distinct moderate compositions
        {
            const int numStdCompositions = 5;
            std::vector<int> candidates;
            candidates.reserve(numStdCompositions);
            for (int c = 0; c < numStdCompositions; ++c)
            {
                if (c != m_lastDecorCompositionIndex)
                {
                    candidates.push_back(c);
                }
            }
            if (candidates.empty())
            {
                for (int c = 0; c < numStdCompositions; ++c) candidates.push_back(c);
            }

            std::uniform_int_distribution<size_t> compDist(0, candidates.size() - 1);
            int chosenComp = candidates[compDist(platRng)];
            m_lastDecorCompositionIndex = chosenComp;

            float baseRot = rotDist(platRng);

            switch (chosenComp)
            {
                case 0: // Composition 0: Mushroom Accent
                {
                    if (difficulty <= 3)
                    {
                        addPlatDecor(m_pkMushroomsModel, primaryPos, 0.85f * scaleJitter(platRng), baseRot, "Decor_StdMushrooms");
                    }
                    else
                    {
                        addPlatDecor(m_pkRocksModel, primaryPos, 0.80f * scaleJitter(platRng), baseRot, "Decor_StdSnowRock");
                    }
                    break;
                }
                case 1: // Composition 1: Lush Bush Accent
                {
                    if (difficulty <= 3)
                    {
                        addPlatDecor(m_pkPlantModel, primaryPos, 0.88f * scaleJitter(platRng), baseRot, "Decor_StdBush");
                    }
                    else
                    {
                        addPlatDecor(m_mfStonesModel, primaryPos, 0.85f * scaleJitter(platRng), baseRot, "Decor_StdSnowStones");
                    }
                    break;
                }
                case 2: // Composition 2: Broken Wooden Fence
                {
                    if (difficulty <= 3)
                    {
                        addPlatDecor(m_pkFenceBrokenModel, primaryPos, 0.78f * scaleJitter(platRng), baseRot, "Decor_StdFence");
                    }
                    else
                    {
                        addPlatDecor(m_pkFenceBrokenModel, primaryPos, 0.78f * scaleJitter(platRng), baseRot, "Decor_StdSnowFence");
                    }
                    break;
                }
                case 3: // Composition 3: Wildflowers / Low Flora
                {
                    if (difficulty <= 3)
                    {
                        addPlatDecor(m_pkFlowersTallModel, primaryPos, 0.85f * scaleJitter(platRng), baseRot, "Decor_StdFlowers");
                    }
                    else
                    {
                        addPlatDecor(m_mfRocksLowModel, primaryPos, 0.78f * scaleJitter(platRng), baseRot, "Decor_StdRocksLow");
                    }
                    break;
                }
                case 4: // Composition 4: Light Accent / Clean Surface
                default:
                {
                    if ((seed % 2) == 0)
                    {
                        if (difficulty <= 3)
                        {
                            addPlatDecor(m_pkGrassModel, primaryPos, 0.85f * scaleJitter(platRng), baseRot, "Decor_StdGrass");
                        }
                        else
                        {
                            addPlatDecor(m_pkStonesModel, primaryPos, 0.78f * scaleJitter(platRng), baseRot, "Decor_StdStones");
                        }
                    }
                    // Else: clean unadorned surface for natural breathing room
                    break;
                }
            }
        }
    }

    void LevelManager::SpawnSegmentScenery(
        int segmentIndex,
        int difficulty,
        const Aion::Vector3& entryPoint,
        const Aion::Vector3& exitPoint,
        std::vector<std::shared_ptr<Aion::Object3D>>& decorObjs)
    {
        Aion::Vector3 forward = exitPoint - entryPoint;
        float lenXZ = std::sqrt(forward.x * forward.x + forward.z * forward.z);
        if (lenXZ < 1.0f) lenXZ = 1.0f;
        Aion::Vector3 fwdNorm = Aion::Vector3(forward.x / lenXZ, 0.0f, forward.z / lenXZ);
        // Perpendicular horizontal right vector
        Aion::Vector3 rightNorm = Aion::Vector3(fwdNorm.z, 0.0f, -fwdNorm.x);

        uint32_t seed = static_cast<uint32_t>(segmentIndex * 1337 + difficulty * 997);
        std::mt19937 rng(seed);
        std::uniform_real_distribution<float> distOffset(5.5f, 8.5f);
        std::uniform_real_distribution<float> distY(-1.8f, -0.6f);
        std::uniform_real_distribution<float> distScale(0.85f, 1.25f);
        std::uniform_real_distribution<float> distRot(0.0f, 360.0f);

        auto addScenery = [&](const std::shared_ptr<Aion::Model>& model, const Aion::Vector3& pos, float scale, float rotY, const std::string& name) {
            auto d = CreateDecorObject(model, pos, scale, rotY, name);
            if (d) decorObjs.push_back(d);
        };

        // --- Cluster 1: Right-flank scenery (~30% along segment) ---
        {
            float t = 0.30f;
            float rDist = distOffset(rng);
            Aion::Vector3 basePos = entryPoint + fwdNorm * (lenXZ * t) + rightNorm * rDist;
            basePos.y = entryPoint.y + distY(rng);

            if (difficulty <= 3) // Grassland / Forest
            {
                addScenery(m_mfRocksHighModel, basePos, 1.2f, distRot(rng), "Scenery_RockR");
                Aion::Vector3 treePos1 = basePos + Aion::Vector3(0.2f, 0.5f, 0.1f);
                addScenery(m_mfTreeHighModel, treePos1, distScale(rng), distRot(rng), "Scenery_TreeHighR");
                Aion::Vector3 plantPos = basePos + Aion::Vector3(-0.5f, 0.5f, 0.4f);
                addScenery(m_mfPlantModel, plantPos, 1.0f, distRot(rng), "Scenery_PlantR");
            }
            else // Snow / Mountain
            {
                addScenery(m_mfRocksHighModel, basePos, 1.4f, distRot(rng), "Scenery_SnowCragR");
                Aion::Vector3 pinePos = basePos + Aion::Vector3(0.2f, 0.6f, 0.0f);
                addScenery(m_pkTreePineSnowModel, pinePos, distScale(rng), distRot(rng), "Scenery_SnowPineR");
                Aion::Vector3 rockPos = basePos + Aion::Vector3(-0.4f, 0.6f, 0.3f);
                addScenery(m_pkRocksModel, rockPos, 0.9f, distRot(rng), "Scenery_SnowRocksR");
            }
        }

        // --- Cluster 2: Left-flank scenery (~70% along segment, sitting in background) ---
        {
            float t = 0.70f;
            float lDist = distOffset(rng);
            Aion::Vector3 basePos = entryPoint + fwdNorm * (lenXZ * t) - rightNorm * lDist;
            basePos.y = entryPoint.y + distY(rng);

            if (difficulty <= 3) // Grassland / Forest
            {
                addScenery(m_mfRocksLowModel, basePos, 1.3f, distRot(rng), "Scenery_RockL");
                Aion::Vector3 treePos = basePos + Aion::Vector3(-0.2f, 0.3f, -0.2f);
                addScenery(m_mfTreeModel, treePos, distScale(rng), distRot(rng), "Scenery_TreeL");
                Aion::Vector3 stonePos = basePos + Aion::Vector3(0.5f, 0.3f, 0.3f);
                addScenery(m_mfStonesModel, stonePos, 1.0f, distRot(rng), "Scenery_StonesL");
            }
            else // Snow / Mountain
            {
                addScenery(m_mfRocksHighModel, basePos, 1.5f, distRot(rng), "Scenery_SnowCragL");
                Aion::Vector3 snowTreePos = basePos + Aion::Vector3(0.0f, 0.6f, 0.0f);
                addScenery(m_pkTreePineSnowSmallModel, snowTreePos, distScale(rng), distRot(rng), "Scenery_SnowPineSmallL");
                Aion::Vector3 rockPos = basePos + Aion::Vector3(0.5f, 0.6f, -0.4f);
                addScenery(m_pkRocksModel, rockPos, 1.1f, distRot(rng), "Scenery_SnowRocksL");
            }
        }

        // --- Cluster 3: Distant environmental silhouette (every 2nd segment) ---
        if (segmentIndex % 2 == 1)
        {
            float distLateral = (segmentIndex % 4 == 1) ? 12.0f : -12.0f;
            Aion::Vector3 distPos = entryPoint + fwdNorm * (lenXZ * 0.5f) + rightNorm * distLateral;
            distPos.y = entryPoint.y - 2.5f;

            if (difficulty <= 3)
            {
                addScenery(m_mfRocksHighModel, distPos, 2.2f, distRot(rng), "Scenery_DistantMountain");
                Aion::Vector3 dTreePos = distPos + Aion::Vector3(0.0f, 1.2f, 0.0f);
                addScenery(m_mfTreeHighModel, dTreePos, 1.5f, distRot(rng), "Scenery_DistantTree");
            }
            else
            {
                addScenery(m_mfRocksHighModel, distPos, 2.8f, distRot(rng), "Scenery_DistantSnowPeak");
                Aion::Vector3 dPinePos = distPos + Aion::Vector3(0.0f, 1.5f, 0.0f);
                addScenery(m_pkTreePineSnowModel, dPinePos, 1.6f, distRot(rng), "Scenery_DistantSnowPine");
            }
        }
    }

    std::shared_ptr<LevelSegment> LevelManager::SpawnNextSegment(int forcePatternIndex)
    {
        if (m_patterns.empty())
        {
            return nullptr;
        }

        int currentDiff = CalculateCurrentDifficulty();

        // Check breathing room pacing: occasionally generate an easier recovery segment
        bool isBreathingRoom = (m_segmentsGenerated > 0) &&
                               (m_config.Difficulty.EasierSegmentInterval > 0) &&
                               (m_segmentsGenerated % m_config.Difficulty.EasierSegmentInterval == 0) &&
                               (currentDiff > 1);

        int targetDiff = isBreathingRoom ? std::max(1, currentDiff - 1) : currentDiff;

        int patternIdx = forcePatternIndex;
        if (patternIdx >= 0 && patternIdx < static_cast<int>(m_patterns.size()))
        {
            targetDiff = m_patterns[patternIdx].MinDifficulty;
        }
        else
        {
            patternIdx = SelectNextPatternIndex(targetDiff);
        }

        const auto& basePattern = m_patterns[patternIdx];
        SegmentPattern pattern = basePattern;

        // Controlled subtle procedural variation per segment instantiation
        // Modulate intermediate platform dimensions slightly to enhance endless variety
        if (pattern.LocalPlatforms.size() > 1)
        {
            SegmentPattern candidatePattern = pattern;
            std::uniform_real_distribution<float> dimVar(-0.10f, 0.10f);

            for (size_t i = 1; i < candidatePattern.LocalPlatforms.size(); ++i)
            {
                auto& plat = candidatePattern.LocalPlatforms[i];
                if (plat.IsSolidObstacle)
                {
                    continue;
                }

                float newDimX = plat.Dimensions.x + dimVar(m_rng);
                float newDimZ = plat.Dimensions.z + dimVar(m_rng);

                float minDim = m_config.Difficulty.MinPlatformDimension;
                if (targetDiff == 1)
                {
                    minDim = std::max(minDim, 3.6f);
                }
                else if (targetDiff == 2)
                {
                    minDim = std::max(minDim, 2.6f);
                }
                else if (targetDiff == 3)
                {
                    minDim = std::max(minDim, 2.2f);
                }
                else if (targetDiff == 4)
                {
                    minDim = std::max(minDim, 1.8f);
                }
                else
                {
                    minDim = std::max(minDim, 1.4f);
                }

                newDimX = std::max(minDim, newDimX);
                newDimZ = std::max(minDim, newDimZ);

                plat.Dimensions.x = newDimX;
                plat.Dimensions.z = newDimZ;
            }

            if (ValidatePatternTraversal(candidatePattern, m_lastExitPoint))
            {
                pattern = std::move(candidatePattern);
            }
        }
        int segmentIndex = static_cast<int>(m_segmentsGenerated);

        // Compute world offset so that segment's LocalEntryPoint aligns with m_lastExitPoint
        Aion::Vector3 worldOffset = m_lastExitPoint - pattern.LocalEntryPoint;
        Aion::Vector3 worldEntryPoint = m_lastExitPoint;
        Aion::Vector3 worldExitPoint = worldOffset + pattern.LocalExitPoint;

        std::vector<PlatformBox> worldPlatforms;
        worldPlatforms.reserve(pattern.LocalPlatforms.size());

        std::vector<std::shared_ptr<Aion::Object3D>> sceneObjs;
        sceneObjs.reserve(pattern.LocalPlatforms.size());

        std::vector<std::shared_ptr<Aion::Object3D>> decorObjs;

        for (size_t i = 0; i < pattern.LocalPlatforms.size(); ++i)
        {
            PlatformBox plat = pattern.LocalPlatforms[i];
            plat.Position = plat.Position + worldOffset;
            plat.BasePosition = plat.Position;

            std::string objName = "Seg" + std::to_string(segmentIndex) + "_" + plat.Name;

            auto platObj = CreatePlatformObject(plat, targetDiff, objName);
            if (m_scene)
            {
                m_scene->Add(platObj);
            }
            sceneObjs.push_back(platObj);
            worldPlatforms.push_back(plat);

            // Platform Top Decorative Accents (Visual Only, no collision)
            CreatePlatformDecorations(plat, targetDiff, segmentIndex, i, decorObjs);
        }

        // Flanking Environmental Scenery
        SpawnSegmentScenery(segmentIndex, targetDiff, worldEntryPoint, worldExitPoint, decorObjs);

        m_lastExitPoint = worldExitPoint;

        std::string segmentName = "Segment_" + std::to_string(segmentIndex) + "_" + pattern.Name;
        auto segment = std::make_shared<LevelSegment>(
            segmentIndex,
            std::move(segmentName),
            targetDiff,
            worldEntryPoint,
            worldExitPoint,
            std::move(worldPlatforms),
            std::move(sceneObjs),
            std::move(decorObjs));

        m_activeSegments.push_back(segment);
        m_segmentsGenerated++;

        float segLength = segment->GetLengthXZ();
        float prevCumulative = m_segmentCumulativeStarts.empty() ? 0.0f
            : (m_segmentCumulativeStarts.back() + m_segmentLengths.back());
        m_segmentLengths.push_back(segLength);
        m_segmentCumulativeStarts.push_back(prevCumulative);

        // Register level checkpoint on the first valid platform of the level
        int firstSegOfLevel = (targetDiff - m_config.Difficulty.InitialDifficulty) * m_config.Difficulty.SegmentsPerDifficultyLevel;
        if (targetDiff > m_currentCheckpoint.Level && segmentIndex >= firstSegOfLevel && !segment->GetPlatforms().empty())
        {
            for (const auto& p : segment->GetPlatforms())
            {
                if (!p.IsSolidObstacle)
                {
                    Aion::Vector3 spawnPos{p.Position.x, p.GetTopSurfaceY() + m_config.Player.HalfExtents.y, p.Position.z};
                    RegisterCheckpoint(targetDiff, segmentIndex, spawnPos, prevCumulative);
                    break;
                }
            }
        }

        return segment;
    }

    void LevelManager::RecycleSegment(const std::shared_ptr<LevelSegment>& segment)
    {
        if (!segment)
        {
            return;
        }

        for (const auto& obj : segment->GetSceneObjects())
        {
            if (m_scene && obj)
            {
                m_scene->RemoveRoot(obj);
            }
        }
        for (const auto& obj : segment->GetDecorationObjects())
        {
            if (m_scene && obj)
            {
                m_scene->RemoveRoot(obj);
            }
        }
    }

    void LevelManager::RebuildActivePlatforms()
    {
        m_activePlatforms.clear();
        size_t total = 0;
        for (const auto& seg : m_activeSegments)
        {
            total += seg->GetPlatforms().size();
        }
        m_activePlatforms.reserve(total);

        for (const auto& seg : m_activeSegments)
        {
            for (const auto& plat : seg->GetPlatforms())
            {
                m_activePlatforms.push_back(plat);
            }
        }
    }

    size_t LevelManager::GetActiveSceneObjectsCount() const
    {
        size_t count = 0;
        for (const auto& seg : m_activeSegments)
        {
            count += seg->GetSceneObjects().size();
        }
        return count;
    }

    size_t LevelManager::GetActiveDecorationObjectsCount() const
    {
        size_t count = 0;
        for (const auto& seg : m_activeSegments)
        {
            count += seg->GetDecorationObjects().size();
        }
        return count;
    }

    bool LevelManager::Update(const Aion::Vector3& playerPosition)
    {
        bool changed = false;

        // 1. Locate player's current segment
        for (const auto& seg : m_activeSegments)
        {
            if (seg->ContainsPoint(playerPosition))
            {
                if (seg->GetIndex() > m_currentSegmentIndex)
                {
                    m_currentSegmentIndex = seg->GetIndex();

                    int newDiff = CalculateCurrentDifficulty();
                    if (newDiff != m_lastReportedDifficulty)
                    {
                        std::cout << "[Difficulty] Completed segment " << m_currentSegmentIndex
                                  << " -> Advanced to Level " << newDiff
                                  << " (" << GetDifficultyStageName(newDiff) << ")!" << std::endl;
                        m_lastReportedDifficulty = newDiff;
                    }
                }
                break;
            }
        }

        // 2. Compute stable internal forward progress
        if (m_currentSegmentIndex >= 0 && m_currentSegmentIndex < static_cast<int>(m_segmentCumulativeStarts.size()))
        {
            float baseDist = m_segmentCumulativeStarts[m_currentSegmentIndex];
            float segLen = m_segmentLengths[m_currentSegmentIndex];

            for (const auto& seg : m_activeSegments)
            {
                if (seg->GetIndex() == m_currentSegmentIndex)
                {
                    float t = seg->ProjectPointXZ(playerPosition);
                    float currentSegDistance = baseDist + t * segLen;
                    if (currentSegDistance > m_forwardProgress)
                    {
                        m_forwardProgress = currentSegDistance;
                    }
                    break;
                }
            }
        }

        // 3. Endless Spawning: Keep UpcomingSegmentsAhead available ahead of current progress
        int upcomingNeeded = m_currentSegmentIndex + m_config.Endless.UpcomingSegmentsAhead;
        while (static_cast<int>(m_segmentsGenerated) <= upcomingNeeded)
        {
            SpawnNextSegment();
            changed = true;
        }

        // Also spawn if player gets close to the furthest generated exit
        if ((m_lastExitPoint - playerPosition).Length() < m_config.Endless.GenerationDistanceThreshold)
        {
            SpawnNextSegment();
            changed = true;
        }

        // 4. Safe Recycling: Remove segments sufficiently far behind player, preserving active checkpoint
        int minSegmentToKeep = std::min(m_currentSegmentIndex - m_config.Endless.SegmentsRetainedBehind, m_currentCheckpoint.SegmentIndex);
        while (!m_activeSegments.empty() && m_activeSegments.front()->GetIndex() < minSegmentToKeep)
        {
            auto oldest = m_activeSegments.front();

            // Safety guard: Never remove a segment while player is still using it or reasonably landing on it
            if (oldest->ContainsPoint(playerPosition, 1.5f))
            {
                break;
            }

            RecycleSegment(oldest);
            m_activeSegments.pop_front();
            changed = true;
        }

        if (changed)
        {
            RebuildActivePlatforms();
        }

        return changed;
    }

    void LevelManager::UpdateMovingObjects(float deltaTime, const Aion::Vector3& playerPos, Aion::Vector3& carriedDelta)
    {
        carriedDelta = Aion::Vector3{0.0f, 0.0f, 0.0f};
        bool anyMoved = false;
        constexpr float footRadius = 0.35f;

        for (auto& segment : m_activeSegments)
        {
            auto& platforms = segment->GetPlatformsMutable();
            const auto& sceneObjs = segment->GetSceneObjects();

            for (size_t i = 0; i < platforms.size(); ++i)
            {
                auto& plat = platforms[i];
                auto obj = (i < sceneObjs.size()) ? sceneObjs[i] : nullptr;

                if (plat.IsMoving)
                {
                    Aion::Vector3 oldPos = plat.Position;
                    plat.MovePhase += plat.MoveSpeed * deltaTime;
                    float offset = std::sin(plat.MovePhase) * plat.MoveDistance;
                    plat.Position = plat.BasePosition + plat.MoveAxis * offset;

                    if (deltaTime > 0.0001f)
                    {
                        plat.Velocity = (plat.Position - oldPos) / deltaTime;
                    }

                    if (obj)
                    {
                        obj->Transform.Position = Aion::Vector3(
                            plat.Position.x,
                            plat.Position.y - 0.5f * plat.Dimensions.y,
                            plat.Position.z);
                    }

                    // Check player carrying (if player is standing on this platform)
                    if (!plat.IsSolidObstacle)
                    {
                        Aion::Vector3 pMin = plat.GetMin();
                        Aion::Vector3 pMax = plat.GetMax();
                        float topY = plat.GetTopSurfaceY();
                        float playerBottomY = playerPos.y - m_config.Player.HalfExtents.y;

                        bool horizInside = (playerPos.x + footRadius >= pMin.x &&
                                            playerPos.x - footRadius <= pMax.x &&
                                            playerPos.z + footRadius >= pMin.z &&
                                            playerPos.z - footRadius <= pMax.z);
                        bool vertOnTop = (std::abs(playerBottomY - topY) <= 0.25f);

                        if (horizInside && vertOnTop)
                        {
                            carriedDelta = carriedDelta + (plat.Position - oldPos);
                        }
                    }

                    anyMoved = true;
                }

                if (plat.IsPeriodicHazard)
                {
                    plat.HazardTimer += deltaTime;
                    if (plat.HazardTimer >= plat.HazardPeriod)
                    {
                        plat.HazardTimer = std::fmod(plat.HazardTimer, plat.HazardPeriod);
                    }

                    bool isActive = (plat.HazardTimer < plat.HazardActiveTime);
                    plat.HazardIsActive = isActive;
                    plat.IsSolidObstacle = isActive;

                    if (obj)
                    {
                        if (isActive)
                        {
                            obj->Transform.Position = Aion::Vector3(
                                plat.Position.x,
                                plat.Position.y - 0.5f * plat.Dimensions.y,
                                plat.Position.z);
                        }
                        else
                        {
                            // Retract below platform surface so spikes are concealed
                            obj->Transform.Position = Aion::Vector3(
                                plat.Position.x,
                                plat.Position.y - 0.5f * plat.Dimensions.y - 1.5f,
                                plat.Position.z);
                        }
                    }

                    anyMoved = true;
                }
            }
        }

        if (anyMoved)
        {
            RebuildActivePlatforms();
        }
    }

    void LevelManager::ResetCheckpoint()
    {
        m_currentCheckpoint = Checkpoint{1, 0, m_config.Player.SpawnPosition, 0.0f, true};
    }

    void LevelManager::RegisterCheckpoint(int level, int segmentIndex, const Aion::Vector3& spawnPos, float forwardProgress)
    {
        if (level > m_currentCheckpoint.Level || !m_currentCheckpoint.IsValid)
        {
            m_currentCheckpoint.Level = level;
            m_currentCheckpoint.SegmentIndex = segmentIndex;
            m_currentCheckpoint.SpawnPosition = spawnPos;
            m_currentCheckpoint.ForwardProgress = forwardProgress;
            m_currentCheckpoint.IsValid = true;
            std::cout << "[LevelManager] Checkpoint Registered: Level " << level
                      << " at Segment " << segmentIndex
                      << " (Spawn: " << spawnPos.x << ", " << spawnPos.y << ", " << spawnPos.z << ")" << std::endl;
        }
    }

    void LevelManager::RestoreToCheckpoint(Aion::Vector3& outSpawnPos)
    {
        outSpawnPos = m_currentCheckpoint.SpawnPosition;
        m_currentSegmentIndex = std::max(m_currentSegmentIndex, m_currentCheckpoint.SegmentIndex);
        m_forwardProgress = std::max(m_forwardProgress, m_currentCheckpoint.ForwardProgress);
    }
} // namespace BlockJumper
