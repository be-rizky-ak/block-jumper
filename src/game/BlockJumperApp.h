#pragma once

#include <atomic>
#include <memory>
#include <string>

#include "Aion/Core/Application.h"
#include "Aion/Renderer/EnvironmentMap.h"
#include "game/GameConfig.h"
#include "game/GameScene.h"
#include "ui/LoadingScreen.h"

namespace Aion
{
    class Model;
}

namespace BlockJumper
{
    enum class AppLifecycleState
    {
        StartupLoading,
        FadingOut,
        ActiveGameplay
    };

    enum class LoadingPhase
    {
        Begin,
        LoadingSkybox,
        LoadingCharacter,
        InitializingLevel,
        FinalizingScene,
        Ready,
        Failed
    };

    class BlockJumperApp : public Aion::Application
    {
    public:
        BlockJumperApp();
        ~BlockJumperApp() override;

        void OnCreate() override;
        void OnUpdate() override;
        void OnRender() override;
        void OnDestroy() override;

        

    private:
        

    private:
        GameConfig m_config;
        GameScene m_gameScene;
        LoadingScreen m_loadingScreen;

        AppLifecycleState m_lifecycleState = AppLifecycleState::StartupLoading;
        LoadingPhase m_loadingPhase = LoadingPhase::Begin;

        float m_loadingProgress = 0.05f;
        std::string m_loadingStatus = "PREPARING APPLICATION...";
        std::string m_errorMessage;
        bool m_loadingFailed = false;

        float m_fadeTimer = 0.0f;
        float m_fadeDuration = 0.45f;
        float m_fadeAlpha = 1.0f;

        std::shared_ptr<Aion::EnvironmentMap> m_grassEnvMap;
        std::shared_ptr<Aion::EnvironmentMap> m_snowEnvMap;
        std::shared_ptr<Aion::Model> m_charModel;
        std::atomic<bool> m_grassSkyboxReady{false};

        };
} // namespace BlockJumper

