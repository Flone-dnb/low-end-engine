#pragma once

// Standard.
#include <memory>

// Custom.
#include "game/geometry/MeshGeometry.h"
#include "render/VertexArrayObject.h"
#include "render/Framebuffer.h"

/** A single place for "talking" with OpenGL context about GPU resources. */
class GpuResourceManager {
public:
    GpuResourceManager() = delete;

    /**
     * Creates a VAO from the specified geometry.
     *
     * @param geometry Geometry to load.
     *
     * @return Created VAO.
     */
    static std::unique_ptr<VertexArrayObject> createVertexArrayObject(const MeshGeometry& geometry);

    /**
     * Creates a new framebuffer with textures.
     *
     * @param iWidth         Width of the textures.
     * @param iHeight        Height of the textures.
     * @param iColorGlFormat GL format of the color texture in the framebuffer.
     * @param iDepthGlFormat GL format of the depth/stencil buffer in the framebuffer.
     *
     * @return Created framebuffer.
     */
    static std::unique_ptr<Framebuffer>
    createFramebuffer(unsigned int iWidth, unsigned int iHeight, int iColorGlFormat, int iDepthGlFormat);
};
