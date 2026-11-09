#pragma once

#include <memory>
#include <vector>

#include "Aion/Scene/Object3D.h"
#include "Aion/Scene/Scene.h"
#include "camera/FollowCamera.h"
#include "game/GameConfig.h"
#include "game/LevelManager.h"
#include "player/PlayerController.h"

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
         */
        void Update(float deltaTime);

        std::shared_ptr<Aion::Object3D> GetPlayer() const { return m_player; }
        std::shared_ptr<Aion::Object3D> GetPlatform() const { return m_mainPlatform; }
        const std::vector<PlatformBox>& GetPlatformData() const { return m_levelManager.GetActivePlatforms(); }

        std::shared_ptr<Aion::Object3D> GetCameraObject() const { return m_cameraObject; }
        PlayerController* GetPlayerController() const { return m_playerController; }
        FollowCamera* GetFollowCamera() const { return m_followCamera; }

        LevelManager& GetLevelManager() { return m_levelManager; }
        const LevelManager& GetLevelManager() const { return m_levelManager; }

    private:
        std::shared_ptr<Aion::Object3D> m_player;
        std::shared_ptr<Aion::Object3D> m_mainPlatform;
        LevelManager m_levelManager;
        std::shared_ptr<Aion::Object3D> m_cameraObject;

        PlayerController* m_playerController = nullptr;
        FollowCamera* m_followCamera = nullptr;
    };
} // namespace BlockJumper
