#pragma once

// Standard.
#include <cstddef>

/** Provides values to be used in `alignas` to align members of a C++ struct to match GLSL alignment rules. */
class ShaderAlignmentConstants {
public:
    /** Scalars have to be aligned by N (= 4 bytes given 32 bit floats). */
    static constexpr unsigned int iScalar = 4;

    /** A vec2 must be aligned by 2N (= 8 bytes). */
    static constexpr unsigned int iVec2 = 8;

    /** A vec3 or vec4 must be aligned by 4N (= 16 bytes). */
    static constexpr unsigned int iVec4 = 16;

    /** A mat4 matrix must have the same alignment as a vec4. */
    static constexpr unsigned int iMat4 = iVec4;
};
