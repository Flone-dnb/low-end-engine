#pragma once

// Custom.
#include "math/GLMath.hpp"

// External.
#include "Jolt/Jolt.h"
#include "Jolt/Math/Vec3.h"

/**
 * Convert a value to Jolt physics coordinate system. Jolt uses left-handed coordinate system with Y-up but we
 * have Z-up thus conversion functions.
 *
 * @param vec Value.
 *
 * @return Result.
 */
static inline JPH::Vec3 convertToJolt(const glm::vec3& vec) { return JPH::Vec3(vec.x, vec.z, vec.y); }

/**
 * Convert a value from Jolt physics coordinate system.
 *
 * @param vec Value.
 *
 * @return Result.
 */
static inline glm::vec3 convertFromJolt(const JPH::Vec3& vec) {
    return glm::vec3(vec.GetX(), vec.GetZ(), vec.GetY());
}

/**
 * Convert a value to Jolt physics coordinate system. Jolt uses left-handed coordinate system with Y-up but we
 * have Z-up thus conversion functions.
 *
 * @param quat Value.
 *
 * @return Result.
 */
static inline JPH::Quat convertToJolt(const glm::quat& quat) {
    return JPH::Quat(quat.x, quat.z, quat.y, -quat.w);
}
