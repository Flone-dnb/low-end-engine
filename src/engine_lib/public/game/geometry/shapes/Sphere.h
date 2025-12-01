#pragma once

// Custom.
#include "math/GLMath.hpp"
#include "game/geometry/shapes/Plane.h"

/** Sphere shape. */
struct Sphere {
    /** Creates uninitialized sphere. */
    Sphere() = default;

    /**
     * Initializes the sphere.
     *
     * @param center Location of the sphere's center point.
     * @param radius Sphere's radius.
     */
    Sphere(const glm::vec3& center, float radius);

    /**
     * Tells if the sphere is fully behind (inside the negative halfspace of) a plane.
     *
     * @param plane Plane to test.
     *
     * @return `true` if the sphere is fully behind the plane, `false` if intersects or in front of it.
     */
    bool isBehindPlane(const Plane& plane) const;

    /** Location of the sphere's center point. */
    glm::vec3 center = glm::vec3(0.0f, 0.0f, 0.0f);

    /** Sphere's radius. */
    float radius = 1.0f;
};
