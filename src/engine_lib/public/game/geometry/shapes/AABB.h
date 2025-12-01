#pragma once

// Custom.
#include "math/GLMath.hpp"
#include "game/geometry/shapes/Plane.h"
#include "misc/Globals.h"

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

    /**
     * Converts AABB to world space.
     *
     * @param worldMatrix World matrix.
     *
     * @return AABB in world space.
     */
    inline AABB convertToWorldSpace(const glm::mat4& worldMatrix) const;

    /** Center of the AABB in model space. */
    glm::vec3 center = glm::vec3(0.0f, 0.0f, 0.0f);

    /** Half extension (size) of the AABB in model space. */
    glm::vec3 extents = glm::vec3(0.0f, 0.0f, 0.0f);
};

bool AABB::isBehindPlane(const Plane& plane) const {
    // Source: https://github.com/gdbooks/3DCollisions/blob/master/Chapter2/static_aabb_plane.md

    const float projectionRadius = extents.x * std::abs(plane.normal.x) +
                                   extents.y * std::abs(plane.normal.y) +
                                   extents.z * std::abs(plane.normal.z);

    const auto distanceToPlane = glm::dot(plane.normal, center) - plane.distanceFromOrigin;

    return !(-projectionRadius <= distanceToPlane);
}

AABB AABB::convertToWorldSpace(const glm::mat4& worldMatrix) const {
    // We can't just transform AABB to world space (using world matrix) as this would result
    // in OBB (oriented bounding box) because of rotation in world matrix while we need an AABB.

    // Prepare an AABB that stores OBB (in world space) converted to AABB (in world space).
    AABB aabb;

    // Calculate AABB center.
    aabb.center = worldMatrix * glm::vec4(center, 1.0f);

    // Calculate OBB directions in world space
    // (directions are considered to point from OBB's center).
    const glm::vec3 obbScaledForward =
        worldMatrix * glm::vec4(Globals::WorldDirection::forward, 0.0f) * extents.z;
    const glm::vec3 obbScaledRight =
        worldMatrix * glm::vec4(Globals::WorldDirection::right, 0.0f) * extents.x;
    const glm::vec3 obbScaledUp = worldMatrix * glm::vec4(Globals::WorldDirection::up, 0.0f) * extents.y;

    // If the specified world matrix contained a rotation OBB's directions are no longer aligned
    // with world axes. We need to adjust these OBB directions to be world axis aligned and save them
    // as resulting AABB extents.

    // We can convert scaled OBB directions to AABB extents (directions) by projecting each OBB direction
    // onto world axis.

    // Calculate X extent.
    aabb.extents.x =
        std::abs(glm::dot(obbScaledForward, glm::vec3(1.0f, 0.0f, 0.0f))) + // project OBB X on world X
        std::abs(glm::dot(obbScaledRight, glm::vec3(1.0f, 0.0f, 0.0f))) +   // project OBB Y on world X
        std::abs(glm::dot(obbScaledUp, glm::vec3(1.0f, 0.0f, 0.0f)));       // project OBB Z on world X

    // Calculate Y extent.
    aabb.extents.y =
        std::abs(glm::dot(obbScaledForward, glm::vec3(0.0f, 1.0f, 0.0f))) + // project OBB X on world Y
        std::abs(glm::dot(obbScaledRight, glm::vec3(0.0f, 1.0f, 0.0f))) +   // project OBB Y on world Y
        std::abs(glm::dot(obbScaledUp, glm::vec3(0.0f, 1.0f, 0.0f)));       // project OBB Z on world Y

    // Calculate Z extent.
    aabb.extents.z =
        std::abs(glm::dot(obbScaledForward, glm::vec3(0.0f, 0.0f, 1.0f))) + // project OBB X on world Z
        std::abs(glm::dot(obbScaledRight, glm::vec3(0.0f, 0.0f, 1.0f))) +   // project OBB Y on world Z
        std::abs(glm::dot(obbScaledUp, glm::vec3(0.0f, 0.0f, 1.0f)));       // project OBB Z on world Z

    return aabb;
}
