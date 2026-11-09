#include "BlockJumperApp.h"

#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <vector>

#include "Aion/Core/Time.h"
#include "Aion/Core/Window.h"
#include "Aion/Renderer/EnvironmentMap.h"
#include "Aion/Renderer/Renderer.h"
#include "glad/glad.h"

namespace BlockJumper
{
    BlockJumperApp::BlockJumperApp()
        : Aion::Application(Aion::ApplicationSpecification{
            .Name = "Block Jumper - Isometric 2.5D",
            .Width = 1280,
            .Height = 720,
            .VSync = true
        })
    {
    }

    void BlockJumperApp::OnCreate()
    {
        std::cout << "========================================" << std::endl;
        std::cout << " Block Jumper - Isometric 2.5D Prototype" << std::endl;
        std::cout << "========================================" << std::endl;

        // Configure polished directional lighting to clearly illuminate platform top surfaces
        // and create distinct 3-tone shading across visible vertical sides
        if (GetRenderer())
        {
            // Consistent world-space directional sunlight vector pointing towards light source.
            // Platform top surfaces (N = 0, 1, 0) receive direct illumination (dot ~ 0.80);
            // Camera-facing +X vertical sides receive medium direct illumination (dot ~ 0.55);
            // Camera-facing +Z vertical sides receive soft direct illumination (dot ~ 0.25).
            // Hidden sides (-X, -Z, -Y) receive ambient illumination only.
            GetRenderer()->SetLightDirection(Aion::Vector3(0.55f, 0.80f, 0.25f).Normalized());

            // Scaled directional sunlight radiance and balanced ambient to compensate for
            // PBR diffuse division by PI in standard.frag, preventing washed out or dark surfaces
            GetRenderer()->SetLightColor(Aion::Vector3(2.2f, 2.15f, 2.05f));
            GetRenderer()->SetAmbientColor(Aion::Vector3(0.28f, 0.29f, 0.34f));

            // Enable shadow mapping with tuned bias
            GetRenderer()->SetShadowsEnabled(true);
            GetRenderer()->SetShadowBias(0.0025f, 0.0005f);
        }

        float aspect = 1280.0f / 720.0f;
        if (GetWindow())
        {
            aspect = static_cast<float>(GetWindow()->GetWidth()) / static_cast<float>(GetWindow()->GetHeight());
        }

        m_gameScene.Setup(GetScene(), m_config, aspect);

        if (m_gameScene.GetPlayer())
        {
        }

        std::cout << "[BlockJumperApp] Direct Controls:" << std::endl;
        std::cout << "  - W / Up Arrow:    Move forward relative to camera" << std::endl;
        std::cout << "  - S / Down Arrow:  Move backward relative to camera" << std::endl;
        std::cout << "  - A / Left Arrow:  Move left relative to camera" << std::endl;
        std::cout << "  - D / Right Arrow: Move right relative to camera" << std::endl;
        std::cout << "  - Space:           Jump" << std::endl;
    }

    void BlockJumperApp::OnUpdate()
    {
        float dt = Aion::Time::GetDeltaTime();
        if (dt > 0.05f)
        {
            dt = 0.05f; // Clamp delta time to avoid simulation jumps during screenshot disk I/O
        }

        m_gameScene.Update(dt);
    }

    void BlockJumperApp::OnRender()
    {
    }

    void BlockJumperApp::OnDestroy()
    {
        std::cout << "[BlockJumperApp] Shutting down cleanly." << std::endl;
    }
} // namespace BlockJumper

