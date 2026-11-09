#pragma once

#include <memory>

#include "Aion/Math/Quaternion.h"
#include "Aion/Math/Vector3.h"
#include "Aion/Scene/Component.h"
#include "Aion/Scene/Object3D.h"
#include "game/GameConfig.h"

namespace BlockJumper
{
    /**
     * @brief Elevated isometric camera controller.
     * Maintains a fixed diagonal 2.5D viewing angle while smoothly tracking the player.
     * Exposes top surfaces and visible sides of platforms and objects.
     */
    class FollowCamera : public Aion::Component
    {
    public:
        FollowCamera();
        explicit FollowCamera(const CameraConfig& config);

        void SetTarget(std::shared_ptr<Aion::Object3D> target);
        std::shared_ptr<Aion::Object3D> GetTarget() const { return m_target; }

        void SnapToTarget();

        void OnUpdate(float deltaTime) override;

        const CameraConfig& GetConfig() const { return m_config; }
        void SetConfig(const CameraConfig& config) { m_config = config; }

    private:
        CameraConfig m_config;
        std::shared_ptr<Aion::Object3D> m_target;
    };
} // namespace BlockJumper
