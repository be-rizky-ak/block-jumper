#include "FollowCamera.h"

#include <cmath>
#include "Aion/Math/Math.h"

namespace BlockJumper
{
    FollowCamera::FollowCamera()
        : m_config(CameraConfig{})
    {
    }

    FollowCamera::FollowCamera(const CameraConfig& config)
        : m_config(config)
    {
    }

    void FollowCamera::SetTarget(std::shared_ptr<Aion::Object3D> target)
    {
        m_target = std::move(target);
        SnapToTarget();
    }

    void FollowCamera::SnapToTarget()
    {
        Aion::Object3D* owner = GetOwner();
        if (!owner || !m_target)
        {
            return;
        }

        Aion::Vector3 targetPos = m_target->Transform.Position;

        owner->Transform.Position = targetPos + m_config.Offset;

        // Fixed isometric orientation: diagonal yaw (45°) and elevated pitch (~-35.264°)
        owner->Transform.Rotation = Aion::Quaternion::FromEuler(
            Aion::Math::Radians(m_config.PitchDegrees),
            Aion::Math::Radians(m_config.YawDegrees),
            0.0f);
    }

    void FollowCamera::OnUpdate(float deltaTime)
    {
        if (deltaTime <= 0.0f)
        {
            return;
        }
        if (deltaTime > 0.05f)
        {
            deltaTime = 0.05f;
        }

        Aion::Object3D* owner = GetOwner();
        if (!owner || !m_target)
        {
            return;
        }

        // Maintain fixed isometric viewing angle at all times
        owner->Transform.Rotation = Aion::Quaternion::FromEuler(
            Aion::Math::Radians(m_config.PitchDegrees),
            Aion::Math::Radians(m_config.YawDegrees),
            0.0f);

        Aion::Vector3 targetPos = m_target->Transform.Position;
        Aion::Vector3 desiredPos = targetPos + m_config.Offset;

        // Frame-rate-independent exponential smoothing
        float alphaH = 1.0f - std::exp(-m_config.FollowSmoothRateHorizontal * deltaTime);
        float alphaV = 1.0f - std::exp(-m_config.FollowSmoothRateVertical * deltaTime);

        Aion::Vector3 camPos = owner->Transform.Position;
        camPos.x += (desiredPos.x - camPos.x) * alphaH;
        camPos.z += (desiredPos.z - camPos.z) * alphaH;
        camPos.y += (desiredPos.y - camPos.y) * alphaV;

        owner->Transform.Position = camPos;
    }
} // namespace BlockJumper
