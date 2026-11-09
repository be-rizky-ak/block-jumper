#pragma once

#include <algorithm>
#include <memory>
#include <string>
#include <vector>

#include "Aion/Math/AABB.h"
#include "Aion/Math/Vector3.h"
#include "Aion/Scene/Object3D.h"
#include "game/GameConfig.h"

namespace BlockJumper
{
    /**
     * @brief Blueprint pattern template used to instantiate course segments.
     */
    struct SegmentPattern
    {
        std::string Name;
        Aion::Vector3 LocalEntryPoint{0.0f};
        Aion::Vector3 LocalExitPoint{0.0f};
        std::vector<PlatformBox> LocalPlatforms;
    };

    /**
     * @brief Represents a self-contained, active platform course segment in world space.
     * Manages its platforms, entry and exit connection anchors, and visual scene objects.
     */
    class LevelSegment
    {
    public:
        LevelSegment(
            int index,
            std::string name,
            const Aion::Vector3& worldEntryPoint,
            const Aion::Vector3& worldExitPoint,
            std::vector<PlatformBox> worldPlatforms,
            std::vector<std::shared_ptr<Aion::Object3D>> sceneObjects)
            : m_index(index),
              m_name(std::move(name)),
              m_worldEntryPoint(worldEntryPoint),
              m_worldExitPoint(worldExitPoint),
              m_worldPlatforms(std::move(worldPlatforms)),
              m_sceneObjects(std::move(sceneObjects))
        {
            ComputeBounds();
        }

        int GetIndex() const { return m_index; }
        const std::string& GetName() const { return m_name; }
        const Aion::Vector3& GetWorldEntryPoint() const { return m_worldEntryPoint; }
        const Aion::Vector3& GetWorldExitPoint() const { return m_worldExitPoint; }
        const std::vector<PlatformBox>& GetPlatforms() const { return m_worldPlatforms; }
        const std::vector<std::shared_ptr<Aion::Object3D>>& GetSceneObjects() const { return m_sceneObjects; }
        const Aion::AABB& GetBounds() const { return m_worldBounds; }

        float GetLength() const
        {
            return (m_worldExitPoint - m_worldEntryPoint).Length();
        }

        /**
         * @brief Checks if a world position falls within this segment's platform bounds.
         */
        bool ContainsPoint(const Aion::Vector3& point, float margin = 0.6f) const
        {
            for (const auto& plat : m_worldPlatforms)
            {
                if (plat.IsSolidObstacle)
                {
                    continue;
                }

                Aion::Vector3 pMin = plat.GetMin();
                Aion::Vector3 pMax = plat.GetMax();

                if (point.x >= pMin.x - margin && point.x <= pMax.x + margin &&
                    point.z >= pMin.z - margin && point.z <= pMax.z + margin &&
                    point.y >= pMin.y - 1.0f && point.y <= plat.GetTopSurfaceY() + 3.0f)
                {
                    return true;
                }
            }
            return false;
        }

        /**
         * @brief Projects a point along the entry-to-exit progression vector.
         * Returns a normalized scalar [0.0, 1.0].
         */
        float ProjectPoint(const Aion::Vector3& point) const
        {
            Aion::Vector3 axis = m_worldExitPoint - m_worldEntryPoint;
            float lenSq = axis.x * axis.x + axis.y * axis.y + axis.z * axis.z;
            if (lenSq < 0.0001f)
            {
                return 0.0f;
            }

            Aion::Vector3 diff = point - m_worldEntryPoint;
            float dot = diff.x * axis.x + diff.y * axis.y + diff.z * axis.z;
            return std::clamp(dot / lenSq, 0.0f, 1.0f);
        }

    private:
        void ComputeBounds()
        {
            if (m_worldPlatforms.empty())
            {
                m_worldBounds = Aion::AABB(m_worldEntryPoint - Aion::Vector3(1.0f), m_worldExitPoint + Aion::Vector3(1.0f));
                return;
            }

            Aion::Vector3 bMin = m_worldPlatforms[0].GetMin();
            Aion::Vector3 bMax = m_worldPlatforms[0].GetMax();

            for (size_t i = 1; i < m_worldPlatforms.size(); ++i)
            {
                Aion::Vector3 pMin = m_worldPlatforms[i].GetMin();
                Aion::Vector3 pMax = m_worldPlatforms[i].GetMax();

                bMin.x = std::min(bMin.x, pMin.x);
                bMin.y = std::min(bMin.y, pMin.y);
                bMin.z = std::min(bMin.z, pMin.z);

                bMax.x = std::max(bMax.x, pMax.x);
                bMax.y = std::max(bMax.y, pMax.y);
                bMax.z = std::max(bMax.z, pMax.z);
            }

            m_worldBounds = Aion::AABB(bMin, bMax);
        }

    private:
        int m_index = 0;
        std::string m_name;
        Aion::Vector3 m_worldEntryPoint{0.0f};
        Aion::Vector3 m_worldExitPoint{0.0f};
        std::vector<PlatformBox> m_worldPlatforms;
        std::vector<std::shared_ptr<Aion::Object3D>> m_sceneObjects;
        Aion::AABB m_worldBounds;
    };
} // namespace BlockJumper
