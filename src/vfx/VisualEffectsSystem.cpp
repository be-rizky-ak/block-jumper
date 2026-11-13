#include "VisualEffectsSystem.h"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <vector>

#include "Aion/Renderer/RenderCommand.h"
#include "Aion/Renderer/Shader.h"
#include "Aion/Renderer/VertexArray.h"

namespace BlockJumper
{
    namespace
    {
        const char* s_VfxVertexShader = R"(#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec4 aColor;

uniform mat4 uViewProj;
out vec4 vColor;

void main()
{
    gl_Position = uViewProj * vec4(aPos, 1.0);
    vColor = aColor;
}
)";

        const char* s_VfxFragmentShader = R"(#version 330 core
in vec4 vColor;
out vec4 FragColor;

void main()
{
    FragColor = vColor;
}
)";

        // Unit cube offsets: 36 vertices (x, y, z) centered at origin
        const float s_UnitCubeVerts[108] = {
            // Front face (+Z)
            -0.5f, -0.5f,  0.5f,   0.5f, -0.5f,  0.5f,   0.5f,  0.5f,  0.5f,
            -0.5f, -0.5f,  0.5f,   0.5f,  0.5f,  0.5f,  -0.5f,  0.5f,  0.5f,
            // Back face (-Z)
             0.5f, -0.5f, -0.5f,  -0.5f, -0.5f, -0.5f,  -0.5f,  0.5f, -0.5f,
             0.5f, -0.5f, -0.5f,  -0.5f,  0.5f, -0.5f,   0.5f,  0.5f, -0.5f,
            // Top face (+Y)
            -0.5f,  0.5f,  0.5f,   0.5f,  0.5f,  0.5f,   0.5f,  0.5f, -0.5f,
            -0.5f,  0.5f,  0.5f,   0.5f,  0.5f, -0.5f,  -0.5f,  0.5f, -0.5f,
            // Bottom face (-Y)
            -0.5f, -0.5f, -0.5f,   0.5f, -0.5f, -0.5f,   0.5f, -0.5f,  0.5f,
            -0.5f, -0.5f, -0.5f,   0.5f, -0.5f,  0.5f,  -0.5f, -0.5f,  0.5f,
            // Right face (+X)
             0.5f, -0.5f,  0.5f,   0.5f, -0.5f, -0.5f,   0.5f,  0.5f, -0.5f,
             0.5f, -0.5f,  0.5f,   0.5f,  0.5f, -0.5f,   0.5f,  0.5f,  0.5f,
            // Left face (-X)
            -0.5f, -0.5f, -0.5f,  -0.5f, -0.5f,  0.5f,  -0.5f,  0.5f,  0.5f,
            -0.5f, -0.5f, -0.5f,  -0.5f,  0.5f,  0.5f,  -0.5f,  0.5f, -0.5f,
        };
    } // namespace

    VisualEffectsSystem::VisualEffectsSystem() = default;

    VisualEffectsSystem::~VisualEffectsSystem()
    {
        Shutdown();
    }

    void VisualEffectsSystem::Initialize()
    {
        if (m_initialized)
        {
            return;
        }

        m_shader = std::make_unique<Aion::Shader>(s_VfxVertexShader, s_VfxFragmentShader);
        m_vertexArray = std::make_unique<Aion::VertexArray>();
        m_vertexArray->SetLayout({3, 4}); // Position (3 floats), Color (4 floats)

        m_initialized = true;
    }

    void VisualEffectsSystem::Shutdown()
    {
        if (!m_initialized)
        {
            return;
        }

        m_vertexArray.reset();
        m_shader.reset();

        m_initialized = false;
    }

    void VisualEffectsSystem::EnsureInitialized()
    {
        if (!m_initialized)
        {
            Initialize();
        }
    }

    void VisualEffectsSystem::Reset()
    {
        for (auto& p : m_particles)
        {
            p.Active = false;
        }
        m_fallStreakTimer = 0.0f;
    }

    size_t VisualEffectsSystem::GetActiveParticleCount() const
    {
        size_t count = 0;
        for (const auto& p : m_particles)
        {
            if (p.Active) count++;
        }
        return count;
    }

    void VisualEffectsSystem::TriggerJumpDust(const Aion::Vector3& feetPosition)
    {
        // Emit 8 subtle low-poly dust block particles puffing outward and slightly up
        int spawned = 0;
        const int TARGET_SPAWN = 8;

        for (auto& p : m_particles)
        {
            if (!p.Active)
            {
                float angle = (static_cast<float>(spawned) / static_cast<float>(TARGET_SPAWN)) * 6.283185f;
                float jitter = static_cast<float>(std::rand() % 100) / 100.0f * 0.4f;
                float speed = 1.2f + jitter;

                p.Position = feetPosition + Aion::Vector3(std::cos(angle) * 0.15f, 0.05f, std::sin(angle) * 0.15f);
                p.Velocity = Aion::Vector3(std::cos(angle) * speed, 0.75f + jitter * 0.5f, std::sin(angle) * speed);
                p.Color = Aion::Vector4(0.92f, 0.90f, 0.82f, 0.70f); // Light warm dust
                p.Size = 0.10f + (static_cast<float>(std::rand() % 100) / 100.0f) * 0.04f;
                p.Lifetime = 0.22f;
                p.MaxLifetime = 0.22f;
                p.Active = true;

                spawned++;
                if (spawned >= TARGET_SPAWN)
                {
                    break;
                }
            }
        }
    }

    void VisualEffectsSystem::TriggerDoubleJumpBurst(const Aion::Vector3& feetPosition)
    {
        // Emit 10 energetic glowing particles with downward thrust in mid-air
        int spawned = 0;
        const int TARGET_SPAWN = 10;

        for (auto& p : m_particles)
        {
            if (!p.Active)
            {
                float angle = (static_cast<float>(spawned) / static_cast<float>(TARGET_SPAWN)) * 6.283185f;
                float speed = 1.8f + (static_cast<float>(std::rand() % 100) / 100.0f) * 0.5f;

                p.Position = feetPosition + Aion::Vector3(std::cos(angle) * 0.12f, -0.05f, std::sin(angle) * 0.12f);
                p.Velocity = Aion::Vector3(std::cos(angle) * speed, -0.8f, std::sin(angle) * speed);
                p.Color = Aion::Vector4(0.40f, 0.85f, 1.0f, 0.85f); // Bright energetic cyan
                p.Size = 0.12f + (static_cast<float>(std::rand() % 100) / 100.0f) * 0.05f;
                p.Lifetime = 0.25f;
                p.MaxLifetime = 0.25f;
                p.Active = true;

                spawned++;
                if (spawned >= TARGET_SPAWN)
                {
                    break;
                }
            }
        }
    }

    void VisualEffectsSystem::TriggerLandingBurst(const Aion::Vector3& feetPosition, float landingVelocity)
    {
        // Emit 12 particles radiating outward along platform surface in a ring
        int spawned = 0;
        const int TARGET_SPAWN = 12;
        float speedScale = std::min(2.0f, std::max(0.8f, std::abs(landingVelocity) * 0.25f));

        for (auto& p : m_particles)
        {
            if (!p.Active)
            {
                float angle = (static_cast<float>(spawned) / static_cast<float>(TARGET_SPAWN)) * 6.283185f;
                float jitter = static_cast<float>(std::rand() % 100) / 100.0f * 0.5f;
                float speed = (2.0f + jitter) * speedScale;

                p.Position = feetPosition + Aion::Vector3(std::cos(angle) * 0.20f, 0.04f, std::sin(angle) * 0.20f);
                p.Velocity = Aion::Vector3(std::cos(angle) * speed, 0.35f, std::sin(angle) * speed);
                p.Color = Aion::Vector4(0.85f, 0.88f, 0.84f, 0.80f); // Soft impact cloud
                p.Size = 0.12f + (static_cast<float>(std::rand() % 100) / 100.0f) * 0.05f;
                p.Lifetime = 0.28f;
                p.MaxLifetime = 0.28f;
                p.Active = true;

                spawned++;
                if (spawned >= TARGET_SPAWN)
                {
                    break;
                }
            }
        }
    }

    void VisualEffectsSystem::TriggerFallingStreak(const Aion::Vector3& playerPosition)
    {
        m_fallStreakTimer += 0.016f;
        if (m_fallStreakTimer < 0.04f)
        {
            return;
        }
        m_fallStreakTimer = 0.0f;

        // Spawn 2 trailing upward streak particles
        int spawned = 0;
        for (auto& p : m_particles)
        {
            if (!p.Active)
            {
                float rx = (static_cast<float>(std::rand() % 100) / 50.0f - 1.0f) * 0.35f;
                float rz = (static_cast<float>(std::rand() % 100) / 50.0f - 1.0f) * 0.35f;

                p.Position = playerPosition + Aion::Vector3(rx, 0.5f, rz);
                p.Velocity = Aion::Vector3(rx * 0.5f, 2.8f, rz * 0.5f); // Upward air streak
                p.Color = Aion::Vector4(0.70f, 0.82f, 0.95f, 0.55f);   // Subtle cyan/wind tint
                p.Size = 0.08f;
                p.Lifetime = 0.20f;
                p.MaxLifetime = 0.20f;
                p.Active = true;

                spawned++;
                if (spawned >= 2) break;
            }
        }
    }

    void VisualEffectsSystem::Update(float deltaTime)
    {
        if (deltaTime <= 0.0f) return;
        if (deltaTime > 0.05f) deltaTime = 0.05f;

        for (auto& p : m_particles)
        {
            if (!p.Active) continue;

            p.Lifetime -= deltaTime;
            if (p.Lifetime <= 0.0f)
            {
                p.Active = false;
                continue;
            }

            // Kinematics update
            p.Position = p.Position + p.Velocity * deltaTime;

            // Soft drag and gravity
            p.Velocity.x *= 0.88f;
            p.Velocity.z *= 0.88f;
            p.Velocity.y -= 1.8f * deltaTime;
        }
    }

    void VisualEffectsSystem::Render(const Aion::Matrix4& viewProjection)
    {
        size_t activeCount = GetActiveParticleCount();
        if (activeCount == 0)
        {
            return;
        }

        EnsureInitialized();

        Aion::RenderState prevState = Aion::RenderCommand::GetCurrentState();
        Aion::RenderState vfxState = prevState;
        vfxState.BlendEnable = true;
        vfxState.SrcBlend = Aion::BlendFactor::SrcAlpha;
        vfxState.DstBlend = Aion::BlendFactor::OneMinusSrcAlpha;
        vfxState.DepthTest = true;
        vfxState.DepthWrite = false; // Don't occlude geometry with transparent dust
        vfxState.CullingMode = Aion::CullMode::Back;
        Aion::RenderCommand::ApplyState(vfxState);

        std::vector<float> vertexBuffer;
        vertexBuffer.reserve(activeCount * 36 * 7);

        for (const auto& p : m_particles)
        {
            if (!p.Active) continue;

            float lifeRatio = std::max(0.0f, std::min(1.0f, p.Lifetime / p.MaxLifetime));
            float alpha = p.Color.w * lifeRatio;
            float currentSize = p.Size * (0.5f + 0.5f * lifeRatio);

            float r = p.Color.x;
            float g = p.Color.y;
            float b = p.Color.z;

            // Generate 36 vertices for the small particle cube
            for (int i = 0; i < 36; ++i)
            {
                float vx = p.Position.x + s_UnitCubeVerts[i * 3 + 0] * currentSize;
                float vy = p.Position.y + s_UnitCubeVerts[i * 3 + 1] * currentSize;
                float vz = p.Position.z + s_UnitCubeVerts[i * 3 + 2] * currentSize;

                // Subtle top/side shading
                float shade = 1.0f;
                if (s_UnitCubeVerts[i * 3 + 1] > 0.4f) shade = 1.15f; // top
                else if (s_UnitCubeVerts[i * 3 + 1] < -0.4f) shade = 0.70f; // bottom
                else shade = 0.90f; // sides

                vertexBuffer.push_back(vx);
                vertexBuffer.push_back(vy);
                vertexBuffer.push_back(vz);
                vertexBuffer.push_back(std::min(1.0f, r * shade));
                vertexBuffer.push_back(std::min(1.0f, g * shade));
                vertexBuffer.push_back(std::min(1.0f, b * shade));
                vertexBuffer.push_back(alpha);
            }
        }

        if (!vertexBuffer.empty())
        {
            m_shader->Use();
            m_shader->SetMat4("uViewProj", viewProjection);
            m_vertexArray->SetDynamicVertexBuffer(
                vertexBuffer.data(),
                static_cast<uint32_t>(vertexBuffer.size() * sizeof(float)));
            m_vertexArray->DrawArrays(
                static_cast<uint32_t>(vertexBuffer.size() / 7),
                Aion::PrimitiveType::Triangles);
        }

        // Restore renderer state
        Aion::RenderCommand::ApplyState(prevState);
    }
} // namespace BlockJumper
