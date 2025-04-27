#pragma once

// Standard.
#include <memory>
#include <mutex>

// Custom.
#include "game/geometry/MeshGeometry.h"
#include "render/wrapper/VertexArrayObject.h"
#include "render/wrapper/Framebuffer.h"
#include "render/wrapper/Buffer.h"
#include "game/geometry/ScreenQuadGeometry.h"

/** Manages creation of GPU resources. */
class GpuResourceManager {
public:
    GpuResourceManager() = default;

    GpuResourceManager(const GpuResourceManager&) = delete;
    GpuResourceManager& operator=(const GpuResourceManager&) = delete;
    GpuResourceManager(GpuResourceManager&&) noexcept = delete;
    GpuResourceManager& operator=(GpuResourceManager&&) noexcept = delete;

    /**
     * Creates a new quad.
     *
     * @param bIsVertexDataDynamic Specify `true` if vertex positions/uvs will be changed often, otherwise
     * specify `false` if vertices of the quad will always be constant.
     * @param vertexPositions      Optionally specify initial positions of quad vertices. If empty creates
     * a full screen quad with positions in normalized device coordinates.
     *
     * @return Quad.
     */
    static std::unique_ptr<ScreenQuadGeometry> createQuad(
        bool bIsVertexDataDynamic,
        std::optional<std::array<glm::vec3, ScreenQuadGeometry::iVertexCount>> vertexPositions = {});

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
     * @param iDepthGlFormat Specify 0 to create a framebuffer without depth. Otherwise GL format of the
     * depth/stencil buffer in the framebuffer.
     *
     * @return Created framebuffer.
     */
    static std::unique_ptr<Framebuffer>
    createFramebuffer(unsigned int iWidth, unsigned int iHeight, int iColorGlFormat, int iDepthGlFormat);

    /**
     * Creates a new uniform buffer.
     *
     * @param iSizeInBytes Size of the buffer in bytes.
     * @param bIsDynamic   Specify `false` if this buffer will not be modified from the CPU side
     * and `true` if you plan on updating the contents of this buffer often.
     *
     * @return Created uniform buffer.
     */
    static std::unique_ptr<Buffer> createUniformBuffer(unsigned int iSizeInBytes, bool bIsDynamic);

    /**
     * Mutex to guard OpenGL context modification.
     *
     * @remark Made public so that you can use it outside of the manager in some cases.
     */
    static std::mutex mtx;
};
