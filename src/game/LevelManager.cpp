#include "LevelManager.h"

#include <cmath>
#include <iostream>

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
        m_segmentsGenerated = 0;
        m_lastPatternIndex = -1;

        InitializePatterns();
        SpawnStartingPlatform();

        // Pre-generate configured upcoming segments ahead of starting platform
        for (int i = 0; i < m_config.Endless.UpcomingSegmentsAhead; ++i)
        {
            SpawnNextSegment();
        }

        RebuildActivePlatforms();

        std::cout << "[LevelManager] Initialized with " << m_activeSegments.size()
                  << " active segments (" << m_activePlatforms.size()
                  << " platforms total, " << m_patterns.size() << " segment patterns)." << std::endl;
    }

    void LevelManager::InitializePatterns()
    {
        m_patterns.clear();
        m_patterns.reserve(5);

        // -----------------------------------------------------------------
        // Pattern 0: SteppingStones (Basic stepping platforms with calibrated jump gaps)
        // -----------------------------------------------------------------
        {
            SegmentPattern p;
            p.Name = "SteppingStones";
            p.LocalEntryPoint = {0.0f, 0.0f, 0.0f};

            p.LocalPlatforms.push_back(PlatformBox{
                .Position = {0.0f, -0.4f, 0.0f},
                .Dimensions = {2.4f, 0.8f, 2.4f},
                .Color = {0.32f, 0.68f, 0.55f, 1.0f}, // Mint Teal
                .Metallic = 0.10f,
                .Roughness = 0.75f,
                .IsSolidObstacle = false,
                .Name = "Step_Mint"
            });

            p.LocalPlatforms.push_back(PlatformBox{
                .Position = {-2.9f, -0.2f, -2.2f},
                .Dimensions = {2.4f, 1.2f, 2.4f},
                .Color = {0.28f, 0.60f, 0.65f, 1.0f}, // Turquoise
                .Metallic = 0.12f,
                .Roughness = 0.70f,
                .IsSolidObstacle = false,
                .Name = "Step_Turquoise"
            });

            p.LocalPlatforms.push_back(PlatformBox{
                .Position = {-6.3f, -0.1f, -4.8f},
                .Dimensions = {2.8f, 1.0f, 2.8f},
                .Color = {0.28f, 0.65f, 0.48f, 1.0f}, // Emerald Mint
                .Metallic = 0.15f,
                .Roughness = 0.70f,
                .IsSolidObstacle = false,
                .Name = "Step_Emerald"
            });

            p.LocalExitPoint = {-9.3f, 0.4f, -7.4f};
            m_patterns.push_back(std::move(p));
        }

        // -----------------------------------------------------------------
        // Pattern 1: WideStraightaway (Wide platforms with small gaps)
        // -----------------------------------------------------------------
        {
            SegmentPattern p;
            p.Name = "WideStraightaway";
            p.LocalEntryPoint = {0.0f, 0.0f, 0.0f};

            p.LocalPlatforms.push_back(PlatformBox{
                .Position = {0.0f, -0.4f, 0.0f},
                .Dimensions = {3.8f, 0.8f, 3.8f},
                .Color = {0.25f, 0.52f, 0.50f, 1.0f}, // Slate Teal
                .Metallic = 0.15f,
                .Roughness = 0.70f,
                .IsSolidObstacle = false,
                .Name = "Straight_Teal1"
            });

            p.LocalPlatforms.push_back(PlatformBox{
                .Position = {-4.5f, -0.4f, -3.8f},
                .Dimensions = {3.6f, 0.8f, 3.6f},
                .Color = {0.20f, 0.45f, 0.58f, 1.0f}, // Deep Cyan
                .Metallic = 0.15f,
                .Roughness = 0.70f,
                .IsSolidObstacle = false,
                .Name = "Straight_Cyan"
            });

            p.LocalPlatforms.push_back(PlatformBox{
                .Position = {-9.0f, -0.4f, -7.6f},
                .Dimensions = {3.8f, 0.8f, 3.8f},
                .Color = {0.25f, 0.52f, 0.50f, 1.0f}, // Slate Teal
                .Metallic = 0.15f,
                .Roughness = 0.70f,
                .IsSolidObstacle = false,
                .Name = "Straight_Teal2"
            });

            p.LocalExitPoint = {-13.5f, 0.0f, -11.4f};
            m_patterns.push_back(std::move(p));
        }

        // -----------------------------------------------------------------
        // Pattern 2: ElevatedPlateau (Elevated platform sequence with obstacle hurdle)
        // -----------------------------------------------------------------
        {
            SegmentPattern p;
            p.Name = "ElevatedPlateau";
            p.LocalEntryPoint = {0.0f, 0.0f, 0.0f};

            p.LocalPlatforms.push_back(PlatformBox{
                .Position = {0.0f, -0.4f, 0.0f},
                .Dimensions = {2.6f, 0.8f, 2.6f},
                .Color = {0.32f, 0.68f, 0.55f, 1.0f}, // Mint Approach
                .Metallic = 0.10f,
                .Roughness = 0.75f,
                .IsSolidObstacle = false,
                .Name = "Plateau_Approach"
            });

            // Ascending Plateau (Top Y = 0.7, requires jump to climb)
            p.LocalPlatforms.push_back(PlatformBox{
                .Position = {-3.6f, -0.2f, -3.0f},
                .Dimensions = {4.2f, 1.8f, 4.2f},
                .Color = {0.88f, 0.48f, 0.35f, 1.0f}, // Terracotta Coral
                .Metallic = 0.15f,
                .Roughness = 0.65f,
                .IsSolidObstacle = false,
                .Name = "Plateau_Coral"
            });

            // Obstacle Hurdle across the Plateau (requires jumping over)
            p.LocalPlatforms.push_back(PlatformBox{
                .Position = {-3.6f, 1.1f, -3.0f},
                .Dimensions = {0.8f, 0.8f, 2.8f},
                .Color = {0.18f, 0.20f, 0.28f, 1.0f}, // Obsidian Obstacle
                .Metallic = 0.25f,
                .Roughness = 0.50f,
                .IsSolidObstacle = true,
                .Name = "Plateau_Hurdle"
            });

            // Landing Terrace (Top Y = 0.7)
            p.LocalPlatforms.push_back(PlatformBox{
                .Position = {-8.2f, -0.2f, -6.8f},
                .Dimensions = {3.4f, 1.8f, 3.4f},
                .Color = {0.95f, 0.75f, 0.25f, 1.0f}, // Golden Citadel
                .Metallic = 0.20f,
                .Roughness = 0.55f,
                .IsSolidObstacle = false,
                .Name = "Plateau_Gold"
            });

            p.LocalExitPoint = {-12.0f, 0.7f, -10.0f};
            m_patterns.push_back(std::move(p));
        }

        // -----------------------------------------------------------------
        // Pattern 3: LateralZigZag (Lateral zig-zag stepping columns)
        // -----------------------------------------------------------------
        {
            SegmentPattern p;
            p.Name = "LateralZigZag";
            p.LocalEntryPoint = {0.0f, 0.0f, 0.0f};

            p.LocalPlatforms.push_back(PlatformBox{
                .Position = {0.0f, -0.6f, 0.0f},
                .Dimensions = {2.4f, 1.2f, 2.4f},
                .Color = {0.90f, 0.42f, 0.48f, 1.0f}, // Rose Coral
                .Metallic = 0.12f,
                .Roughness = 0.70f,
                .IsSolidObstacle = false,
                .Name = "ZigZag_Center1"
            });

            // Steer Left relative to camera
            p.LocalPlatforms.push_back(PlatformBox{
                .Position = {-3.4f, -0.5f, 0.0f},
                .Dimensions = {2.2f, 1.4f, 2.2f},
                .Color = {0.92f, 0.70f, 0.22f, 1.0f}, // Golden Amber
                .Metallic = 0.15f,
                .Roughness = 0.65f,
                .IsSolidObstacle = false,
                .Name = "ZigZag_Left"
            });

            // Steer Right relative to camera
            p.LocalPlatforms.push_back(PlatformBox{
                .Position = {-4.2f, -0.5f, -4.6f},
                .Dimensions = {2.2f, 1.6f, 2.2f},
                .Color = {0.90f, 0.42f, 0.48f, 1.0f}, // Rose Coral
                .Metallic = 0.12f,
                .Roughness = 0.70f,
                .IsSolidObstacle = false,
                .Name = "ZigZag_Right"
            });

            // Return to central axis
            p.LocalPlatforms.push_back(PlatformBox{
                .Position = {-8.0f, -0.5f, -6.4f},
                .Dimensions = {3.2f, 1.2f, 3.2f},
                .Color = {0.28f, 0.65f, 0.48f, 1.0f}, // Emerald Mint
                .Metallic = 0.15f,
                .Roughness = 0.70f,
                .IsSolidObstacle = false,
                .Name = "ZigZag_Center2"
            });

            p.LocalExitPoint = {-11.8f, 0.1f, -9.6f};
            m_patterns.push_back(std::move(p));
        }

        // -----------------------------------------------------------------
        // Pattern 4: DescendingDrop (Descending platform sequence with one-way drop)
        // -----------------------------------------------------------------
        {
            SegmentPattern p;
            p.Name = "DescendingDrop";
            p.LocalEntryPoint = {0.0f, 0.0f, 0.0f};

            p.LocalPlatforms.push_back(PlatformBox{
                .Position = {0.0f, -0.8f, 0.0f},
                .Dimensions = {3.2f, 1.6f, 3.2f},
                .Color = {0.58f, 0.35f, 0.82f, 1.0f}, // Amethyst Violet
                .Metallic = 0.15f,
                .Roughness = 0.60f,
                .IsSolidObstacle = false,
                .Name = "Drop_Launch"
            });

            // One-Way Drop Landing (Drop of 1.8 units > max jump 1.5 units)
            p.LocalPlatforms.push_back(PlatformBox{
                .Position = {-4.4f, -2.2f, -3.6f},
                .Dimensions = {4.0f, 0.8f, 4.0f},
                .Color = {0.20f, 0.45f, 0.58f, 1.0f}, // Deep Cyan
                .Metallic = 0.15f,
                .Roughness = 0.70f,
                .IsSolidObstacle = false,
                .Name = "Drop_Landing"
            });

            p.LocalPlatforms.push_back(PlatformBox{
                .Position = {-8.6f, -2.1f, -7.2f},
                .Dimensions = {3.4f, 1.0f, 3.4f},
                .Color = {0.22f, 0.58f, 0.72f, 1.0f}, // Sky Cyan
                .Metallic = 0.15f,
                .Roughness = 0.65f,
                .IsSolidObstacle = false,
                .Name = "Drop_Recovery"
            });

            p.LocalExitPoint = {-12.4f, -1.6f, -10.4f};
            m_patterns.push_back(std::move(p));
        }
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
            auto platObj = std::make_shared<Aion::Object3D>(plat.Name);
            auto mat = std::make_shared<Aion::Material>();
            mat->BaseColor = plat.Color;
            mat->MetallicFactor = plat.Metallic;
            mat->RoughnessFactor = plat.Roughness;

            platObj->AddComponent<Aion::MeshRenderer>(m_cubeMesh, mat);
            platObj->Transform.Position = plat.Position;
            platObj->Transform.Scale = plat.Dimensions;

            if (m_scene)
            {
                m_scene->Add(platObj);
            }
            sceneObjs.push_back(platObj);
        }

        Aion::Vector3 entryPoint{0.0f, 0.0f, 0.0f};
        Aion::Vector3 exitPoint{-5.5f, 0.0f, -3.2f};
        m_lastExitPoint = exitPoint;

        auto startSegment = std::make_shared<LevelSegment>(
            0,
            "StartPlaza",
            entryPoint,
            exitPoint,
            std::move(startPlatforms),
            std::move(sceneObjs));

        m_activeSegments.push_back(startSegment);
        m_segmentsGenerated = 1;
    }

    int LevelManager::SelectNextPatternIndex()
    {
        if (m_patterns.empty())
        {
            return 0;
        }
        if (m_patterns.size() == 1)
        {
            m_lastPatternIndex = 0;
            return 0;
        }

        std::uniform_int_distribution<int> dist(0, static_cast<int>(m_patterns.size()) - 2);
        int offset = dist(m_rng);
        int nextIdx = (m_lastPatternIndex + 1 + offset) % static_cast<int>(m_patterns.size());
        m_lastPatternIndex = nextIdx;
        return nextIdx;
    }

    std::shared_ptr<LevelSegment> LevelManager::SpawnNextSegment(int forcePatternIndex)
    {
        if (m_patterns.empty())
        {
            return nullptr;
        }

        int patternIdx = forcePatternIndex;
        if (patternIdx < 0 || patternIdx >= static_cast<int>(m_patterns.size()))
        {
            patternIdx = SelectNextPatternIndex();
        }

        const auto& pattern = m_patterns[patternIdx];
        int segmentIndex = static_cast<int>(m_segmentsGenerated);

        // Compute world offset so that segment's LocalEntryPoint aligns with m_lastExitPoint
        Aion::Vector3 worldOffset = m_lastExitPoint - pattern.LocalEntryPoint;
        Aion::Vector3 worldEntryPoint = m_lastExitPoint;
        Aion::Vector3 worldExitPoint = worldOffset + pattern.LocalExitPoint;

        std::vector<PlatformBox> worldPlatforms;
        worldPlatforms.reserve(pattern.LocalPlatforms.size());

        std::vector<std::shared_ptr<Aion::Object3D>> sceneObjs;
        sceneObjs.reserve(pattern.LocalPlatforms.size());

        for (size_t i = 0; i < pattern.LocalPlatforms.size(); ++i)
        {
            PlatformBox plat = pattern.LocalPlatforms[i];
            plat.Position = plat.Position + worldOffset;

            std::string objName = "Seg" + std::to_string(segmentIndex) + "_" + plat.Name;

            auto platObj = std::make_shared<Aion::Object3D>(objName);
            auto mat = std::make_shared<Aion::Material>();
            mat->BaseColor = plat.Color;
            mat->MetallicFactor = plat.Metallic;
            mat->RoughnessFactor = plat.Roughness;

            platObj->AddComponent<Aion::MeshRenderer>(m_cubeMesh, mat);
            platObj->Transform.Position = plat.Position;
            platObj->Transform.Scale = plat.Dimensions;

            if (m_scene)
            {
                m_scene->Add(platObj);
            }
            sceneObjs.push_back(platObj);
            worldPlatforms.push_back(plat);
        }

        m_lastExitPoint = worldExitPoint;

        std::string segmentName = "Segment_" + std::to_string(segmentIndex) + "_" + pattern.Name;
        auto segment = std::make_shared<LevelSegment>(
            segmentIndex,
            std::move(segmentName),
            worldEntryPoint,
            worldExitPoint,
            std::move(worldPlatforms),
            std::move(sceneObjs));

        m_activeSegments.push_back(segment);
        m_segmentsGenerated++;

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
                }
                break;
            }
        }

        // 2. Compute stable internal forward progress
        float cumulativeDistance = 0.0f;
        for (const auto& seg : m_activeSegments)
        {
            if (seg->GetIndex() < m_currentSegmentIndex)
            {
                cumulativeDistance += seg->GetLength();
            }
            else if (seg->GetIndex() == m_currentSegmentIndex)
            {
                float t = seg->ProjectPoint(playerPosition);
                float currentSegDistance = cumulativeDistance + t * seg->GetLength();
                if (currentSegDistance > m_forwardProgress)
                {
                    m_forwardProgress = currentSegDistance;
                }
                break;
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

        // 4. Safe Recycling: Remove segments sufficiently far behind player
        int minSegmentToKeep = m_currentSegmentIndex - m_config.Endless.SegmentsRetainedBehind;
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
} // namespace BlockJumper
