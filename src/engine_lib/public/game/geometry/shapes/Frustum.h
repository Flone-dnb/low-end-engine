#pragma once

// Custom.
#include "math/GLMath.hpp"
#include "game/geometry/shapes/Plane.h"
#include "game/geometry/shapes/AABB.h"
#include "game/geometry/shapes/Sphere.h"
#include "game/geometry/shapes/Cone.h"

/** Frustum represented by 6 planes. */
struct Frustum {
    /**
     * Tests if the specified axis-aligned bounding box is inside of the frustum or intersects it.
     *
     * @param aabb AABB.
     *
     * @return `true` if the AABB is inside of the frustum or intersects it, `false` if the AABB is
     * outside of the frustum.
     */
    inline bool isAabbInFrustum(const AABB& aabb) const;

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

bool Frustum::isAabbInFrustum(const AABB& aabb) const {
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
