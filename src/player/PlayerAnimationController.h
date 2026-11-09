#pragma once

#include <cstddef>
#include <string>

#include "Aion/Scene/AnimationComponent.h"
#include "Aion/Scene/Component.h"
#include "player/PlayerController.h"

namespace BlockJumper
{
    enum class PlayerAnimState
    {
        Idle,
        Walk,
        Sprint,
        Jump,
        Fall,
        Landing
    };

    class PlayerAnimationController : public Aion::Component
    {
    public:
        // Verified zero-based GLB animation clip indices for character-male-a.glb
        static constexpr size_t CLIP_STATIC = 0;
        static constexpr size_t CLIP_IDLE   = 1;
        static constexpr size_t CLIP_WALK   = 2;
        static constexpr size_t CLIP_SPRINT = 3;
        static constexpr size_t CLIP_JUMP   = 4;
        static constexpr size_t CLIP_FALL   = 5;
        static constexpr size_t CLIP_CROUCH = 6;

        // Tuned transition durations in seconds
        static constexpr float TRANSITION_IDLE    = 0.15f;
        static constexpr float TRANSITION_WALK    = 0.12f;
        static constexpr float TRANSITION_JUMP    = 0.06f;
        static constexpr float TRANSITION_FALL    = 0.12f;
        static constexpr float TRANSITION_LANDING = 0.08f;

        // Kinematic velocity thresholds
        static constexpr float MOVE_ENTER_THRESHOLD = 0.25f;
        static constexpr float MOVE_EXIT_THRESHOLD  = 0.15f;
        static constexpr float LANDING_DURATION     = 0.12f;

        PlayerAnimationController() = default;

        void Initialize(PlayerController* controller, Aion::AnimationComponent* animComp);

        void OnUpdate(float deltaTime) override;

        void Reset();

        PlayerAnimState GetCurrentState() const { return m_currentState; }
        size_t GetCurrentClipIndex() const;
        static const char* GetStateName(PlayerAnimState state);

    private:
        void TransitionToState(PlayerAnimState newState);

        PlayerController* m_controller = nullptr;
        Aion::AnimationComponent* m_animComponent = nullptr;

        PlayerAnimState m_currentState = PlayerAnimState::Idle;
        float m_landingTimer = 0.0f;
    };
} // namespace BlockJumper
