#pragma once

// Custom.
#include "math/GLMath.hpp"
#include "game/geometry/shapes/Plane.h"

/** Axis-aligned bounding box. */
struct AABB {
    /**
     * Tells if the AABB is fully behind (inside the negative halfspace of) a plane.
     *
     * @param plane Plane to test.
     *
     * @return `true` if the AABB is fully behind the plane, `false` if intersects or in front of it.
     */
    inline bool isBehindPlane(const Plane& plane) const;

    /** Center of the AABB in model space. */
    glm::vec3 center = glm::vec3(0.0F, 0.0F, 0.0F);

    /** Half extension (size) of the AABB in model space. */
    glm::vec3 extents = glm::vec3(0.0F, 0.0F, 0.0F);
};

bool AABB::isBehindPlane(const Plane& plane) const {
    // Source: https://github.com/gdbooks/3DCollisions/blob/master/Chapter2/static_aabb_plane.md

    const float projectionRadius = extents.x * std::abs(plane.normal.x) +
                                   extents.y * std::abs(plane.normal.y) +
                                   extents.z * std::abs(plane.normal.z);

    const auto distanceToPlane = glm::dot(plane.normal, center) - plane.distanceFromOrigin;

    return !(-projectionRadius <= distanceToPlane);
}
