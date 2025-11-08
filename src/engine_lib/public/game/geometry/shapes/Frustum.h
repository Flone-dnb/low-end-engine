#pragma once

// Custom.
#include "math/GLMath.hpp"
#include "game/geometry/shapes/Plane.h"
#include "game/geometry/shapes/AABB.h"
#include "game/geometry/shapes/Sphere.h"
#include "game/geometry/shapes/Cone.h"
#include "misc/Globals.h"

/** Frustum represented by 6 planes. */
struct Frustum {
    /**
     * Tests if the specified axis-aligned bounding box is inside of the frustum or intersects it.
     *
     * @remark Does frustum/AABB intersection in world space.
     *
     * @param aabbInModelSpace Axis-aligned bounding box in model space.
     * @param worldMatrix      Matrix that transforms the specified AABB from model space to world space.
     *
     * @return `true` if the AABB is inside of the frustum or intersects it, `false` if the AABB is
     * outside of the frustum.
     */
    inline bool isAabbInFrustum(const AABB& aabbInModelSpace, const glm::mat4x4& worldMatrix) const;

    /**
     * Tests if the specified sphere is inside of the frustum or intersects it.
     *
     * @remark Expects that both frustum and sphere are in the same coordinate space.
     *
     * @param sphere Sphere to test.
     *
     * @return `true` if the sphere is inside of the frustum or intersects it, `false` if the sphere is
     * outside of the frustum.
     */
    inline bool isSphereInFrustum(const Sphere& sphere) const;

    /**
     * Tests if the specified cone is inside of the frustum or intersects it.
     *
     * @remark Expects that both frustum and cone are in the same coordinate space.
     *
     * @param cone Cone to test.
     *
     * @return `true` if the cone is inside of the frustum or intersects it, `false` if the cone is
     * outside of the frustum.
     */
    inline bool isConeInFrustum(const Cone& cone) const;

    /** Top face of the frustum that points inside of the frustum volume. */
    Plane topFace;

    /** Bottom face of the frustum that points inside of the frustum volume. */
    Plane bottomFace;

    /** Right face of the frustum that points inside of the frustum volume. */
    Plane rightFace;

    /** Left face of the frustum that points inside of the frustum volume. */
    Plane leftFace;

    /** Near face of the frustum that points inside of the frustum volume. */
    Plane nearFace;

    /** Far face of the frustum that points inside of the frustum volume. */
    Plane farFace;
};

bool Frustum::isAabbInFrustum(const AABB& aabbInModelSpace, const glm::mat4x4& worldMatrix) const {
    // Before comparing frustum faces against AABB we need to take care of something:
    // we can't just transform AABB to world space (using world matrix) as this would result
    // in OBB (oriented bounding box) because of rotation in world matrix while we need an AABB.

    // Prepare an AABB that stores OBB (in world space) converted to AABB (in world space).
    AABB aabb;

    // Calculate AABB center.
    aabb.center = worldMatrix * glm::vec4(aabbInModelSpace.center, 1.0F);

    // Calculate OBB directions in world space
    // (directions are considered to point from OBB's center).
    const glm::vec3 obbScaledForward =
        worldMatrix * glm::vec4(Globals::WorldDirection::forward, 0.0F) * aabbInModelSpace.extents.z;
    const glm::vec3 obbScaledRight =
        worldMatrix * glm::vec4(Globals::WorldDirection::right, 0.0F) * aabbInModelSpace.extents.x;
    const glm::vec3 obbScaledUp =
        worldMatrix * glm::vec4(Globals::WorldDirection::up, 0.0F) * aabbInModelSpace.extents.y;

    // If the specified world matrix contained a rotation OBB's directions are no longer aligned
    // with world axes. We need to adjust these OBB directions to be world axis aligned and save them
    // as resulting AABB extents.

    // We can convert scaled OBB directions to AABB extents (directions) by projecting each OBB direction
    // onto world axis.

    // Calculate X extent.
    aabb.extents.x =
        std::abs(glm::dot(obbScaledForward, glm::vec3(1.0F, 0.0F, 0.0F))) + // project OBB X on world X
        std::abs(glm::dot(obbScaledRight, glm::vec3(1.0F, 0.0F, 0.0F))) +   // project OBB Y on world X
        std::abs(glm::dot(obbScaledUp, glm::vec3(1.0F, 0.0F, 0.0F)));       // project OBB Z on world X

    // Calculate Y extent.
    aabb.extents.y =
        std::abs(glm::dot(obbScaledForward, glm::vec3(0.0F, 1.0F, 0.0F))) + // project OBB X on world Y
        std::abs(glm::dot(obbScaledRight, glm::vec3(0.0F, 1.0F, 0.0F))) +   // project OBB Y on world Y
        std::abs(glm::dot(obbScaledUp, glm::vec3(0.0F, 1.0F, 0.0F)));       // project OBB Z on world Y

    // Calculate Z extent.
    aabb.extents.z =
        std::abs(glm::dot(obbScaledForward, glm::vec3(0.0F, 0.0F, 1.0F))) + // project OBB X on world Z
        std::abs(glm::dot(obbScaledRight, glm::vec3(0.0F, 0.0F, 1.0F))) +   // project OBB Y on world Z
        std::abs(glm::dot(obbScaledUp, glm::vec3(0.0F, 0.0F, 1.0F)));       // project OBB Z on world Z

    // Test each AABB face against the frustum.
    return !aabb.isBehindPlane(leftFace) && !aabb.isBehindPlane(rightFace) && !aabb.isBehindPlane(topFace) &&
           !aabb.isBehindPlane(bottomFace) && !aabb.isBehindPlane(nearFace) && !aabb.isBehindPlane(farFace);
}

bool Frustum::isSphereInFrustum(const Sphere& sphere) const {
    return !sphere.isBehindPlane(leftFace) && !sphere.isBehindPlane(rightFace) &&
           !sphere.isBehindPlane(topFace) && !sphere.isBehindPlane(bottomFace) &&
           !sphere.isBehindPlane(nearFace) && !sphere.isBehindPlane(farFace);
}

bool Frustum::isConeInFrustum(const Cone& cone) const {
    return !cone.isBehindPlane(leftFace) && !cone.isBehindPlane(rightFace) && !cone.isBehindPlane(topFace) &&
           !cone.isBehindPlane(bottomFace) && !cone.isBehindPlane(nearFace) && !cone.isBehindPlane(farFace);
}
