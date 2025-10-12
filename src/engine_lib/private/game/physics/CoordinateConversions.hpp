#pragma once

// Custom.
#include "math/GLMath.hpp"

// External.
#include "Jolt/Jolt.h"
#include "Jolt/Math/Vec3.h"

/**
 * Convert type.
 *
 * @param vec Value.
 *
 * @return Result.
 */
static inline JPH::Vec3 convertPosDirToJolt(const glm::vec3& vec) { return JPH::Vec3(vec.x, vec.y, vec.z); }

/**
 * Convert type.
 *
 * @param vec Value.
 *
 * @return Result.
 */
static inline glm::vec3 convertPosDirFromJolt(const JPH::Vec3& vec) {
    return glm::vec3(vec.GetX(), vec.GetY(), vec.GetZ());
}

/**
 * Convert type.
 *
 * @param rotation Rotation in degrees.
 *
 * @return Result.
 */
static inline JPH::Quat convertRotationToJolt(const glm::vec3& rotation) {
    const auto quat = glm::quat(glm::radians(rotation));
    return JPH::Quat(quat.x, quat.y, quat.z, quat.w);
}

/**
 * Convert type.
 *
 * @param rotation Rotation.
 *
 * @return Result.
 */
static inline glm::vec3 convertRotationFromJolt(const JPH::Quat& rotation) {
    const auto quat = glm::quat(rotation.GetW(), rotation.GetX(), rotation.GetY(), rotation.GetZ());
    return glm::degrees(glm::eulerAngles(quat));
}
