#pragma once

// Custom.
#include "math/GLMath.hpp"
#include "game/geometry/shapes/Plane.h"

/** Cone shape. */
struct Cone {
    /** Creates uninitialized cone. */
    Cone() = default;

    /**
     * Initializes the cone.
     *
     * @param location     Location of cone's tip.
     * @param height       Height of the cone.
     * @param direction    Direction unit vector from cone's tip.
     * @param bottomRadius Radius of the bottom part of the cone.
     */
    Cone(const glm::vec3& location, float height, const glm::vec3& direction, float bottomRadius) {
        this->location = location;
        this->height = height;
        this->direction = direction;
        this->bottomRadius = bottomRadius;
    }

    /**
     * Tells if the cone is fully behind (inside the negative halfspace of) a plane.
     *
     * @param plane Plane to test.
     *
     * @return `true` if the cone is fully behind the plane, `false` if intersects or in front of it.
     */
    inline bool isBehindPlane(const Plane& plane) const;

    /** Location of cone's tip. */
    glm::vec3 location = glm::vec3(0.0F, 0.0F, 0.0F);

    /** Height of the cone. */
    float height = 1.0F;

    /** Direction unit vector from cone's tip. */
    glm::vec3 direction = glm::vec3(1.0F, 0.0F, 0.0F);

    /** Radius of the bottom part of the cone. */
    float bottomRadius = 1.0F;
};

bool Cone::isBehindPlane(const Plane& plane) const {
    // Source: Real-time collision detection, Christer Ericson (2005).

    // Calculate an intermediate vector which is parallel but opposite to plane's normal and perpendicular
    // to the cone's direction.
    const glm::vec3 intermediate = glm::cross(glm::cross(plane.normal, direction), direction);

    // Calculate the point Q that is on the base (bottom) of the cone that is farthest away from the plane
    // in the direction of plane's normal.
    const glm::vec3 pointOnConeBottom = location + direction * height - intermediate * bottomRadius;

    // The cone is behind the plane if both cone's tip and Q are behind the plane.
    return plane.isPointBehindPlane(location) && plane.isPointBehindPlane(pointOnConeBottom);
}
