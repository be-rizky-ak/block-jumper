#include "PlayerCollider.h"

#include <algorithm>
#include <cmath>

namespace BlockJumper
{
    PlayerCollider::PlayerCollider(const Aion::Vector3& halfExtents)
        : m_halfExtents(halfExtents)
    {
    }

    Aion::AABB PlayerCollider::GetBounds(const Aion::Vector3& position) const
    {
        return Aion::AABB(position - m_halfExtents, position + m_halfExtents);
    }

    bool PlayerCollider::ResolveCollisions(
        Aion::Vector3& position,
        Aion::Vector3& horizontalVelocity,
        float& verticalVelocity,
        bool& isGrounded,
        const std::vector<PlatformBox>& platforms,
        float previousY,
        float /*deltaTime*/)
    {
        float currentBottomY = position.y - m_halfExtents.y;
        float prevBottomY = previousY - m_halfExtents.y;

        // -------------------------------------------------------------
        // 1. Resolve Solid Obstacle Collisions (Horizontal pushout)
        // -------------------------------------------------------------
        for (const auto& plat : platforms)
        {
            if (!plat.IsSolidObstacle)
            {
                continue;
            }

            Aion::Vector3 pMin = plat.GetMin();
            Aion::Vector3 pMax = plat.GetMax();

            // Check if player Y overlaps the solid obstacle body
            float playerMinY = position.y - m_halfExtents.y;
            float playerMaxY = position.y + m_halfExtents.y;

            if (playerMaxY <= pMin.y || playerMinY >= pMax.y)
            {
                continue; // No vertical overlap with obstacle
            }

            // Check horizontal overlap
            float playerMinX = position.x - m_halfExtents.x;
            float playerMaxX = position.x + m_halfExtents.x;
            float playerMinZ = position.z - m_halfExtents.z;
            float playerMaxZ = position.z + m_halfExtents.z;

            if (playerMaxX > pMin.x && playerMinX < pMax.x &&
                playerMaxZ > pMin.z && playerMinZ < pMax.z)
            {
                // Penetration depth along each axis
                float overlapLeft = playerMaxX - pMin.x;
                float overlapRight = pMax.x - playerMinX;
                float overlapBack = playerMaxZ - pMin.z;
                float overlapFront = pMax.z - playerMinZ;

                float minOverlapX = std::min(overlapLeft, overlapRight);
                float minOverlapZ = std::min(overlapBack, overlapFront);

                if (minOverlapX < minOverlapZ)
                {
                    if (overlapLeft < overlapRight)
                    {
                        position.x = pMin.x - m_halfExtents.x;
                        if (horizontalVelocity.x > 0.0f) horizontalVelocity.x = 0.0f;
                    }
                    else
                    {
                        position.x = pMax.x + m_halfExtents.x;
                        if (horizontalVelocity.x < 0.0f) horizontalVelocity.x = 0.0f;
                    }
                }
                else
                {
                    if (overlapBack < overlapFront)
                    {
                        position.z = pMin.z - m_halfExtents.z;
                        if (horizontalVelocity.z > 0.0f) horizontalVelocity.z = 0.0f;
                    }
                    else
                    {
                        position.z = pMax.z + m_halfExtents.z;
                        if (horizontalVelocity.z < 0.0f) horizontalVelocity.z = 0.0f;
                    }
                }
            }
        }

        // -------------------------------------------------------------
        // 2. Resolve Platform Top Landing and Ground Support
        // -------------------------------------------------------------
        // If ascending, player cannot land on top of platforms
        if (verticalVelocity > 0.0f)
        {
            isGrounded = false;
            return false;
        }

        constexpr float surfaceTolerance = 0.25f;
        const PlatformBox* bestPlatform = nullptr;
        float highestTopY = -9999.0f;

        float footRadius = m_halfExtents.x * 0.5f;

        for (const auto& plat : platforms)
        {
            Aion::Vector3 pMin = plat.GetMin();
            Aion::Vector3 pMax = plat.GetMax();

            // Horizontal footprint overlap with platform
            bool horizOverlap = (position.x + footRadius >= pMin.x &&
                                 position.x - footRadius <= pMax.x &&
                                 position.z + footRadius >= pMin.z &&
                                 position.z - footRadius <= pMax.z);

            if (!horizOverlap)
            {
                continue;
            }

            float topY = plat.GetTopSurfaceY();

            // Check if player bottom was above/near top surface and reached/crossed it
            bool wasAboveOrAt = (prevBottomY >= topY - surfaceTolerance);
            bool reachedOrBelow = (currentBottomY <= topY + surfaceTolerance);

            if (wasAboveOrAt && reachedOrBelow)
            {
                if (topY > highestTopY)
                {
                    highestTopY = topY;
                    bestPlatform = &plat;
                }
            }
        }

        if (bestPlatform)
        {
            position.y = bestPlatform->GetTopSurfaceY() + m_halfExtents.y;
            verticalVelocity = 0.0f;
            isGrounded = true;
            return true;
        }

        // If currently grounded, check whether player is still supported by an overlapping platform
        if (isGrounded)
        {
            bool stillSupported = false;
            for (const auto& plat : platforms)
            {
                Aion::Vector3 pMin = plat.GetMin();
                Aion::Vector3 pMax = plat.GetMax();

                bool horizOverlap = (position.x + footRadius >= pMin.x &&
                                     position.x - footRadius <= pMax.x &&
                                     position.z + footRadius >= pMin.z &&
                                     position.z - footRadius <= pMax.z);

                if (horizOverlap)
                {
                    float topY = plat.GetTopSurfaceY();
                    if (std::abs(currentBottomY - topY) <= 0.25f)
                    {
                        position.y = topY + m_halfExtents.y;
                        stillSupported = true;
                        break;
                    }
                }
            }

            if (!stillSupported)
            {
                // Player stepped off platform edge into empty space -> begin falling
                isGrounded = false;
            }
        }

        return isGrounded;
    }
} // namespace BlockJumper
