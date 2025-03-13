#pragma once

// Standard.
#include <cstddef>

/** Provides values to be used in `alignas` to align members of a C++ struct to match GLSL alignment rules. */
class ShaderAlignmentConstants {
public:
    /** Scalars have to be aligned by N (= 4 bytes given 32 bit floats). */
    static constexpr size_t iScalar = 4; // NOLINT

    /** A vec2 must be aligned by 2N (= 8 bytes). */
    static constexpr size_t iVec2 = 8; // NOLINT

    /** A vec3 or vec4 must be aligned by 4N (= 16 bytes). */
    static constexpr size_t iVec4 = 16; // NOLINT

    /** A mat4 matrix must have the same alignment as a vec4. */
    static constexpr size_t iMat4 = iVec4;
};
