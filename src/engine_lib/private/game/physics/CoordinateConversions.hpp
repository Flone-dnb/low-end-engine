#pragma once

// Custom.
#include "math/GLMath.hpp"

// External.
#include "Jolt/Jolt.h"
#include "Jolt/Math/Vec3.h"

/**
 * Convert a position or a direction to Jolt physics coordinate system. Jolt uses left-handed
 * coordinate system with Y-up but we have Z-up thus conversion functions.
 *
 * @param vec Value.
 *
 * @return Result.
 */
static inline JPH::Vec3 convertPosDirToJolt(const glm::vec3& vec) { return JPH::Vec3(vec.x, vec.z, vec.y); }

/**
 * Convert a position or a direction from Jolt physics coordinate system.
 *
 * @param vec Value.
 *
 * @return Result.
 */
static inline glm::vec3 convertPosDirFromJolt(const JPH::Vec3& vec) {
    return glm::vec3(vec.GetX(), vec.GetZ(), vec.GetY());
}

/**
 * Convert rotation to Jolt physics coordinate system.
 *
 * @param rotation Rotation in degrees.
 *
 * @return Result.
 */
static inline JPH::Quat convertRotationToJolt(const glm::vec3& rotation) {
    const auto quat = glm::quat(glm::radians(rotation));
    return JPH::Quat(quat.x, quat.z, quat.y, -quat.w);
}

/**
 * Convert rotation from Jolt physics coordinate system.
 *
 * @param rotation Rotation.
 *
 * @return Result.
 */
static inline glm::vec3 convertRotationFromJolt(const JPH::Quat& rotation) {
    const auto quat = glm::quat(glm::vec3(rotation.GetX(), rotation.GetY(), rotation.GetZ()));
    return glm::degrees(glm::eulerAngles(quat));
}