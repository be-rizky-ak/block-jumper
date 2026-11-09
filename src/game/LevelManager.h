#pragma once

#include <deque>
#include <memory>
#include <random>
#include <vector>

#include "Aion/Assets/Mesh.h"
#include "Aion/Math/Vector3.h"
#include "Aion/Scene/Object3D.h"
#include "Aion/Scene/Scene.h"
#include "game/GameConfig.h"
#include "game/LevelSegment.h"

namespace BlockJumper
{
    /**
     * @brief Manages procedural endless level generation, progression tracking,
     * and automatic segment recycling for Block Jumper.
     */
    class LevelManager
    {
    public:
        LevelManager() = default;
        ~LevelManager() = default;

        /**
         * @brief Initializes the level manager, registers segment patterns,
         * spawns the starting plaza (Segment 0), and creates the initial upcoming segments.
         */
        void Initialize(
            Aion::Scene* scene,
            const GameConfig& config,
            std::shared_ptr<Aion::Mesh> sharedCubeMesh);

        /**
         * @brief Evaluates player position, updates internal forward progress and current segment index,
         * dynamically spawns upcoming segments ahead, and recycles distant segments behind.
         *
         * @param playerPosition The player's current world coordinates.
         * @return true if the active platform list was modified.
         */
        bool Update(const Aion::Vector3& playerPosition);

        // Progress Tracking Telemetry
        int GetCurrentSegmentIndex() const { return m_currentSegmentIndex; }
        float GetForwardProgress() const { return m_forwardProgress; }
        size_t GetSegmentsGenerated() const { return m_segmentsGenerated; }
        size_t GetActiveSegmentsCount() const { return m_activeSegments.size(); }
        size_t GetActiveSceneObjectsCount() const;

        const std::vector<PlatformBox>& GetActivePlatforms() const { return m_activePlatforms; }
        const std::deque<std::shared_ptr<LevelSegment>>& GetActiveSegments() const { return m_activeSegments; }

        /**
         * @brief Spawns the next segment along the progression path.
         * @param forcePatternIndex If >= 0, forces that pattern index; otherwise picks with variety.
         */
        std::shared_ptr<LevelSegment> SpawnNextSegment(int forcePatternIndex = -1);

        /**
         * @brief Safely removes a segment from the active list and cleans up scene objects.
         */
        void RecycleSegment(const std::shared_ptr<LevelSegment>& segment);

    private:
        void InitializePatterns();
        void SpawnStartingPlatform();
        void RebuildActivePlatforms();
        int SelectNextPatternIndex();

    private:
        Aion::Scene* m_scene = nullptr;
        GameConfig m_config;
        std::shared_ptr<Aion::Mesh> m_cubeMesh;

        std::vector<SegmentPattern> m_patterns;
        std::deque<std::shared_ptr<LevelSegment>> m_activeSegments;
        std::vector<PlatformBox> m_activePlatforms;

        Aion::Vector3 m_lastExitPoint{0.0f};
        int m_currentSegmentIndex = 0;
        float m_forwardProgress = 0.0f;
        size_t m_segmentsGenerated = 0;
        int m_lastPatternIndex = -1;

        std::mt19937 m_rng{1337};
    };
} // namespace BlockJumper
