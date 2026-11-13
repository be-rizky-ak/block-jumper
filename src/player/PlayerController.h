#pragma once

#include <vector>
#include "Aion/Math/Vector3.h"
#include "Aion/Scene/Component.h"
#include "game/GameConfig.h"
#include "player/PlayerCollider.h"

namespace BlockJumper
{
    class PlayerController : public Aion::Component
    {
    public:
        PlayerController();
        PlayerController(
            const PlayerConfig& config,
            const std::vector<PlatformBox>& platforms,
            float cameraYawDegrees = 45.0f);

        void OnUpdate(float deltaTime) override;

        // Kinematics and state queries
        const Aion::Vector3& GetHorizontalVelocity() const { return m_horizontalVelocity; }
        float GetSpeed() const;
        float GetVerticalVelocity() const { return m_verticalVelocity; }
        bool IsGrounded() const { return m_isGrounded; }
        bool JustJumped() const { return m_justJumped; }
        bool JustDoubleJumped() const { return m_justDoubleJumped; }
        bool JustLanded() const { return m_justLanded; }
        bool IsFalling() const { return !m_isGrounded && m_verticalVelocity < 0.0f; }
        bool IsAscending() const { return !m_isGrounded && m_verticalVelocity > 0.0f; }
        int GetJumpCount() const { return m_jumpCount; }
        bool CanDoubleJump() const { return !m_isGrounded && m_jumpCount < 2; }

        const PlatformBox* GetGroundedPlatform() const { return m_groundedPlatform; }
        void SetGroundedPlatform(const PlatformBox* plat) { m_groundedPlatform = plat; }

        const PlayerConfig& GetConfig() const { return m_config; }
        void SetConfig(const PlayerConfig& config) { m_config = config; }

        const std::vector<PlatformBox>& GetPlatforms() const { return m_platforms; }
        void SetPlatforms(const std::vector<PlatformBox>& platforms) { m_platforms = platforms; }

        float GetCameraYaw() const { return m_cameraYawDegrees; }
        void SetCameraYaw(float yawDegrees) { m_cameraYawDegrees = yawDegrees; }

        const PlayerCollider& GetCollider() const { return m_collider; }

        // Control safety and simulation freeze
        void SetInputEnabled(bool enabled) { m_inputEnabled = enabled; }
        bool IsInputEnabled() const { return m_inputEnabled; }

        void SetFrozen(bool frozen) { m_frozen = frozen; }
        bool IsFrozen() const { return m_frozen; }

        /**
         * @brief Applies carried displacement from a moving platform the player is standing on.
         */
        void ApplyCarriedOffset(const Aion::Vector3& offset);

        /**
         * @brief Completely resets player kinematics, velocities, fall state, and input flags.
         */
        void ResetKinematics(const Aion::Vector3& spawnPos, float facingYaw = -135.0f);

        // Touch input integration
        void SetTouchInput(float forwardAxis, float rightAxis, bool jumpEdge)
        {
            m_touchForward = std::clamp(forwardAxis, -1.0f, 1.0f);
            m_touchRight = std::clamp(rightAxis, -1.0f, 1.0f);
            if (jumpEdge)
            {
                m_touchJumpEdge = true;
            }
        }
        void ClearTouchInput()
        {
            m_touchForward = 0.0f;
            m_touchRight = 0.0f;
            m_touchJumpEdge = false;
        }

        // Simulation override for automated verification/testing
        // forwardAxis: +1 (W / forward relative to camera), -1 (S / backward)
        // rightAxis:   +1 (D / right relative to camera),   -1 (A / left)
        void SetSimulatedInput(bool overrideInput, float forwardAxis, float rightAxis, bool jumpPressed);

    private:
        PlayerConfig m_config;
        std::vector<PlatformBox> m_platforms;
        PlayerCollider m_collider;
        float m_cameraYawDegrees = 45.0f;

        Aion::Vector3 m_horizontalVelocity{0.0f};
        float m_verticalVelocity = 0.0f;
        bool m_isGrounded = true;
        bool m_justJumped = false;
        bool m_justDoubleJumped = false;
        bool m_justLanded = false;
        int m_jumpCount = 0;
        bool m_jumpWasPressed = false;
        const PlatformBox* m_groundedPlatform = nullptr;

        bool m_inputEnabled = true;
        bool m_frozen = false;

        bool m_useSimulatedInput = false;
        float m_simulatedForward = 0.0f;
        float m_simulatedRight = 0.0f;
        bool m_simulatedJump = false;

        float m_touchForward = 0.0f;
        float m_touchRight = 0.0f;
        bool m_touchJumpEdge = false;
    };
} // namespace BlockJumper
