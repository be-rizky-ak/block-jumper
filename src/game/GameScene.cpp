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
        // 1. Build Multi-Directional Platform World
        // -------------------------------------------------------------
        m_platforms = GameConfig::CreateDefaultLevel();
        m_platformObjects.clear();

        for (size_t i = 0; i < m_platforms.size(); ++i)
        {
            const auto& plat = m_platforms[i];
            auto platObj = std::make_shared<Aion::Object3D>(plat.Name);

            auto mat = std::make_shared<Aion::Material>();
            mat->BaseColor = plat.Color;
            mat->MetallicFactor = plat.Metallic;
            mat->RoughnessFactor = plat.Roughness;

            platObj->AddComponent<Aion::MeshRenderer>(cubeMesh, mat);
            platObj->Transform.Position = plat.Position;
            platObj->Transform.Scale = plat.Dimensions;

            scene->Add(platObj);
            m_platformObjects.push_back(platObj);

            if (i == 0)
            {
                m_mainPlatform = platObj;
            }
        }

        std::cout << "[GameScene] Created " << m_platforms.size()
                  << " multi-directional platforms, elevated sections, and obstacles." << std::endl;

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
            m_platforms,
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
} // namespace BlockJumper
