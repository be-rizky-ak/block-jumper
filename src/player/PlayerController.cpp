#include "PlayerController.h"

#include <algorithm>
#include <cmath>
#include <iostream>

#include "Aion/Core/Input.h"
#include "Aion/Core/KeyCodes.h"
#include "Aion/Math/Math.h"
#include "Aion/Math/Quaternion.h"
#include "Aion/Scene/Object3D.h"

namespace BlockJumper
{
    PlayerController::PlayerController()
        : m_config(PlayerConfig{}),
          m_platforms(GameConfig::CreateDefaultLevel()),
          m_collider(m_config.HalfExtents),
          m_cameraYawDegrees(45.0f)
    {
    }

    PlayerController::PlayerController(
        const PlayerConfig& config,
        const std::vector<PlatformBox>& platforms,
        float cameraYawDegrees)
        : m_config(config),
          m_platforms(platforms),
          m_collider(config.HalfExtents),
          m_cameraYawDegrees(cameraYawDegrees)
    {
    }

    float PlayerController::GetSpeed() const
    {
        return std::sqrt(m_horizontalVelocity.x * m_horizontalVelocity.x +
                         m_horizontalVelocity.z * m_horizontalVelocity.z);
    }

    void PlayerController::SetSimulatedInput(
        bool overrideInput,
        float forwardAxis,
        float rightAxis,
        bool jumpPressed)
    {
        m_useSimulatedInput = overrideInput;
        m_simulatedForward = std::clamp(forwardAxis, -1.0f, 1.0f);
        m_simulatedRight = std::clamp(rightAxis, -1.0f, 1.0f);
        m_simulatedJump = jumpPressed;
    }

    void PlayerController::ApplyCarriedOffset(const Aion::Vector3& offset)
    {
        Aion::Object3D* owner = GetOwner();
        if (owner)
        {
            owner->Transform.Position = owner->Transform.Position + offset;
        }
    }

    void PlayerController::ResetKinematics(const Aion::Vector3& spawnPos, float facingYaw)
    {
        m_horizontalVelocity = Aion::Vector3(0.0f);
        m_verticalVelocity = 0.0f;
        m_isGrounded = true;
        m_justJumped = false;
        m_justDoubleJumped = false;
        m_justLanded = false;
        m_jumpCount = 0;
        m_jumpWasPressed = false;
        m_groundedPlatform = nullptr;
        m_inputEnabled = true;
        m_frozen = false;
        m_useSimulatedInput = false;
        m_simulatedForward = 0.0f;
        m_simulatedRight = 0.0f;
        m_simulatedJump = false;
        m_touchForward = 0.0f;
        m_touchRight = 0.0f;
        m_touchJumpEdge = false;

        Aion::Object3D* owner = GetOwner();
        if (owner)
        {
            owner->Transform.Position = spawnPos;
            owner->Transform.Rotation = Aion::Quaternion::FromEuler(0.0f, Aion::Math::Radians(facingYaw), 0.0f);
        }
    }

    void PlayerController::OnUpdate(float deltaTime)
    {
        if (m_frozen)
        {
            return;
        }

        if (deltaTime <= 0.0f)
        {
            return;
        }
        if (deltaTime > 0.05f)
        {
            deltaTime = 0.05f;
        }

        Aion::Object3D* owner = GetOwner();
        if (!owner)
        {
            return;
        }

        Aion::Vector3 currentPosition = owner->Transform.Position;
        float previousY = currentPosition.y;
        bool wasGrounded = m_isGrounded;
        m_justJumped = false;
        m_justDoubleJumped = false;
        m_justLanded = false;

        // -------------------------------------------------------------
        // 1. Direct Camera-Relative Movement
        // No automatic forward movement. Player moves only upon input.
        // W/S: Forward/Backward relative to camera.
        // A/D: Left/Right relative to camera.
        // -------------------------------------------------------------
        float forwardInput = 0.0f;
        float rightInput = 0.0f;

        if (m_inputEnabled)
        {
            if (m_useSimulatedInput)
            {
                forwardInput = m_simulatedForward;
                rightInput = m_simulatedRight;
            }
            else
            {
                // Keyboard inputs
                float kbdForward = 0.0f;
                float kbdRight = 0.0f;

                // Forward (W / Up) and Backward (S / Down)
                if (Aion::Input::GetKey(Aion::Key::W) || Aion::Input::GetKey(Aion::Key::Up))
                {
                    kbdForward += 1.0f;
                }
                if (Aion::Input::GetKey(Aion::Key::S) || Aion::Input::GetKey(Aion::Key::Down))
                {
                    kbdForward -= 1.0f;
                }

                // Right (D / Right) and Left (A / Left)
                if (Aion::Input::GetKey(Aion::Key::D) || Aion::Input::GetKey(Aion::Key::Right))
                {
                    kbdRight += 1.0f;
                }
                if (Aion::Input::GetKey(Aion::Key::A) || Aion::Input::GetKey(Aion::Key::Left))
                {
                    kbdRight -= 1.0f;
                }

                // Combine keyboard and touch input seamlessly
                forwardInput = std::clamp(kbdForward + m_touchForward, -1.0f, 1.0f);
                rightInput = std::clamp(kbdRight + m_touchRight, -1.0f, 1.0f);
            }
        }

        // Project movement directions relative to camera viewing yaw angle
        float yawRad = Aion::Math::Radians(m_cameraYawDegrees);
        // Forward relative to camera in horizontal XZ plane
        Aion::Vector3 camFwd(-std::sin(yawRad), 0.0f, -std::cos(yawRad));
        // Right relative to camera in horizontal XZ plane
        Aion::Vector3 camRight(std::cos(yawRad), 0.0f, -std::sin(yawRad));

        Aion::Vector3 moveDir = camFwd * forwardInput + camRight * rightInput;
        float inputMagnitude = std::sqrt(moveDir.x * moveDir.x + moveDir.z * moveDir.z);

        if (inputMagnitude > 0.001f)
        {
            moveDir = moveDir * (1.0f / inputMagnitude);
            float speedScale = std::min(inputMagnitude, 1.0f);
            Aion::Vector3 targetVelocity = moveDir * (m_config.MoveSpeed * speedScale);

            // Accelerate smoothly towards target velocity
            Aion::Vector3 velDiff = targetVelocity - m_horizontalVelocity;
            float diffLen = std::sqrt(velDiff.x * velDiff.x + velDiff.z * velDiff.z);
            float maxStep = m_config.Acceleration * deltaTime;

            if (diffLen <= maxStep)
            {
                m_horizontalVelocity = targetVelocity;
            }
            else
            {
                m_horizontalVelocity = m_horizontalVelocity + (velDiff * (maxStep / diffLen));
            }
        }
        else
        {
            // Decelerate smoothly towards zero
            float currentSpeed = GetSpeed();
            float maxDecel = m_config.Deceleration * deltaTime;

            if (currentSpeed <= maxDecel)
            {
                m_horizontalVelocity = Aion::Vector3(0.0f);
            }
            else
            {
                m_horizontalVelocity = m_horizontalVelocity * ((currentSpeed - maxDecel) / currentSpeed);
            }
        }

        // Apply horizontal movement
        currentPosition = currentPosition + m_horizontalVelocity * deltaTime;

        // -------------------------------------------------------------
        // 2. Jump and Gravity (Ground Jump & Double Jump)
        // Space key triggers jump: 1st from ground, 2nd while airborne.
        // Third jump is prevented. Holding space does not re-trigger.
        // -------------------------------------------------------------
        bool jumpRequested = false;
        if (m_inputEnabled)
        {
            if (m_useSimulatedInput)
            {
                jumpRequested = m_simulatedJump;
            }
            else
            {
                bool kbdJump = Aion::Input::GetKey(Aion::Key::Space);
                bool touchJump = m_touchJumpEdge;
                m_touchJumpEdge = false; // Consume jump edge
                jumpRequested = kbdJump || touchJump;
            }
        }

        bool jumpPressedThisFrame = jumpRequested && !m_jumpWasPressed;
        m_jumpWasPressed = jumpRequested;

        if (m_isGrounded)
        {
            if (jumpPressedThisFrame)
            {
                m_verticalVelocity = m_config.JumpVelocity;
                m_isGrounded = false;
                m_justJumped = true;
                m_jumpCount = 1;
                m_groundedPlatform = nullptr;
            }
        }
        else
        {
            if (jumpPressedThisFrame && m_jumpCount < 2)
            {
                m_verticalVelocity = m_config.JumpVelocity;
                m_justJumped = true;
                m_justDoubleJumped = true;
                m_jumpCount = 2;
                m_groundedPlatform = nullptr;
            }
        }

        if (!m_isGrounded)
        {
            m_verticalVelocity -= m_config.Gravity * deltaTime;
            if (m_verticalVelocity < -30.0f)
            {
                m_verticalVelocity = -30.0f; // Terminal velocity
            }
        }

        // Apply vertical movement
        currentPosition.y += m_verticalVelocity * deltaTime;

        // -------------------------------------------------------------
        // 3. Multi-Platform Collision and Landing Detection
        // -------------------------------------------------------------
        m_collider.ResolveCollisions(
            currentPosition,
            m_horizontalVelocity,
            m_verticalVelocity,
            m_isGrounded,
            m_platforms,
            previousY,
            deltaTime,
            &m_groundedPlatform);

        if (m_isGrounded)
        {
            m_jumpCount = 0; // Landing resets both jumps
        }

        if (!wasGrounded && m_isGrounded)
        {
            m_justLanded = true;
        }

        // -------------------------------------------------------------
        // 4. Void Bound Safety Clamp
        // (Fall detection and run reset are governed by GameScene state flow)
        // -------------------------------------------------------------
        if (currentPosition.y < m_config.VoidFallLimit - 10.0f)
        {
            currentPosition.y = m_config.VoidFallLimit - 10.0f;
            m_verticalVelocity = 0.0f;
        }

        // -------------------------------------------------------------
        // 5. Update Visual Facing Direction (Orient character to movement)
        // -------------------------------------------------------------
        if (GetSpeed() > 0.2f)
        {
            float facingYaw = std::atan2(-m_horizontalVelocity.x, -m_horizontalVelocity.z);
            Aion::Quaternion targetRot = Aion::Quaternion::FromEuler(0.0f, facingYaw, 0.0f);
            float turnRate = 18.0f * deltaTime;
            owner->Transform.Rotation = Aion::Quaternion::Slerp(
                owner->Transform.Rotation,
                targetRot,
                std::clamp(turnRate, 0.0f, 1.0f)
            ).Normalized();
        }

        owner->Transform.Position = currentPosition;
    }
} // namespace BlockJumper
