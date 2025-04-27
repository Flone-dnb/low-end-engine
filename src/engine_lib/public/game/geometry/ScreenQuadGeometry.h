#pragma once

// Standard.
#include <array>
#include <memory>

// Custom.
#include "render/wrapper/VertexArrayObject.h"
#include "math/GLMath.hpp"

/** Quad on a screen (not always full-screen quad). */
class ScreenQuadGeometry {
    /** Only GPU resource manager is allowed to create such objects. */
    friend class GpuResourceManager;

public:
    /** A single vertex of the quad. */
    struct VertexLayout {
        /** May be in normalized device coordinates or other space (depends on the usage). */
        glm::vec2 position;

        /** UVs. */
        glm::vec2 uv;
    };

    ScreenQuadGeometry() = delete;

    /** 2 triangles. */
    static constexpr unsigned int iVertexCount = 6; // NOLINT

    /**
     * Returns vertex data.
     *
     * @return Vertex data.
     */
    std::array<VertexLayout, iVertexCount>& getVertices() { return vVertices; }

    /**
     * Return VAO.
     *
     * @return VAO.
     */
    VertexArrayObject& getVao() const { return *pQuadVao; }

private:
    /**
     * Creates a new object.
     *
     * @param vVertices Initial vertex data.
     * @param pQuadVao  VAO.
     */
    ScreenQuadGeometry(
        const std::array<VertexLayout, iVertexCount>& vVertices, std::unique_ptr<VertexArrayObject> pQuadVao);

    /** Vertex buffer of the full screen quad. By default positions are in normalized device coordinates. */
    std::array<VertexLayout, iVertexCount> vVertices;

    /** Quad VAO. */
    const std::unique_ptr<VertexArrayObject> pQuadVao;
};
