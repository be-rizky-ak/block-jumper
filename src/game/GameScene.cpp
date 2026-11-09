#include "GameScene.h"

#include <filesystem>
#include <iostream>

#include "Aion/Assets/AssetManager.h"
#include "Aion/Assets/MeshFactory.h"
#include "Aion/Importers/ModelImporter.h"
#include "Aion/Math/Math.h"
#include "Aion/Math/Quaternion.h"
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

        m_config = config;
        m_gameState = GameState::Playing;
        m_fallTimer = 0.0f;

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
        // 2. Player Character (Kenney 3D Character)
        // -------------------------------------------------------------
        std::string rawCharPath = "assets/models/kenney_mini-characters/models/character-male-a.glb";
        std::string charPath = rawCharPath;
        if (!std::filesystem::exists(charPath))
        {
            if (std::filesystem::exists("block-jumper-stagging/" + rawCharPath))
            {
                charPath = "block-jumper-stagging/" + rawCharPath;
            }
            else if (std::filesystem::exists("../" + rawCharPath))
            {
                charPath = "../" + rawCharPath;
            }
        }
        std::cout << "[GameScene] Loading Kenney 3D player character: " << charPath << std::endl;
        auto charModel = Aion::AssetManager::LoadModel(charPath);

        if (charModel)
        {
            m_player = Aion::ModelImporter::CreateModelObject(charModel);
            if (m_player)
            {
                m_player->SetName("Player");
                // Align visual model child nodes:
                // Unscaled model height is ~0.67 units, base at Y=0.
                // Scale factor 1.45 scales height to ~0.97 units (~1.0 unit player cube height).
                // Offset Y=-0.5 places character feet at bottom of collision box (currentPosition.y - 0.5).
                // Rotation 180 deg around Y aligns +Z glTF forward facing with -Z movement forward.
                for (const auto& child : m_player->GetChildren())
                {
                    if (child)
                    {
                        child->Transform.Position = Aion::Vector3(0.0f, -0.5f, 0.0f);
                        child->Transform.Rotation = Aion::Quaternion::FromEuler(0.0f, Aion::Math::Radians(180.0f), 0.0f);
                        child->Transform.Scale = Aion::Vector3(1.45f);
                    }
                }
            }
        }

        // Safe fallback to primitive 3D block if model loading failed
        if (!m_player)
        {
            m_player = std::make_shared<Aion::Object3D>("Player");

            auto playerMaterial = std::make_shared<Aion::Material>();
            playerMaterial->BaseColor = Aion::Vector4(1.0f, 0.48f, 0.12f, 1.0f); // Vibrant orange
            playerMaterial->MetallicFactor = 0.2f;
            playerMaterial->RoughnessFactor = 0.35f;

            m_player->AddComponent<Aion::MeshRenderer>(cubeMesh, playerMaterial);
        }

        // Position player on top of the central starting plaza
        m_player->Transform.Position = config.Player.SpawnPosition;
        m_player->Transform.Scale = Aion::Vector3(1.0f);

        m_playerController = m_player->AddComponent<PlayerController>(
            config.Player,
            m_levelManager.GetActivePlatforms(),
            config.Camera.YawDegrees);

        auto* animComp = m_player->GetComponent<Aion::AnimationComponent>();
        if (animComp)
        {
            m_playerAnimController = m_player->AddComponent<PlayerAnimationController>();
            m_playerAnimController->Initialize(m_playerController, animComp);
            std::cout << "[GameScene] Attached PlayerAnimationController with Kenney character model." << std::endl;
        }

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

        // -------------------------------------------------------------
        // 4. Initialize Game Over UI Overlay
        // -------------------------------------------------------------
        m_gameOverUI.Initialize();

        std::cout << "[GameScene] Elevated isometric camera initialized (Pitch="
                  << config.Camera.PitchDegrees << " deg, Yaw="
                  << config.Camera.YawDegrees << " deg, FOV="
                  << config.Camera.Fov << " deg)." << std::endl;
        std::cout << "[GameScene] Isometric scene setup complete. Initial state: Playing." << std::endl;
    }

    void GameScene::TriggerFall()
    {
        if (m_gameState != GameState::Playing)
        {
            return; // Trigger fall only once per run
        }

        m_gameState = GameState::Falling;
        m_fallTimer = 0.0f;
        m_deathCount++;

        // Stop accepting movement and jump input
        if (m_playerController)
        {
            m_playerController->SetInputEnabled(false);
        }

        float playerY = m_player ? m_player->Transform.Position.y : 0.0f;
        std::cout << "[GameState] Transition: Playing -> Falling. Fall detected at Y="
                  << playerY << " (Death threshold: " << m_config.Player.FallDeathThreshold
                  << "). Movement/jump input and segment spawning paused." << std::endl;
    }

    void GameScene::TriggerGameOver()
    {
        if (m_gameState != GameState::Falling)
        {
            return;
        }

        m_gameState = GameState::GameOver;

        // Freeze player simulation cleanly
        if (m_playerController)
        {
            m_playerController->SetFrozen(true);
        }

        // Capture final run distance from HUD progress measurement
        m_finalRunDistance = m_levelManager.GetForwardProgress();

        // Update and persist best record before presenting final result
        m_isNewRecord = m_saveManager.TryUpdateBestDistance(m_finalRunDistance);

        std::cout << "[GameState] Transition: Falling -> GameOver. Simulation frozen cleanly."
                  << " Run distance: " << m_finalRunDistance << "m"
                  << (m_isNewRecord ? " (NEW RECORD!)" : "")
                  << ", Best record: " << m_saveManager.GetBestDistance() << "m. Game Over UI active."
                  << std::endl;
    }

    void GameScene::TriggerRetry()
    {
        if (m_gameState != GameState::GameOver)
        {
            return; // Disallow retry during Playing or Falling transition
        }

        ResetRun();
        std::cout << "[GameState] Transition: GameOver -> Playing (Retry executed)." << std::endl;
    }

    void GameScene::ResetRun()
    {
        std::cout << "[GameScene] Executing coordinated run reset..." << std::endl;

        // 1. Reset Game State & Counters
        m_gameState = GameState::Playing;
        m_fallTimer = 0.0f;
        m_finalRunDistance = 0.0f;
        m_isNewRecord = false;
        m_retryCount++;

        // 2. Reset Player Kinematics, Velocity, and Orientation
        if (m_playerController && m_player)
        {
            m_playerController->ResetKinematics(m_config.Player.SpawnPosition, -135.0f);
        }

        if (m_playerAnimController)
        {
            m_playerAnimController->Reset();
        }

        // 3. Restore Initial Camera Follow State & Offset
        if (m_followCamera)
        {
            m_followCamera->SnapToTarget();
        }

        // 4. Reinitialize Level Progression, Segment History, and Active Platforms
        m_levelManager.Reset();

        if (!m_levelManager.GetActiveSegments().empty() &&
            !m_levelManager.GetActiveSegments().front()->GetSceneObjects().empty())
        {
            m_mainPlatform = m_levelManager.GetActiveSegments().front()->GetSceneObjects().front();
        }

        // 5. Synchronize fresh active platforms with player collider
        if (m_playerController)
        {
            m_playerController->SetPlatforms(m_levelManager.GetActivePlatforms());
        }

        std::cout << "[GameScene] Run reset complete. State: Playing. Restored to initial difficulty ("
                  << m_levelManager.GetDifficultyStageName(m_levelManager.GetCurrentDifficulty())
                  << ") with " << m_levelManager.GetActivePlatforms().size() << " initial platforms." << std::endl;
    }

    void GameScene::Update(float deltaTime, int screenWidth, int screenHeight)
    {
        if (!m_player)
        {
            return;
        }

        // 1. Playing State
        if (m_gameState == GameState::Playing)
        {
            // Reliable Fall Detection: Check if player falls below configurable world-space death threshold
            if (m_player->Transform.Position.y < m_config.Player.FallDeathThreshold)
            {
                TriggerFall();
                return;
            }

            // Normal progression: update segments and collision
            bool platformsChanged = m_levelManager.Update(m_player->Transform.Position);
            if (platformsChanged && m_playerController)
            {
                m_playerController->SetPlatforms(m_levelManager.GetActivePlatforms());
            }
        }
        // 2. Falling State: Intentional graceful falling transition under natural gravity
        else if (m_gameState == GameState::Falling)
        {
            m_fallTimer += deltaTime;

            // Physics continues naturally; no segment spawning or difficulty progression occurs
            if (m_fallTimer >= m_config.Player.FallTransitionDuration ||
                m_player->Transform.Position.y < m_config.Player.VoidFallLimit)
            {
                TriggerGameOver();
            }
        }
        // 3. GameOver State: Simulation is cleanly frozen; poll for Retry action
        else if (m_gameState == GameState::GameOver)
        {
            if (m_gameOverUI.CheckRetryAction(screenWidth, screenHeight))
            {
                TriggerRetry();
            }
        }
    }

    void GameScene::RenderUI(int windowWidth, int windowHeight)
    {
        if (m_gameState == GameState::Playing || m_gameState == GameState::Falling)
        {
            m_gameOverUI.RenderHUD(
                windowWidth,
                windowHeight,
                m_levelManager.GetForwardProgress(),
                m_levelManager.GetCurrentDifficulty(),
                m_levelManager.GetDifficultyStageName(m_levelManager.GetCurrentDifficulty()),
                m_saveManager.GetBestDistance());
        }
        else if (m_gameState == GameState::GameOver)
        {
            m_gameOverUI.RenderGameOver(
                windowWidth,
                windowHeight,
                m_finalRunDistance,
                m_saveManager.GetBestDistance(),
                m_isNewRecord,
                m_levelManager.GetCompletedSegments(),
                m_levelManager.GetDifficultyStageName(m_levelManager.GetCurrentDifficulty()));
        }
    }
} // namespace BlockJumper
