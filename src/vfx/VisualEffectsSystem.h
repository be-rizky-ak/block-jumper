#pragma once

#include <array>
#include <cstdint>
#include <memory>
#include <vector>

#include "Aion/Math/Matrix4.h"
#include "Aion/Math/Vector3.h"
#include "Aion/Math/Vector4.h"

namespace Aion
{
    class Shader;
    class VertexArray;
}

namespace BlockJumper
{
    /**
     * @brief Low-poly block-styled visual effects system for Block Jumper.
     * Manages jump dust puffs, landing bursts, and falling trails with a
     * fixed preallocated particle pool (zero per-frame dynamic allocations).
     */
    class VisualEffectsSystem
    {
    public:
        struct Particle
        {
            Aion::Vector3 Position{0.0f};
            Aion::Vector3 Velocity{0.0f};
            Aion::Vector4 Color{1.0f};
            float Size = 0.15f;
            float Lifetime = 0.0f;
            float MaxLifetime = 0.25f;
            bool Active = false;
        };

        static constexpr size_t MAX_PARTICLES = 128;

    public:
        VisualEffectsSystem();
        ~VisualEffectsSystem();

        void Initialize();
        void Shutdown();

        void Update(float deltaTime);
        void Render(const Aion::Matrix4& viewProjection);

        /**
         * @brief Spawns a subtle low-poly dust puff at the player's feet upon takeoff.
         */
        void TriggerJumpDust(const Aion::Vector3& feetPosition);

        /**
         * @brief Spawns an energetic mid-air burst beneath the player upon double jumping.
         */
        void TriggerDoubleJumpBurst(const Aion::Vector3& feetPosition);

        /**
         * @brief Spawns an expanding horizontal dust burst upon touching down on a platform.
         */
        void TriggerLandingBurst(const Aion::Vector3& feetPosition, float landingVelocity = 1.0f);

        /**
         * @brief Spawns subtle wind/speed streaks during the falling state.
         */
        void TriggerFallingStreak(const Aion::Vector3& playerPosition);

        /**
         * @brief Clears all active particles immediately (e.g. on run reset).
         */
        void Reset();

        size_t GetActiveParticleCount() const;

    private:
        void EnsureInitialized();

    private:
        bool m_initialized = false;
        std::unique_ptr<Aion::Shader> m_shader;
        std::unique_ptr<Aion::VertexArray> m_vertexArray;

        std::array<Particle, MAX_PARTICLES> m_particles{};
        float m_fallStreakTimer = 0.0f;
    };
} // namespace BlockJumper
