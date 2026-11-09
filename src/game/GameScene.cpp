#include "GameScene.h"

#include <iostream>

#include "Aion/Assets/MeshFactory.h"
#include "Aion/Renderer/Material.h"
#include "Aion/Scene/CameraComponent.h"
#include "Aion/Scene/MeshRenderer.h"
#include "Aion/Scene/PerspectiveCamera.h"

namespace BlockJumper
{
    void GameScene::Setup(Aion::Scene* scene, const GameConfig& config, float aspectRatio)
    {
        if (!scene)
        {
            return;
        }

        std::cout << "[GameScene] Initializing 2.5D Isometric Block Jumper scene..." << std::endl;

        // Shared primitive cube mesh for 3D Kenney-style block geometry
        auto cubeMesh = std::shared_ptr<Aion::Mesh>(Aion::MeshFactory::CreateCube());

        // -------------------------------------------------------------
        // 1. Initialize Endless Level Progression & Starting Platform
        // -------------------------------------------------------------
        m_levelManager.Initialize(scene, config, cubeMesh);

        if (!m_levelManager.GetActiveSegments().empty() &&
            !m_levelManager.GetActiveSegments().front()->GetSceneObjects().empty())
        {
            m_mainPlatform = m_levelManager.GetActiveSegments().front()->GetSceneObjects().front();
        }

        std::cout << "[GameScene] Endless level progression initialized with "
                  << m_levelManager.GetActivePlatforms().size()
                  << " initial platforms across " << m_levelManager.GetActiveSegmentsCount()
                  << " active segments." << std::endl;

        // -------------------------------------------------------------
        // 2. Player Character (Primitive 3D Block)
        // -------------------------------------------------------------
        m_player = std::make_shared<Aion::Object3D>("Player");

        auto playerMaterial = std::make_shared<Aion::Material>();
        playerMaterial->BaseColor = Aion::Vector4(1.0f, 0.48f, 0.12f, 1.0f); // Vibrant orange
        playerMaterial->MetallicFactor = 0.2f;
        playerMaterial->RoughnessFactor = 0.35f;

        m_player->AddComponent<Aion::MeshRenderer>(cubeMesh, playerMaterial);

        // Position player on top of the central starting plaza
        m_player->Transform.Position = config.Player.SpawnPosition;
        m_player->Transform.Scale = config.Player.Dimensions;

        m_playerController = m_player->AddComponent<PlayerController>(
            config.Player,
            m_levelManager.GetActivePlatforms(),
            config.Camera.YawDegrees);

        scene->Add(m_player);
        std::cout << "[GameScene] Player character spawned at ("
                  << config.Player.SpawnPosition.x << ", "
                  << config.Player.SpawnPosition.y << ", "
                  << config.Player.SpawnPosition.z << ")" << std::endl;

        // -------------------------------------------------------------
        // 3. Elevated Isometric 2.5D Camera
        // -------------------------------------------------------------
        m_cameraObject = std::make_shared<Aion::Object3D>("MainCamera");

        auto perspectiveCamera = std::make_unique<Aion::PerspectiveCamera>(
            config.Camera.Fov,
            aspectRatio,
            config.Camera.NearPlane,
            config.Camera.FarPlane);

        auto* cameraComponent = m_cameraObject->AddComponent<Aion::CameraComponent>(std::move(perspectiveCamera));

        m_followCamera = m_cameraObject->AddComponent<FollowCamera>(config.Camera);
        m_followCamera->SetTarget(m_player);

        scene->Add(m_cameraObject);
        scene->SetActiveCamera(cameraComponent);

        std::cout << "[GameScene] Elevated isometric camera initialized (Pitch="
                  << config.Camera.PitchDegrees << " deg, Yaw="
                  << config.Camera.YawDegrees << " deg, FOV="
                  << config.Camera.Fov << " deg)." << std::endl;
        std::cout << "[GameScene] Isometric scene setup complete." << std::endl;
    }

    void GameScene::Update(float /*deltaTime*/)
    {
        if (!m_player)
        {
            return;
        }

        bool platformsChanged = m_levelManager.Update(m_player->Transform.Position);
        if (platformsChanged && m_playerController)
        {
            m_playerController->SetPlatforms(m_levelManager.GetActivePlatforms());
        }
    }
} // namespace BlockJumper
