#include "BlockJumperApp.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <vector>

#include "Aion/Assets/AssetManager.h"
#include "Aion/Assets/Image.h"
#include "Aion/Core/Input.h"
#include "Aion/Core/KeyCodes.h"
#include "Aion/Core/TaskSystem.h"
#include "Aion/Core/Time.h"
#include "Aion/Core/Window.h"
#include "Aion/Renderer/EnvironmentMap.h"
#include "Aion/Renderer/RenderCommand.h"
#include "Aion/Renderer/Renderer.h"
#include "ui/LoadingScreen.h"

namespace BlockJumper
{
    namespace
    {
        std::string ResolveAsset(const std::string& path)
        {
            const std::vector<std::string> prefixes = {
                "",
                "games/BlockJumper/",
                "assets/",
                "build/games/BlockJumper/",
                "../",
                "../games/BlockJumper/",
                "../../games/BlockJumper/",
                "prorotype-stagging/games/BlockJumper/",
                "../prorotype-stagging/games/BlockJumper/",
                "block-jumper-stagging/",
            };

            for (const auto& prefix : prefixes)
            {
                std::string candidate = prefix + path;
                if (std::filesystem::exists(candidate))
                {
                    return candidate;
                }
            }
            return path;
        }
    } // namespace

    BlockJumperApp::BlockJumperApp()
        : Aion::Application(Aion::ApplicationSpecification{
            .Name = "Block Jumper - Isometric 2.5D",
            .Width = 1280,
            .Height = 720,
            .VSync = true
        })
    {
    }

    BlockJumperApp::~BlockJumperApp()
    {
        std::cout << "[BlockJumperApp] Destructor started." << std::endl;
        m_gameScene.Shutdown();
        m_loadingScreen.Shutdown();
        m_grassEnvMap.reset();
        m_snowEnvMap.reset();
        m_charModel.reset();
        std::cout << "[BlockJumperApp] Destructor completed." << std::endl;
    }

    void BlockJumperApp::OnCreate()
    {
        std::cout << "========================================" << std::endl;
        std::cout << " Block Jumper - Isometric 2.5D Prototype" << std::endl;
        std::cout << "========================================" << std::endl;

        // Configure polished directional lighting
        if (GetRenderer())
        {
            GetRenderer()->SetLightDirection(Aion::Vector3(0.55f, 0.80f, 0.25f).Normalized());
            GetRenderer()->SetLightColor(Aion::Vector3(2.2f, 2.15f, 2.05f));
            GetRenderer()->SetAmbientColor(Aion::Vector3(0.28f, 0.29f, 0.34f));
            GetRenderer()->SetShadowsEnabled(true);
            GetRenderer()->SetShadowBias(0.0025f, 0.0005f);
        }

        // Initialize dedicated full-screen startup loading presentation layer
        m_loadingScreen.Initialize();
        m_lifecycleState = AppLifecycleState::StartupLoading;
        m_loadingPhase = LoadingPhase::Begin;
        m_loadingProgress = 0.05f;
        m_loadingStatus = "PREPARING APPLICATION...";
        m_fadeAlpha = 1.0f;
        m_grassSkyboxReady.store(false);

        std::cout << "[StartupLoading] Application window created. Starting startup loading sequence..." << std::endl;
        std::cout << "[StartupLoading] Loading screen presentation layer initialized." << std::endl;
    }

    void BlockJumperApp::OnUpdate()
    {
        float dt = Aion::Time::GetDeltaTime();
        if (dt > 0.05f)
        {
            dt = 0.05f; // Clamp delta time to avoid simulation jumps during disk I/O
        }

        int winW = 1280;
        int winH = 720;
        float aspect = 1280.0f / 720.0f;
        if (GetWindow())
        {
            winW = GetWindow()->GetWidth();
            winH = GetWindow()->GetHeight();
            if (winH > 0)
            {
                aspect = static_cast<float>(winW) / static_cast<float>(winH);
            }
        }

        // -------------------------------------------------------------
        // 1. Startup Loading Lifecycle: asynchronous asset staging
        // -------------------------------------------------------------
        if (m_lifecycleState == AppLifecycleState::StartupLoading)
        {
            if (Aion::Input::GetKey(Aion::Key::Escape) && m_loadingFailed)
            {
                if (GetWindow())
                {
                    GetWindow()->SetShouldClose(true);
                }
                return;
            }

            if (m_loadingFailed)
            {
                return; // Keep loading screen error visible; do not proceed
            }

            if (m_loadingPhase == LoadingPhase::Begin)
            {
                m_loadingStatus = "LOADING SKYBOX ENVIRONMENT...";
                m_loadingProgress = 0.20f;
                m_loadingPhase = LoadingPhase::LoadingSkybox;

                std::string grassSkyPath = ResolveAsset("assets/textures/kenney_skyboxes/skyboxes/skybox-day.png");
                std::string snowSkyPath = ResolveAsset("assets/textures/kenney_skyboxes/skyboxes/skybox-morning.png");

                std::cout << "[StartupLoading] Asynchronously loading required Grassland skybox: " << grassSkyPath << std::endl;

                // Async decode of required Grassland skybox on TaskSystem worker thread
                Aion::TaskSystem::Run([this, grassSkyPath]() {
                    std::cout << "[StartupLoading Worker] Starting Image::Load for: " << grassSkyPath << std::endl;
                    auto grassImage = Aion::Image::Load(grassSkyPath, true, false);
                    std::cout << "[StartupLoading Worker] Image::Load returned for: " << grassSkyPath
                              << " | Valid: " << (grassImage && grassImage->IsValid()) << std::endl;
                    Aion::TaskSystem::PostToMainThread([this, grassImage, grassSkyPath]() {
                        std::cout << "[StartupLoading Main] PostToMainThread callback executing for Grassland skybox." << std::endl;
                        if (grassImage && grassImage->IsValid())
                        {
                            m_grassEnvMap = Aion::EnvironmentMap::CreateFromImage(grassImage);
                            std::cout << "[StartupLoading] Primary Grassland skybox GPU cubemap created on main thread." << std::endl;
                            m_grassSkyboxReady.store(true);
                        }
                        else
                        {
                            std::cerr << "[StartupLoading ERROR] Failed to load Grassland skybox image: " << grassSkyPath << std::endl;
                            m_loadingFailed = true;
                            m_errorMessage = "FAILED TO LOAD SKYBOX ASSET";
                        }
                    });
                });

                // Async decode of secondary Snow skybox in background (optional resource)
                Aion::TaskSystem::Run([this, snowSkyPath]() {
                    auto snowImage = Aion::Image::Load(snowSkyPath, true, false);
                    Aion::TaskSystem::PostToMainThread([this, snowImage]() {
                        if (snowImage && snowImage->IsValid())
                        {
                            m_snowEnvMap = Aion::EnvironmentMap::CreateFromImage(snowImage);
                            std::cout << "[StartupLoading] Secondary Snow skybox GPU cubemap initialized in background." << std::endl;
                        }
                    });
                });
            }
            else if (m_loadingPhase == LoadingPhase::LoadingSkybox)
            {
                if (m_grassSkyboxReady.load())
                {
                    if (GetRenderer() && m_grassEnvMap && m_grassEnvMap->IsValid())
                    {
                        GetRenderer()->SetEnvironmentMap(m_grassEnvMap);
                        GetRenderer()->SetShowSkybox(true);
                        GetRenderer()->SetIBLEnabled(false);
                    }

                    m_loadingProgress = 0.50f;
                    m_loadingStatus = "LOADING CHARACTER MODEL...";
                    m_loadingPhase = LoadingPhase::LoadingCharacter;
                    std::cout << "[StartupLoading] Grassland skybox ready. Now loading character model..." << std::endl;
                }
            }
            else if (m_loadingPhase == LoadingPhase::LoadingCharacter)
            {
                std::string rawCharPath = "assets/models/kenney_mini-characters/models/character-male-a.glb";
                std::string charPath = ResolveAsset(rawCharPath);
                std::cout << "[StartupLoading] Loading Kenney 3D character: " << charPath << std::endl;
                m_charModel = Aion::AssetManager::LoadModel(charPath);

                if (!m_charModel)
                {
                    std::cerr << "[StartupLoading ERROR] Failed to load character model: " << charPath << std::endl;
                    m_loadingFailed = true;
                    m_errorMessage = "FAILED TO LOAD CHARACTER MODEL";
                    return;
                }

                m_loadingProgress = 0.75f;
                m_loadingStatus = "INITIALIZING LEVEL SEGMENTS...";
                m_loadingPhase = LoadingPhase::InitializingLevel;
                std::cout << "[StartupLoading] Character model loaded. Initializing level geometry..." << std::endl;
            }
            else if (m_loadingPhase == LoadingPhase::InitializingLevel)
            {
                m_gameScene.SetupWithPreloaded(
                    GetScene(), m_config, aspect, GetRenderer(),
                    m_grassEnvMap, m_snowEnvMap, m_charModel);

                if (m_gameScene.GetPlayer())
                {
                }

                m_loadingProgress = 0.95f;
                m_loadingStatus = "FINALIZING SCENE...";
                m_loadingPhase = LoadingPhase::FinalizingScene;
                std::cout << "[StartupLoading] Level geometry initialized. Verifying scene..." << std::endl;
            }
            else if (m_loadingPhase == LoadingPhase::FinalizingScene)
            {
                if (!m_gameScene.GetPlayer() || !m_gameScene.GetCameraObject())
                {
                    std::cerr << "[StartupLoading ERROR] Scene initialization verification failed!" << std::endl;
                    m_loadingFailed = true;
                    m_errorMessage = "SCENE VERIFICATION FAILED";
                    return;
                }

                m_loadingProgress = 1.0f;
                m_loadingStatus = "READY!";
                m_loadingPhase = LoadingPhase::Ready;
                std::cout << "[StartupLoading] All required startup resources and game scene ready (100%)." << std::endl;
            }
            else if (m_loadingPhase == LoadingPhase::Ready)
            {
                m_lifecycleState = AppLifecycleState::FadingOut;
                m_fadeTimer = 0.0f;
                m_fadeDuration = 0.45f;
                m_fadeAlpha = 1.0f;
                std::cout << "[StartupLoading] Beginning smooth fade-out transition (duration: 0.45s)..." << std::endl;
            }

            return; // Gameplay input and physics updates remain blocked during startup loading
        }
        // -------------------------------------------------------------
        // 2. Fading Out State: smooth fade-out reveals the game scene
        // -------------------------------------------------------------
        else if (m_lifecycleState == AppLifecycleState::FadingOut)
        {
            m_fadeTimer += dt;
            m_fadeAlpha = 1.0f - std::clamp(m_fadeTimer / m_fadeDuration, 0.0f, 1.0f);

            // Gameplay input remains disabled during fade-out
            if (m_fadeTimer >= m_fadeDuration)
            {
                m_lifecycleState = AppLifecycleState::ActiveGameplay;
                m_fadeAlpha = 0.0f;
                std::cout << "[StartupLoading] Loading screen fade-out complete. Gameplay active." << std::endl;
                std::cout << "[BlockJumperApp] Direct Controls:" << std::endl;
                std::cout << "  - W / Up Arrow:    Move forward relative to camera" << std::endl;
                std::cout << "  - S / Down Arrow:  Move backward relative to camera" << std::endl;
                std::cout << "  - A / Left Arrow:  Move left relative to camera" << std::endl;
                std::cout << "  - D / Right Arrow: Move right relative to camera" << std::endl;
                std::cout << "  - Space:           Jump" << std::endl;
            }
            return;
        }

        // -------------------------------------------------------------
        // 3. Active Gameplay State
        // -------------------------------------------------------------
        m_gameScene.Update(dt, winW, winH);
    }

    void BlockJumperApp::OnRender()
    {
        int winW = 1280;
        int winH = 720;
        if (GetWindow())
        {
            winW = GetWindow()->GetWidth();
            winH = GetWindow()->GetHeight();
        }

        float dt = Aion::Time::GetDeltaTime();

        // 1. Startup Loading State
        if (m_lifecycleState == AppLifecycleState::StartupLoading)
        {
            m_loadingScreen.Render(
                winW, winH, 1.0f, m_loadingProgress, m_loadingStatus, m_errorMessage, dt);
            return;
        }

        // 2. Fading Out State
        if (m_lifecycleState == AppLifecycleState::FadingOut)
        {
            m_gameScene.RenderUI(winW, winH);

            m_loadingScreen.Render(
                winW, winH, m_fadeAlpha, 1.0f, "READY!", "", dt);
            return;
        }

        // 3. Active Gameplay State
        m_gameScene.RenderUI(winW, winH);
    }

    void BlockJumperApp::OnDestroy()
    {
        std::cout << "[BlockJumperApp] Shutting down cleanly." << std::endl;
        m_gameScene.Shutdown();
        m_loadingScreen.Shutdown();
        m_grassEnvMap.reset();
        m_snowEnvMap.reset();
        m_charModel.reset();
    }
} // namespace BlockJumper

