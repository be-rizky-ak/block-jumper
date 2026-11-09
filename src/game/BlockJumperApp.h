#pragma once

#include <memory>
#include <string>

#include "Aion/Core/Application.h"
#include "game/GameConfig.h"
#include "game/GameScene.h"

namespace BlockJumper
{
    class BlockJumperApp : public Aion::Application
    {
    public:
        BlockJumperApp();
        ~BlockJumperApp() override = default;

        void OnCreate() override;
        void OnUpdate() override;
        void OnRender() override;
        void OnDestroy() override;

        

    private:
        

    private:
        GameConfig m_config;
        GameScene m_gameScene;

        };
} // namespace BlockJumper

