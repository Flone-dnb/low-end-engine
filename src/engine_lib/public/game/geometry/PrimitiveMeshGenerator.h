#pragma once

// Custom.
#include "game/geometry/MeshNodeGeometry.h"

/** Provides static functions for creating primitive shapes. */
class PrimitiveMeshGenerator {
public:
    PrimitiveMeshGenerator() = delete;

    /**
     * Creates a cube mesh.
     *
     * @param size Size of the cube.
     *
     * @return Cube mesh.
     */
    static MeshNodeGeometry createCube(float size);
};
