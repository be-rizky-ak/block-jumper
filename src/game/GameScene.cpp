#include "GameScene.h"
#include "ResourcePath.h"

#include <filesystem>
#include <iostream>

#include "Aion/Assets/AssetManager.h"
#include "Aion/Assets/Image.h"
#include "Aion/Assets/MeshFactory.h"
#include "Aion/Audio/AudioClip.h"
#include "Aion/Audio/AudioSystem.h"
#include "Aion/Core/TaskSystem.h"
#include "Aion/Importers/ModelImporter.h"
#include "Aion/Math/Math.h"
#include "Aion/Math/Quaternion.h"
#include "Aion/Renderer/Material.h"
#include "Aion/Scene/CameraComponent.h"
#include "Aion/Scene/MeshRenderer.h"
#include "Aion/Scene/PerspectiveCamera.h"

namespace BlockJumper
{
    void GameScene::Setup(Aion::Scene* scene, const GameConfig& config, float aspectRatio, Aion::Renderer* renderer)
    {
        SetupWithPreloaded(scene, config, aspectRatio, renderer, nullptr, nullptr, nullptr);
    }

    void GameScene::SetupWithPreloaded(
        Aion::Scene* scene,
        const GameConfig& config,
        float aspectRatio,
        Aion::Renderer* renderer,
        std::shared_ptr<Aion::EnvironmentMap> grassEnvMap,
        std::shared_ptr<Aion::EnvironmentMap> snowEnvMap,
        std::shared_ptr<Aion::Model> charModel)
    {
        if (!scene)
        {
            return;
        }

        m_renderer = renderer;
        m_config = config;
        m_gameState = GameState::Playing;
        m_fallTimer = 0.0f;

        std::cout << "[GameScene] Initializing 2.5D Isometric Block Jumper scene..." << std::endl;

        // Initialize persistent skybox environment maps
        if (m_renderer)
        {
            if (grassEnvMap)
            {
                m_grassEnvMap = grassEnvMap;
            }
            else
            {
                std::string grassSkyPath = ResolveAsset(m_config.Rendering.GrassSkyboxPath);
                m_grassEnvMap = Aion::EnvironmentMap::CreateFromEquirectangular(grassSkyPath);
            }

            if (snowEnvMap)
            {
                m_snowEnvMap = snowEnvMap;
            }
            else
            {
                std::string snowSkyPath = ResolveAsset(m_config.Rendering.SnowSkyboxPath);
                // Asynchronously decode secondary skybox on TaskSystem worker thread
                Aion::TaskSystem::Run([this, snowSkyPath]() {
                    std::cout << "[GameScene] Decoding Snow skybox (4096x2048) asynchronously on TaskSystem worker thread..." << std::endl;
                    auto snowImage = Aion::Image::Load(snowSkyPath, true, false);
                    Aion::TaskSystem::PostToMainThread([this, snowSkyPath, snowImage]() {
                        if (snowImage && snowImage->IsValid())
                        {
                            m_snowEnvMap = Aion::EnvironmentMap::CreateFromImage(snowImage);
                            std::cout << "[GameScene] Snow skybox GPU cubemap initialized on main thread via TaskSystem handoff." << std::endl;
                        }
                        else
                        {
                            m_snowEnvMap = Aion::EnvironmentMap::CreateFromEquirectangular(snowSkyPath);
                        }
                    });
                });
            }

            if (m_grassEnvMap && m_grassEnvMap->IsValid())
            {
                m_renderer->SetEnvironmentMap(m_grassEnvMap);
                m_renderer->SetShowSkybox(true);
                m_renderer->SetIBLEnabled(false);
                m_currentSkyboxTheme = SkyboxTheme::Grassland;
                std::cout << "[GameScene] Initialized persistent Grassland/Forest skybox (skybox-day.png)." << std::endl;
                std::cout << "[Diagnostics] GPU Texture ID: " << m_grassEnvMap->GetEnvironmentCubemap()
                          << " | Skybox rendering enabled (ShowSkybox=true, IBL=false)." << std::endl;
            }
            else
            {
                std::cout << "[GameScene] Warning: Failed to load Grassland skybox." << std::endl;
            }
        }

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
        if (!charModel)
        {
            std::string charPath = ResolveAsset(m_config.Rendering.CharacterModelPath);
            std::cout << "[GameScene] Loading Kenney 3D player character: " << charPath << std::endl;
            charModel = Aion::AssetManager::LoadModel(charPath);
        }

        if (charModel)
        {
            // Tune character material properties for clean visual readability and contrast
            for (const auto& mat : charModel->GetMaterials())
            {
                if (mat)
                {
                    mat->BaseColor = Aion::Vector4(1.22f, 1.22f, 1.22f, 1.0f);
                    mat->RoughnessFactor = 0.50f;
                    mat->EmissiveFactor = Aion::Vector3(0.08f, 0.08f, 0.08f);
                }
            }

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
        // 4. Initialize Game Over UI Overlay, Options UI, and Visual Effects System
        // -------------------------------------------------------------
        m_gameOverUI.Initialize();
        m_optionsUI.Initialize();
        m_touchControls.Initialize();
        m_vfxSystem.Initialize();
        m_stageMilestoneTimer = 0.0f;
        m_lastDifficulty = 1;
        m_gameOverDuration = 0.0f;

        std::cout << "[GameScene] Elevated isometric camera initialized (Pitch="
                  << config.Camera.PitchDegrees << " deg, Yaw="
                  << config.Camera.YawDegrees << " deg, FOV="
                  << config.Camera.Fov << " deg)." << std::endl;

        // -------------------------------------------------------------
        // 5. Preload Gameplay Sound Effects
        // -------------------------------------------------------------
        m_jumpClip = Aion::AudioSystem::LoadClip(ResolveAsset(m_config.Audio.JumpSFXPath), Aion::AudioCategory::SFX, false);
        m_gameOverClip = Aion::AudioSystem::LoadClip(ResolveAsset(m_config.Audio.GameOverSFXPath), Aion::AudioCategory::SFX, false);
        m_retryClip = Aion::AudioSystem::LoadClip(ResolveAsset(m_config.Audio.RetrySFXPath), Aion::AudioCategory::SFX, false);
        m_gameOverSoundPlayed = false;
        m_optionsUI.SetFeedbackClip(m_retryClip);
        m_highestStageReached = std::max(1, m_saveManager.GetCheckpointLevel());
        m_gameOverUI.SetLatestStageReached(m_highestStageReached);
        std::cout << "[GameScene] Audio SFX clips loaded (Jump: " << (m_jumpClip ? "OK" : "FAILED")
                  << ", GameOver: " << (m_gameOverClip ? "OK" : "FAILED")
                  << ", Retry: " << (m_retryClip ? "OK" : "FAILED")
                  << ", Checkpoint Stage: " << m_highestStageReached << ")." << std::endl;

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
        m_touchControls.ClearAllTouches();
        if (m_playerController)
        {
            m_playerController->SetInputEnabled(false);
            m_playerController->ClearTouchInput();
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
        m_gameOverDuration = 0.0f;

        // Freeze player simulation cleanly
        if (m_playerController)
        {
            m_playerController->SetFrozen(true);
        }

        // Capture final run distance from HUD progress measurement
        m_finalRunDistance = m_levelManager.GetForwardProgress();

        // Update and persist best record before presenting final result
        m_isNewRecord = m_saveManager.TryUpdateBestDistance(m_finalRunDistance);

        // Trigger Game Over popup sound effect once
        if (m_gameOverClip && !m_gameOverSoundPlayed)
        {
            Aion::AudioSystem::PlayOneShot(m_gameOverClip, 1.0f);
            m_gameOverSoundPlayed = true;
        }

        std::cout << "[GameState] Transition: Falling -> GameOver. Simulation frozen cleanly."
                  << " Run distance: " << m_finalRunDistance << "m"
                  << (m_isNewRecord ? " (NEW RECORD!)" : "")
                  << ", Best record: " << m_saveManager.GetBestDistance() << "m. Game Over UI active."
                  << std::endl;
    }

    void GameScene::TriggerRetry(int targetStage)
    {
        if (m_gameState != GameState::GameOver)
        {
            return; // Disallow retry during Playing or Falling transition
        }

        if (targetStage <= 0)
        {
            targetStage = m_highestStageReached;
        }

        if (m_retryClip)
        {
            Aion::AudioSystem::PlayOneShot(m_retryClip, 1.0f);
        }

        ResetRun(targetStage);
        std::cout << "[GameState] Transition: GameOver -> Playing (Retry executed for Stage " << targetStage << ")." << std::endl;
    }

    void GameScene::ResetRun(int targetStage)
    {
        if (targetStage <= 0)
        {
            targetStage = 1;
        }

        std::cout << "[GameScene] Executing coordinated run reset to Stage " << targetStage << "..." << std::endl;

        // 1. Reset Game State & Counters
        m_gameState = GameState::Playing;
        m_fallTimer = 0.0f;
        m_gameOverDuration = 0.0f;
        m_stageMilestoneTimer = 0.0f;
        m_finalRunDistance = 0.0f;
        m_isNewRecord = false;
        m_gameOverSoundPlayed = false;
        m_retryCount++;
        m_vfxSystem.Reset();
        m_touchControls.Reset();

        Aion::Vector3 spawnPos = m_config.Player.SpawnPosition;
        if (targetStage > 1)
        {
            m_levelManager.ResetToStage(targetStage, spawnPos);
        }
        else
        {
            m_levelManager.Reset();
        }

        m_lastDifficulty = m_levelManager.GetCurrentDifficulty();

        // 2. Reset Player Kinematics, Velocity, and Orientation
        if (m_playerController && m_player)
        {
            m_playerController->ResetKinematics(spawnPos, -135.0f);
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

        // 4. Update main platform reference
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

        // 6. Synchronize skybox theme with stage difficulty
        if (m_renderer)
        {
            int diff = m_levelManager.GetCurrentDifficulty();
            if (diff >= 4 && m_snowEnvMap && m_snowEnvMap->IsValid())
            {
                if (m_currentSkyboxTheme != SkyboxTheme::Snow)
                {
                    m_renderer->SetEnvironmentMap(m_snowEnvMap);
                    m_currentSkyboxTheme = SkyboxTheme::Snow;
                }
            }
            else if (diff < 4 && m_grassEnvMap && m_grassEnvMap->IsValid())
            {
                if (m_currentSkyboxTheme != SkyboxTheme::Grassland)
                {
                    m_renderer->SetEnvironmentMap(m_grassEnvMap);
                    m_currentSkyboxTheme = SkyboxTheme::Grassland;
                }
            }
        }

        std::cout << "[GameScene] Run reset complete. State: Playing. Restored to difficulty ("
                  << m_levelManager.GetDifficultyStageName(m_levelManager.GetCurrentDifficulty())
                  << ") with " << m_levelManager.GetActivePlatforms().size() << " initial platforms." << std::endl;
    }

    void GameScene::Update(float deltaTime, int screenWidth, int screenHeight)
    {
        // 0. Options Window Management
        if (m_optionsUI.IsOpen())
        {
            m_optionsUI.Update(deltaTime, screenWidth, screenHeight);
            return; // Gameplay and simulation remain completely frozen while Options is open
        }

        // Check entry points to open Options
        if (m_gameState == GameState::GameOver)
        {
            if (m_gameOverUI.CheckGameOverOptionsAction(screenWidth, screenHeight))
            {
                OpenOptions();
                return;
            }
        }
        else
        {
            if (m_gameOverUI.CheckHUDOptionsAction(screenWidth, screenHeight))
            {
                OpenOptions();
                return;
            }
        }

        // Update touch controls layout and input
        bool blockGameplayTouch = (m_gameState != GameState::Playing);
        m_touchControls.Update(deltaTime, screenWidth, screenHeight, blockGameplayTouch);

        if (m_playerController)
        {
            if (m_gameState == GameState::Playing)
            {
                m_playerController->SetTouchInput(
                    m_touchControls.GetForwardInput(),
                    m_touchControls.GetRightInput(),
                    m_touchControls.ConsumeJumpEdge());
            }
            else
            {
                m_playerController->ClearTouchInput();
            }
        }

        if (!m_player)
        {
            return;
        }

        // Decrement milestone feedback timer
        if (m_stageMilestoneTimer > 0.0f)
        {
            m_stageMilestoneTimer -= deltaTime;
        }

        // Detect jump and landing events for visual effects
        if (m_playerController)
        {
            Aion::Vector3 feetPos = m_player->Transform.Position;
            feetPos.y -= 0.5f;

            if (m_playerController->JustDoubleJumped())
            {
                m_vfxSystem.TriggerDoubleJumpBurst(feetPos);
                if (m_jumpClip)
                {
                    Aion::AudioSystem::PlayOneShot(m_jumpClip, 1.0f);
                }
            }
            else if (m_playerController->JustJumped())
            {
                m_vfxSystem.TriggerJumpDust(feetPos);
                if (m_jumpClip)
                {
                    Aion::AudioSystem::PlayOneShot(m_jumpClip, 1.0f);
                }
            }
            else if (m_playerController->JustLanded())
            {
                m_vfxSystem.TriggerLandingBurst(feetPos, m_playerController->GetVerticalVelocity());
            }
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

            // Detect difficulty milestone progression
            int currentDiff = m_levelManager.GetCurrentDifficulty();
            if (currentDiff > m_lastDifficulty)
            {
                m_stageMilestoneTimer = 1.2f; // Trigger visual emphasis on HUD
                m_lastDifficulty = currentDiff;
            }

            if (currentDiff > m_highestStageReached)
            {
                m_highestStageReached = currentDiff;
                m_saveManager.SetCheckpointLevel(m_highestStageReached);
                m_gameOverUI.SetLatestStageReached(m_highestStageReached);
            }

            // Synchronize skybox theme with difficulty progression
            if (m_renderer)
            {
                int diff = m_levelManager.GetCurrentDifficulty();
                if (diff >= 4 && m_currentSkyboxTheme != SkyboxTheme::Snow)
                {
                    if (m_snowEnvMap && m_snowEnvMap->IsValid())
                    {
                        m_renderer->SetEnvironmentMap(m_snowEnvMap);
                        m_currentSkyboxTheme = SkyboxTheme::Snow;
                        std::cout << "[GameScene] Transitioned skybox to Snow/Mountain alpine theme (skybox-morning.png)." << std::endl;
                    }
                }
                else if (diff < 4 && m_currentSkyboxTheme != SkyboxTheme::Grassland)
                {
                    if (m_grassEnvMap && m_grassEnvMap->IsValid())
                    {
                        m_renderer->SetEnvironmentMap(m_grassEnvMap);
                        m_currentSkyboxTheme = SkyboxTheme::Grassland;
                        std::cout << "[GameScene] Restored skybox to Grassland theme (skybox-day.png)." << std::endl;
                    }
                }
            }
        }
        // 2. Falling State: Intentional graceful falling transition under natural gravity
        else if (m_gameState == GameState::Falling)
        {
            m_fallTimer += deltaTime;
            m_vfxSystem.TriggerFallingStreak(m_player->Transform.Position);

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
            m_gameOverDuration += deltaTime;
            if (m_gameOverUI.CheckRetryAction(screenWidth, screenHeight))
            {
                int targetStage = m_gameOverUI.GetSelectedRetryStage();
                TriggerRetry(targetStage);
            }
        }

        // Update active visual effect particles
        m_vfxSystem.Update(deltaTime);
    }

    void GameScene::RenderUI(int windowWidth, int windowHeight)
    {
        // 1. Render 3D Visual Effects in world space before 2D screen UI
        if (m_cameraObject)
        {
            auto* camComp = m_cameraObject->GetComponent<Aion::CameraComponent>();
            if (camComp)
            {
                Aion::Matrix4 viewProj = camComp->GetProjectionMatrix() * camComp->GetViewMatrix();
                m_vfxSystem.Render(viewProj);
            }
        }

        // 2. Render 2D UI Overlay
        if (m_gameState == GameState::Playing || m_gameState == GameState::Falling)
        {
            m_gameOverUI.RenderHUD(
                windowWidth,
                windowHeight,
                m_levelManager.GetForwardProgress(),
                m_levelManager.GetCurrentDifficulty(),
                m_levelManager.GetDifficultyStageName(m_levelManager.GetCurrentDifficulty()),
                m_saveManager.GetBestDistance(),
                m_stageMilestoneTimer);

            // Render Touch Controls (Joystick, Jump, Toggle Button)
            m_touchControls.Render(windowWidth, windowHeight);
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
                m_levelManager.GetDifficultyStageName(m_levelManager.GetCurrentDifficulty()),
                m_gameOverDuration,
                m_highestStageReached);
        }

        // 3. Render Options Window on top if open
        if (m_optionsUI.IsOpen())
        {
            m_optionsUI.Render(windowWidth, windowHeight);
        }
    }

    void GameScene::Shutdown()
    {
        m_jumpClip.reset();
        m_gameOverClip.reset();
        m_retryClip.reset();
        m_gameOverSoundPlayed = false;

        m_touchControls.Shutdown();
        m_gameOverUI.Shutdown();
        m_optionsUI.Shutdown();
        m_vfxSystem.Shutdown();
        m_grassEnvMap.reset();
        m_snowEnvMap.reset();
        m_player.reset();
        m_mainPlatform.reset();
        m_cameraObject.reset();
        m_renderer = nullptr;
    }
} // namespace BlockJumper
