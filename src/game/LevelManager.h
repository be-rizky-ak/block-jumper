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
    struct PlatformModelVariant
    {
        std::shared_ptr<Aion::Model> Model;
        float BaseWidth = 1.0f;
        float BaseDepth = 1.0f;
        float BaseHeight = 1.0f;
        bool CanRotateY = false;
        std::string Name;
    };

    /**
     * @brief Safe checkpoint registered at the first valid platform of each level.
     */
    struct Checkpoint
    {
        int Level = 1;
        int SegmentIndex = 0;
        Aion::Vector3 SpawnPosition{2.2f, 0.5f, 0.4f};
        float ForwardProgress = 0.0f;
        bool IsValid = true;
    };

    enum class IslandType
    {
        AncientGrove,
        CliffSanctuary,
        ExpeditionCamp,
        WildflowerRidge,
        DistantHorizon
    };

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
        size_t GetActiveDecorationObjectsCount() const;

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

        /**
         * @brief Resets level progression and returns difficulty to initial state.
         */
        void Reset();

        /**
         * @brief Resets level progression to a specific difficulty stage checkpoint.
         * Deterministically reconstructs the level route up to targetStage and returns the safe spawn position.
         */
        void ResetToStage(int targetStage, Aion::Vector3& outSpawnPos);

        // Difficulty Progression Telemetry & Validation
        int GetCurrentDifficulty() const;
        int GetCompletedSegments() const { return m_currentSegmentIndex; }
        size_t GetPatternCount() const { return m_patterns.size(); }
        const char* GetDifficultyStageName(int difficulty) const;
        bool ValidatePatternTraversal(const SegmentPattern& pattern, const Aion::Vector3& prevExitPoint) const;

        // Moving platforms & dynamic hazards simulation
        void UpdateMovingObjects(float deltaTime, const Aion::Vector3& playerPos, Aion::Vector3& carriedDelta);

        // Checkpoint Telemetry & Lifecycle
        const Checkpoint& GetCurrentCheckpoint() const { return m_currentCheckpoint; }
        int GetCurrentCheckpointLevel() const { return m_currentCheckpoint.Level; }
        const Aion::Vector3& GetCheckpointSpawnPosition() const { return m_currentCheckpoint.SpawnPosition; }
        bool HasActiveCheckpoint() const { return m_currentCheckpoint.IsValid; }
        void ResetCheckpoint();
        void RegisterCheckpoint(int level, int segmentIndex, const Aion::Vector3& spawnPos, float forwardProgress);
        void RestoreToCheckpoint(Aion::Vector3& outSpawnPos);

    private:
        void InitializePatterns();
        void LoadPlatformModels();
        void LoadEnvironmentModels();
        void SpawnStartingPlatform();
        void RebuildActivePlatforms();
        int CalculateCurrentDifficulty() const;
        int SelectNextPatternIndex(int targetDifficulty);
        PlatformModelVariant SelectPlatformModelVariant(
            const PlatformBox& plat,
            int difficulty);
        std::shared_ptr<Aion::Object3D> CreatePlatformObject(
            const PlatformBox& plat,
            int difficulty,
            const std::string& objName);
        void CreatePlatformDecorations(
            const PlatformBox& plat,
            int difficulty,
            int segmentIndex,
            size_t platformIndex,
            std::vector<std::shared_ptr<Aion::Object3D>>& decorObjs);
        void SpawnSegmentSideDecorations(
            int segmentIndex,
            int difficulty,
            const Aion::Vector3& entryPoint,
            const Aion::Vector3& exitPoint,
            std::vector<std::shared_ptr<Aion::Object3D>>& decorObjs);
        void SpawnFloatingIsland(
            const Aion::Vector3& centerPos,
            float baseScale,
            float rotYDegrees,
            IslandType type,
            int difficulty,
            int segmentIndex,
            std::vector<std::shared_ptr<Aion::Object3D>>& decorObjs);
        std::shared_ptr<Aion::Object3D> CreateDecorObject(
            const std::shared_ptr<Aion::Model>& model,
            const Aion::Vector3& position,
            float scale = 1.0f,
            float rotYDegrees = 0.0f,
            const std::string& name = "Decor");

    private:
        Aion::Scene* m_scene = nullptr;
        GameConfig m_config;
        std::shared_ptr<Aion::Mesh> m_cubeMesh;

        // Cached Kenney 3D platform and obstacle models for reuse
        std::shared_ptr<Aion::Model> m_grassLargeModel;
        std::shared_ptr<Aion::Model> m_grassStandardModel;
        std::shared_ptr<Aion::Model> m_grassNarrowModel;
        std::shared_ptr<Aion::Model> m_snowLargeModel;
        std::shared_ptr<Aion::Model> m_snowStandardModel;
        std::shared_ptr<Aion::Model> m_snowNarrowModel;
        std::shared_ptr<Aion::Model> m_spikeObstacleWideModel;
        std::shared_ptr<Aion::Model> m_spikeObstacleModel;
        std::shared_ptr<Aion::Model> m_movingPlatformModel;
        std::shared_ptr<Aion::Model> m_movingLargeModel;
        std::shared_ptr<Aion::Model> m_sawModel;

        // Cached Kenney Mini-Forest environmental nature models (strictly trees, rocks, flora)
        std::shared_ptr<Aion::Model> m_mfTreeModel;
        std::shared_ptr<Aion::Model> m_mfTreeHighModel;
        std::shared_ptr<Aion::Model> m_mfPlantModel;
        std::shared_ptr<Aion::Model> m_mfStonesModel;
        std::shared_ptr<Aion::Model> m_mfRocksLowModel;
        std::shared_ptr<Aion::Model> m_mfRocksHighModel;

        // Cached Kenney Platformer-Kit decorative nature models (strictly trees, rocks, flowers)
        std::shared_ptr<Aion::Model> m_pkGrassModel;
        std::shared_ptr<Aion::Model> m_pkFlowersModel;
        std::shared_ptr<Aion::Model> m_pkFlowersTallModel;
        std::shared_ptr<Aion::Model> m_pkPlantModel;
        std::shared_ptr<Aion::Model> m_pkRocksModel;
        std::shared_ptr<Aion::Model> m_pkStonesModel;
        std::shared_ptr<Aion::Model> m_pkMushroomsModel;
        std::shared_ptr<Aion::Model> m_pkTreeModel;
        std::shared_ptr<Aion::Model> m_pkTreePineModel;
        std::shared_ptr<Aion::Model> m_pkTreePineSmallModel;

        // Mini-Forest ground patch models
        std::shared_ptr<Aion::Model> m_mfPatchGrassModel;
        std::shared_ptr<Aion::Model> m_mfPatchDirtModel;

        // Snow / Mountain nature models
        std::shared_ptr<Aion::Model> m_pkTreePineSnowModel;
        std::shared_ptr<Aion::Model> m_pkTreePineSnowSmallModel;
        std::shared_ptr<Aion::Model> m_pkTreeSnowModel;

        // Decorative background platform structure models
        std::shared_ptr<Aion::Model> m_pkPlatformModel;

        // Large Floating Island foundation models (Platformer-Kit)
        std::shared_ptr<Aion::Model> m_pkGrassLargeTallModel;
        std::shared_ptr<Aion::Model> m_pkGrassOverhangTallModel;
        std::shared_ptr<Aion::Model> m_pkGrassOverhangLargeModel;
        std::shared_ptr<Aion::Model> m_pkGrassHexagonModel;
        std::shared_ptr<Aion::Model> m_pkGrassOverhangHexagonModel;
        std::shared_ptr<Aion::Model> m_pkSnowLargeTallModel;
        std::shared_ptr<Aion::Model> m_pkSnowOverhangTallModel;
        std::shared_ptr<Aion::Model> m_pkSnowHexagonModel;

        // Expanded platform model variant collections for Grass and Snow
        std::vector<PlatformModelVariant> m_grassLargeVariants;
        std::vector<PlatformModelVariant> m_grassStandardVariants;
        std::vector<PlatformModelVariant> m_grassNarrowVariants;

        std::vector<PlatformModelVariant> m_snowLargeVariants;
        std::vector<PlatformModelVariant> m_snowStandardVariants;
        std::vector<PlatformModelVariant> m_snowNarrowVariants;

        std::string m_lastSelectedModelName;

        std::vector<SegmentPattern> m_patterns;
        std::deque<std::shared_ptr<LevelSegment>> m_activeSegments;
        std::vector<PlatformBox> m_activePlatforms;

        Aion::Vector3 m_lastExitPoint{0.0f};
        int m_currentSegmentIndex = 0;
        float m_forwardProgress = 0.0f;
        std::vector<float> m_segmentLengths;
        std::vector<float> m_segmentCumulativeStarts;
        size_t m_segmentsGenerated = 0;
        int m_lastPatternIndex = -1;
        int m_lastDecorCompositionIndex = -1;
        std::deque<int> m_recentPatternsHistory;
        int m_lastReportedDifficulty = 1;

        Checkpoint m_currentCheckpoint;

        std::mt19937 m_rng{1337};
    };
} // namespace BlockJumper
