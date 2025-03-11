#include "game/geometry/MeshGeometry.h"

// External.
#include "glad/glad.h"

void MeshVertex::setVertexAttributes() {
    static_assert(
        sizeof(MeshGeometry::MeshIndexType) == sizeof(unsigned short),
        "change index type in renderer's draw command");

    // Prepare offsets of fields.
    const auto iPositionOffset = offsetof(MeshVertex, position);
    const auto iNormalOffset = offsetof(MeshVertex, normal);
    const auto iUvOffset = offsetof(MeshVertex, uv);

    // Specify position.
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(
        0,                                         // attribute index (layout location)
        3,                                         // number of components
        GL_FLOAT,                                  // type of component
        GL_FALSE,                                  // whether data should be normalized or not
        sizeof(MeshVertex),                        // stride (size in bytes between elements)
        reinterpret_cast<void*>(iPositionOffset)); // NOLINT: beginning offset

    // Specify normal.
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(
        1,                                       // attribute index (layout location)
        3,                                       // number of components
        GL_FLOAT,                                // type of component
        GL_FALSE,                                // whether data should be normalized or not
        sizeof(MeshVertex),                      // stride (size in bytes between elements)
        reinterpret_cast<void*>(iNormalOffset)); // NOLINT: beginning offset

    // Specify UV.
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(
        2,                                   // attribute index (layout location)
        2,                                   // number of components
        GL_FLOAT,                            // type of component
        GL_FALSE,                            // whether data should be normalized or not
        sizeof(MeshVertex),                  // stride (size in bytes between elements)
        reinterpret_cast<void*>(iUvOffset)); // NOLINT: beginning offset
}

bool MeshVertex::operator==(const MeshVertex& other) const {
    constexpr auto delta = 0.00001F;

    static_assert(sizeof(MeshVertex) == 32, "add new fields here"); // NOLINT: current size

    return glm::all(glm::epsilonEqual(position, other.position, delta)) &&
           glm::all(glm::epsilonEqual(normal, other.normal, delta)) &&
           glm::all(glm::epsilonEqual(uv, other.uv, delta));
}
