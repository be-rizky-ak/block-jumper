#include "PlayerAnimationController.h"

#include <algorithm>
#include <iostream>

#include "Aion/Scene/Object3D.h"

namespace BlockJumper
{
    void PlayerAnimationController::Initialize(PlayerController* controller, Aion::AnimationComponent* animComp)
    {
        m_controller = controller;
        m_animComponent = animComp;
        Reset();
    }

    void PlayerAnimationController::Reset()
    {
        m_currentState = PlayerAnimState::Idle;
        m_landingTimer = 0.0f;

        if (m_animComponent)
        {
            m_animComponent->Reset();
            m_animComponent->SetLooping(true);
            m_animComponent->SetPlaybackSpeed(1.0f);
            m_animComponent->PlayClip(CLIP_IDLE);
        }
    }

    size_t PlayerAnimationController::GetCurrentClipIndex() const
    {
        if (m_animComponent)
        {
            return m_animComponent->IsTransitioning() ? m_animComponent->GetTargetClip() : m_animComponent->GetActiveClip();
        }
        return CLIP_STATIC;
    }

    const char* PlayerAnimationController::GetStateName(PlayerAnimState state)
    {
        switch (state)
        {
            case PlayerAnimState::Idle:    return "Idle";
            case PlayerAnimState::Walk:    return "Walk";
            case PlayerAnimState::Sprint:  return "Sprint";
            case PlayerAnimState::Jump:    return "Jump";
            case PlayerAnimState::Fall:    return "Fall";
            case PlayerAnimState::Landing: return "Landing";
            default:                       return "Unknown";
        }
    }

    void PlayerAnimationController::TransitionToState(PlayerAnimState newState)
    {
        if (!m_animComponent)
        {
            m_currentState = newState;
            return;
        }

        m_currentState = newState;

        // Ensure any active transition is terminated before dispatching high-priority state
        if (m_animComponent->IsTransitioning())
        {
            m_animComponent->StopTransition(false);
        }

        switch (newState)
        {
            case PlayerAnimState::Idle:
                m_animComponent->SetLooping(true);
                m_animComponent->TransitionTo(CLIP_IDLE, TRANSITION_IDLE);
                break;

            case PlayerAnimState::Walk:
                m_animComponent->SetLooping(true);
                m_animComponent->TransitionTo(CLIP_WALK, TRANSITION_WALK);
                break;

            case PlayerAnimState::Sprint:
                m_animComponent->SetLooping(true);
                m_animComponent->TransitionTo(CLIP_SPRINT, TRANSITION_WALK);
                break;

            case PlayerAnimState::Jump:
                m_animComponent->SetLooping(false);
                m_animComponent->TransitionTo(CLIP_JUMP, TRANSITION_JUMP);
                break;

            case PlayerAnimState::Fall:
                m_animComponent->SetLooping(true);
                m_animComponent->TransitionTo(CLIP_FALL, TRANSITION_FALL);
                break;

            case PlayerAnimState::Landing:
                m_animComponent->SetLooping(false);
                m_animComponent->TransitionTo(CLIP_CROUCH, TRANSITION_LANDING);
                break;
        }
    }

    void PlayerAnimationController::OnUpdate(float deltaTime)
    {
        if (!m_controller && GetOwner())
        {
            m_controller = GetOwner()->GetComponent<PlayerController>();
        }
        if (!m_animComponent && GetOwner())
        {
            m_animComponent = GetOwner()->GetComponent<Aion::AnimationComponent>();
        }

        if (!m_controller || !m_animComponent)
        {
            return;
        }

        // If the simulation is frozen (e.g. GameOver), maintain current pose and speed
        if (m_controller->IsFrozen())
        {
            m_animComponent->SetPlaybackSpeed(0.0f);
            return;
        }

        bool isGrounded = m_controller->IsGrounded();
        float speed = m_controller->GetSpeed();
        float vy = m_controller->GetVerticalVelocity();
        bool justJumped = m_controller->JustJumped();
        bool justLanded = m_controller->JustLanded();
        bool isAscending = m_controller->IsAscending();

        PlayerAnimState desiredState = m_currentState;

        if (!isGrounded)
        {
            // Airborne states: distinguish ascending takeoff vs descending fall
            if (justJumped || isAscending)
            {
                desiredState = PlayerAnimState::Jump;
            }
            else if (vy <= 0.0f)
            {
                desiredState = PlayerAnimState::Fall;
            }
        }
        else
        {
            // Grounded states
            if (justLanded)
            {
                // If the player lands while actively running, transition straight into Walk without pause
                if (speed > MOVE_ENTER_THRESHOLD)
                {
                    desiredState = PlayerAnimState::Walk;
                }
                else
                {
                    desiredState = PlayerAnimState::Landing;
                    m_landingTimer = LANDING_DURATION;
                }
            }
            else if (m_currentState == PlayerAnimState::Landing)
            {
                m_landingTimer -= deltaTime;
                if (speed > MOVE_ENTER_THRESHOLD)
                {
                    // Immediate movement cancels remaining landing dip
                    desiredState = PlayerAnimState::Walk;
                }
                else if (m_landingTimer <= 0.0f)
                {
                    desiredState = PlayerAnimState::Idle;
                }
                else
                {
                    desiredState = PlayerAnimState::Landing;
                }
            }
            else
            {
                // Normal grounded idle vs walk selection with hysteresis to prevent rapid flickering
                if (speed > MOVE_ENTER_THRESHOLD)
                {
                    desiredState = PlayerAnimState::Walk;
                }
                else if (speed < MOVE_EXIT_THRESHOLD)
                {
                    desiredState = PlayerAnimState::Idle;
                }
                else
                {
                    desiredState = m_currentState;
                }
            }
        }

        if (desiredState != m_currentState)
        {
            TransitionToState(desiredState);
        }

        // Dynamically adjust Walk playback speed proportional to movement velocity to minimize foot sliding
        if (m_currentState == PlayerAnimState::Walk)
        {
            float baseSpeed = m_controller->GetConfig().MoveSpeed;
            float normSpeed = (baseSpeed > 0.01f) ? (speed / baseSpeed) : 1.0f;
            float playbackSpeed = std::clamp(normSpeed * 1.15f, 0.6f, 1.4f);
            m_animComponent->SetPlaybackSpeed(playbackSpeed);
        }
        else
        {
            m_animComponent->SetPlaybackSpeed(1.0f);
        }
    }
} // namespace BlockJumper
