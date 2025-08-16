#pragma once

// Standard.
#include <cstdint>

/** Determine the layer in which a mesh is drawn. */
enum class MeshDrawLayer : uint8_t {
    LAYER1 = 0,
    LAYER2,

    COUNT // mark the number of elements in this enum
};
