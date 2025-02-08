/**
 * GLM headers that the engine uses with needed predefined macros.
 * Prefer to include this header instead of the original GLM headers.
 */

#pragma once

// right-handed coordinate system
// clip-space Z is in range [-1; 1]
#define GLM_FORCE_INLINE
#define GLM_FORCE_XYZW_ONLY
#define GLM_ENABLE_EXPERIMENTAL
// #define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES  // Disabling these because they have proved to cause
// #define GLM_FORCE_INTRINSICS                // some absurd crashed only in release builds.

#include "glm/glm.hpp"
#include "glm/trigonometric.hpp"
#include "glm/gtx/vector_angle.hpp"
#include "glm/gtx/matrix_decompose.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "glm/gtx/compatibility.hpp"
