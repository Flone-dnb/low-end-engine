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
     * @param size Full size (both sides) of the cube on one axis.
     *
     * @return Cube mesh.
     */
    static MeshNodeGeometry createCube(float size);
};
