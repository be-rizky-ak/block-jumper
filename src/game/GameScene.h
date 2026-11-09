#pragma once

#include <memory>
#include <vector>

#include "Aion/Renderer/EnvironmentMap.h"
#include "Aion/Renderer/Renderer.h"
#include "Aion/Scene/Object3D.h"
#include "Aion/Scene/Scene.h"
#include "camera/FollowCamera.h"
#include "game/GameConfig.h"
#include "game/GameState.h"
#include "game/LevelManager.h"
#include "game/SaveManager.h"
#include "player/PlayerAnimationController.h"
#include "player/PlayerController.h"
#include "ui/GameOverUI.h"
#include "vfx/VisualEffectsSystem.h"

namespace Aion
{
    class Model;
}

namespace BlockJumper
{
    enum class SkyboxTheme
    {
        None,
        Grassland,
        Snow
    };

    class GameScene
    {
    public:
        GameScene() = default;
        ~GameScene() = default;

        /**
         * @brief Builds and populates the scene with player, multi-directional platforms, and isometric camera.
         */
        void Setup(Aion::Scene* scene, const GameConfig& config, float aspectRatio, Aion::Renderer* renderer = nullptr);

        /**
         * @brief Builds the scene reusing preloaded assets (skybox, character model).
         */
        void SetupWithPreloaded(
            Aion::Scene* scene,
            const GameConfig& config,
            float aspectRatio,
            Aion::Renderer* renderer,
            std::shared_ptr<Aion::EnvironmentMap> grassEnvMap,
            std::shared_ptr<Aion::EnvironmentMap> snowEnvMap = nullptr,
            std::shared_ptr<Aion::Model> charModel = nullptr);

        /**
         * @brief Cleans up GPU and scene resources.
         */
        void Shutdown();

        /**
         * @brief Updates endless level progression and synchronizes active platform collision.
         * Evaluates falling detection, falling transition timer, and retry input.
         */
        void Update(float deltaTime, int screenWidth = 1280, int screenHeight = 720);

        /**
         * @brief Renders the Game Over UI overlay when in GameOver state.
         */
        void RenderUI(int windowWidth, int windowHeight);

        // Gameplay State queries and transitions
        GameState GetGameState() const { return m_gameState; }
        bool IsPlaying() const { return m_gameState == GameState::Playing; }
        bool IsFalling() const { return m_gameState == GameState::Falling; }
        bool IsGameOver() const { return m_gameState == GameState::GameOver; }

        void TriggerFall();
        void TriggerGameOver();
        void TriggerRetry();
        void ResetRun();

        std::shared_ptr<Aion::Object3D> GetPlayer() const { return m_player; }
        std::shared_ptr<Aion::Object3D> GetPlatform() const { return m_mainPlatform; }
        const std::vector<PlatformBox>& GetPlatformData() const { return m_levelManager.GetActivePlatforms(); }

        std::shared_ptr<Aion::Object3D> GetCameraObject() const { return m_cameraObject; }
        PlayerController* GetPlayerController() const { return m_playerController; }
        PlayerAnimationController* GetPlayerAnimationController() const { return m_playerAnimController; }
        FollowCamera* GetFollowCamera() const { return m_followCamera; }

        LevelManager& GetLevelManager() { return m_levelManager; }
        const LevelManager& GetLevelManager() const { return m_levelManager; }
        GameOverUI& GetGameOverUI() { return m_gameOverUI; }
        VisualEffectsSystem& GetVFXSystem() { return m_vfxSystem; }
        const VisualEffectsSystem& GetVFXSystem() const { return m_vfxSystem; }

        int GetDeathCount() const { return m_deathCount; }
        int GetRetryCount() const { return m_retryCount; }

        SaveManager& GetSaveManager() { return m_saveManager; }
        const SaveManager& GetSaveManager() const { return m_saveManager; }
        float GetFinalRunDistance() const { return m_finalRunDistance; }
        bool IsNewRecord() const { return m_isNewRecord; }

        SkyboxTheme GetCurrentSkyboxTheme() const { return m_currentSkyboxTheme; }
        std::shared_ptr<Aion::EnvironmentMap> GetGrasslandSkybox() const { return m_grassEnvMap; }
        std::shared_ptr<Aion::EnvironmentMap> GetSnowSkybox() const { return m_snowEnvMap; }

    private:
        std::shared_ptr<Aion::Object3D> m_player;
        std::shared_ptr<Aion::Object3D> m_mainPlatform;
        LevelManager m_levelManager;
        std::shared_ptr<Aion::Object3D> m_cameraObject;

        PlayerController* m_playerController = nullptr;
        PlayerAnimationController* m_playerAnimController = nullptr;
        FollowCamera* m_followCamera = nullptr;

        Aion::Renderer* m_renderer = nullptr;
        std::shared_ptr<Aion::EnvironmentMap> m_grassEnvMap;
        std::shared_ptr<Aion::EnvironmentMap> m_snowEnvMap;
        SkyboxTheme m_currentSkyboxTheme = SkyboxTheme::None;

        GameState m_gameState = GameState::Playing;
        GameOverUI m_gameOverUI;
        VisualEffectsSystem m_vfxSystem;
        GameConfig m_config;
        SaveManager m_saveManager;

        float m_fallTimer = 0.0f;
        float m_gameOverDuration = 0.0f;
        float m_stageMilestoneTimer = 0.0f;
        int m_lastDifficulty = 1;
        float m_finalRunDistance = 0.0f;
        bool m_isNewRecord = false;
        int m_deathCount = 0;
        int m_retryCount = 0;
    };
} // namespace BlockJumper
