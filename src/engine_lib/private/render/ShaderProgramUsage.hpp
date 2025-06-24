#pragma once

/**
 * Determines the usage of a shader program.
 *
 * @remark Used to group special (!) types shaders.
 */
enum class ShaderProgramUsage : unsigned char {
    MESH_NODE = 0,
    TRANSPARENT_MESH_NODE, // TODO: not a really good solution because if we will have the same shader used
                           // for both opaque and transparent objects we will duplicate that shader
    OTHER,
    // ... new special types go here ...

    COUNT, // marks the size of this enum
};
