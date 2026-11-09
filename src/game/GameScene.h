#pragma once

#include <memory>
#include <vector>

#include "Aion/Scene/Object3D.h"
#include "Aion/Scene/Scene.h"
#include "camera/FollowCamera.h"
#include "game/GameConfig.h"
#include "game/GameState.h"
#include "game/LevelManager.h"
#include "game/SaveManager.h"
#include "player/PlayerController.h"
#include "ui/GameOverUI.h"

namespace BlockJumper
{
    class GameScene
    {
    public:
        GameScene() = default;

        /**
         * @brief Builds and populates the scene with player, multi-directional platforms, and isometric camera.
         */
        void Setup(Aion::Scene* scene, const GameConfig& config, float aspectRatio);

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
        FollowCamera* GetFollowCamera() const { return m_followCamera; }

        LevelManager& GetLevelManager() { return m_levelManager; }
        const LevelManager& GetLevelManager() const { return m_levelManager; }
        GameOverUI& GetGameOverUI() { return m_gameOverUI; }

        int GetDeathCount() const { return m_deathCount; }
        int GetRetryCount() const { return m_retryCount; }

        SaveManager& GetSaveManager() { return m_saveManager; }
        const SaveManager& GetSaveManager() const { return m_saveManager; }
        float GetFinalRunDistance() const { return m_finalRunDistance; }
        bool IsNewRecord() const { return m_isNewRecord; }

    private:
        std::shared_ptr<Aion::Object3D> m_player;
        std::shared_ptr<Aion::Object3D> m_mainPlatform;
        LevelManager m_levelManager;
        std::shared_ptr<Aion::Object3D> m_cameraObject;

        PlayerController* m_playerController = nullptr;
        FollowCamera* m_followCamera = nullptr;

        GameState m_gameState = GameState::Playing;
        GameOverUI m_gameOverUI;
        GameConfig m_config;
        SaveManager m_saveManager;

        float m_fallTimer = 0.0f;
        float m_finalRunDistance = 0.0f;
        bool m_isNewRecord = false;
        int m_deathCount = 0;
        int m_retryCount = 0;
    };
} // namespace BlockJumper
