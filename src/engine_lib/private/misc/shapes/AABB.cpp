#include "misc/shapes/AABB.h"

bool AABB::isBehindPlane(const Plane& plane) const {
    // Source: https://github.com/gdbooks/3DCollisions/blob/master/Chapter2/static_aabb_plane.md

    const float projectionRadius = extents.x * std::abs(plane.normal.x) +
                                   extents.y * std::abs(plane.normal.y) +
                                   extents.z * std::abs(plane.normal.z);

    const auto distanceToPlane = glm::dot(plane.normal, center) - plane.distanceFromOrigin;

    return !(-projectionRadius <= distanceToPlane);
}
