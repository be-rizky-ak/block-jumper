#pragma once

#include <vector>
#include "Aion/Math/AABB.h"
#include "Aion/Math/Vector3.h"
#include "game/GameConfig.h"

namespace BlockJumper
{
    /**
     * @brief Independent collision representation for the player.
     * Keeps physics collision logic separate from rendering mesh/visual model.
     */
    class PlayerCollider
    {
    public:
        PlayerCollider() = default;
        explicit PlayerCollider(const Aion::Vector3& halfExtents);

        const Aion::Vector3& GetHalfExtents() const { return m_halfExtents; }
        void SetHalfExtents(const Aion::Vector3& halfExtents) { m_halfExtents = halfExtents; }

        Aion::AABB GetBounds(const Aion::Vector3& position) const;

        /**
         * @brief Resolves collision with the level platforms and obstacles.
         * Only allows landing when descending onto the platform surface from above.
         * Prevents landing from underneath.
         * Resolves solid horizontal obstacles.
         *
         * @return true if grounded on a platform.
         */
        bool ResolveCollisions(
            Aion::Vector3& position,
            Aion::Vector3& horizontalVelocity,
            float& verticalVelocity,
            bool& isGrounded,
            const std::vector<PlatformBox>& platforms,
            float previousY,
            float deltaTime,
            const PlatformBox** outGroundedPlatform = nullptr);

    private:
        Aion::Vector3 m_halfExtents{0.5f, 0.5f, 0.5f};
    };
} // namespace BlockJumper
