#include "game/geometry/PrimitiveMeshGenerator.h"

MeshGeometry PrimitiveMeshGenerator::createCube(float size) {
    MeshGeometry geometry;
    MeshVertex vertex;
    const auto halfSize = size * 0.5F;

    static_assert(sizeof(MeshVertex) == 32, "properly generate new mesh data"); // NOLINT: current size

    // NOLINTBEGIN(readability-magic-numbers)

    // Vertices:
    geometry.getVertices().resize(24);

    // +X face.
    vertex.normal = glm::vec3(1.0F, 0.0F, 0.0F);
    vertex.position = glm::vec3(halfSize, -halfSize, -halfSize);
    vertex.uv = glm::vec2(1.0F, 1.0F);
    geometry.getVertices().at(0) = vertex;
    vertex.position = glm::vec3(halfSize, halfSize, -halfSize);
    vertex.uv = glm::vec2(0.0F, 1.0F);
    geometry.getVertices().at(1) = vertex;
    vertex.position = glm::vec3(halfSize, -halfSize, halfSize);
    vertex.uv = glm::vec2(1.0F, 0.0F);
    geometry.getVertices().at(2) = vertex;
    vertex.position = glm::vec3(halfSize, halfSize, halfSize);
    vertex.uv = glm::vec2(0.0F, 0.0F);
    geometry.getVertices().at(3) = vertex;

    // -X face.
    vertex.normal = glm::vec3(-1.0F, 0.0F, 0.0F);
    vertex.position = glm::vec3(-halfSize, halfSize, -halfSize);
    vertex.uv = glm::vec2(1.0F, 1.0F);
    geometry.getVertices().at(4) = vertex;
    vertex.position = glm::vec3(-halfSize, -halfSize, -halfSize);
    vertex.uv = glm::vec2(0.0F, 1.0F);
    geometry.getVertices().at(5) = vertex;
    vertex.position = glm::vec3(-halfSize, halfSize, halfSize);
    vertex.uv = glm::vec2(1.0F, 0.0F);
    geometry.getVertices().at(6) = vertex;
    vertex.position = glm::vec3(-halfSize, -halfSize, halfSize);
    vertex.uv = glm::vec2(0.0F, 0.0F);
    geometry.getVertices().at(7) = vertex;

    // +Y face.
    vertex.normal = glm::vec3(0.0F, 1.0F, 0.0F);
    vertex.position = glm::vec3(halfSize, halfSize, -halfSize);
    vertex.uv = glm::vec2(1.0F, 1.0F);
    geometry.getVertices().at(8) = vertex;
    vertex.position = glm::vec3(-halfSize, halfSize, -halfSize);
    vertex.uv = glm::vec2(0.0F, 1.0F);
    geometry.getVertices().at(9) = vertex;
    vertex.position = glm::vec3(halfSize, halfSize, halfSize);
    vertex.uv = glm::vec2(1.0F, 0.0F);
    geometry.getVertices().at(10) = vertex;
    vertex.position = glm::vec3(-halfSize, halfSize, halfSize);
    vertex.uv = glm::vec2(0.0F, 0.0F);
    geometry.getVertices().at(11) = vertex;

    // -Y face.
    vertex.normal = glm::vec3(0.0F, -1.0F, 0.0F);
    vertex.position = glm::vec3(-halfSize, -halfSize, -halfSize);
    vertex.uv = glm::vec2(1.0F, 1.0F);
    geometry.getVertices().at(12) = vertex;
    vertex.position = glm::vec3(halfSize, -halfSize, -halfSize);
    vertex.uv = glm::vec2(0.0F, 1.0F);
    geometry.getVertices().at(13) = vertex;
    vertex.position = glm::vec3(-halfSize, -halfSize, halfSize);
    vertex.uv = glm::vec2(1.0F, 0.0F);
    geometry.getVertices().at(14) = vertex;
    vertex.position = glm::vec3(halfSize, -halfSize, halfSize);
    vertex.uv = glm::vec2(0.0F, 0.0F);
    geometry.getVertices().at(15) = vertex;

    // +Z face.
    vertex.normal = glm::vec3(0.0F, 0.0F, 1.0F);
    vertex.position = glm::vec3(-halfSize, -halfSize, halfSize);
    vertex.uv = glm::vec2(1.0F, 1.0F);
    geometry.getVertices().at(16) = vertex;
    vertex.position = glm::vec3(halfSize, -halfSize, halfSize);
    vertex.uv = glm::vec2(0.0F, 1.0F);
    geometry.getVertices().at(17) = vertex;
    vertex.position = glm::vec3(-halfSize, halfSize, halfSize);
    vertex.uv = glm::vec2(1.0F, 0.0F);
    geometry.getVertices().at(18) = vertex;
    vertex.position = glm::vec3(halfSize, halfSize, halfSize);
    vertex.uv = glm::vec2(0.0F, 0.0F);
    geometry.getVertices().at(19) = vertex;

    // -Z face.
    vertex.normal = glm::vec3(0.0F, 0.0F, -1.0F);
    vertex.position = glm::vec3(-halfSize, halfSize, -halfSize);
    vertex.uv = glm::vec2(1.0F, 1.0F);
    geometry.getVertices().at(20) = vertex;
    vertex.position = glm::vec3(halfSize, halfSize, -halfSize);
    vertex.uv = glm::vec2(0.0F, 1.0F);
    geometry.getVertices().at(21) = vertex;
    vertex.position = glm::vec3(-halfSize, -halfSize, -halfSize);
    vertex.uv = glm::vec2(1.0F, 0.0F);
    geometry.getVertices().at(22) = vertex;
    vertex.position = glm::vec3(halfSize, -halfSize, -halfSize);
    vertex.uv = glm::vec2(0.0F, 0.0F);
    geometry.getVertices().at(23) = vertex;

    // Indices:
    geometry.getIndices() = {
        0,  1,  2,  3,  2,  1,  // +X face.
        4,  5,  6,  7,  6,  5,  // -X face.
        8,  9,  10, 11, 10, 9,  // +Y face.
        12, 13, 14, 15, 14, 13, // -Y face.
        16, 17, 18, 19, 18, 17, // +Z face.
        20, 21, 22, 23, 22, 21  // -Z face.
    };

    // NOLINTEND(readability-magic-numbers)

    return geometry;
}
