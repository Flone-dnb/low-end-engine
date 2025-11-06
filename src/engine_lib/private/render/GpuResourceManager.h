#pragma once

// Standard.
#include <memory>
#include <mutex>

// Custom.
#include "game/geometry/MeshNodeGeometry.h"
#include "game/geometry/SkeletalMeshNodeGeometry.h"
#include "render/wrapper/VertexArrayObject.h"
#include "render/wrapper/Framebuffer.h"
#include "render/wrapper/Buffer.h"
#include "game/geometry/ScreenQuadGeometry.h"

class Texture;

/** Manages creation of GPU resources. */
class GpuResourceManager {
public:
    GpuResourceManager() = default;

    GpuResourceManager(const GpuResourceManager&) = delete;
    GpuResourceManager& operator=(const GpuResourceManager&) = delete;
    GpuResourceManager(GpuResourceManager&&) noexcept = delete;
    GpuResourceManager& operator=(GpuResourceManager&&) noexcept = delete;
    /**
     * Creates a new vertex array object for N positions (vec3) and optionally N indices.
     *
     * @param iPositionCount       Total number of vec3 items in the vertex buffer.
     * @param bIsVertexDataDynamic Specify `true` if vertex positions will change, otherwise
     * specify `false`.
     * @param vVertexPositions     Specify non-empty to copy the data to the vertex buffer.
     * @param vIndices             Specify empty array to avoid creating index buffer.
     *
     * @return VAO.
     */
    static std::unique_ptr<VertexArrayObject> createVertexArrayObject(
        unsigned int iPositionCount,
        bool bIsVertexDataDynamic,
        const std::vector<glm::vec3>& vVertexPositions = {},
        const std::vector<unsigned short>& vIndices = {});

    /**
     * Creates a new quad.
     *
     * @param bIsVertexDataDynamic Specify `true` if vertex positions/uvs will be changed often, otherwise
     * specify `false` if vertices of the quad will always be constant.
     * @param vertexData           Optionally specify initial positions of quad vertices. If empty creates
     * a full screen quad with data in normalized device coordinates.
     *
     * @return Quad.
     */
    static std::unique_ptr<ScreenQuadGeometry> createQuad(
        bool bIsVertexDataDynamic,
        std::optional<std::array<ScreenQuadGeometry::VertexLayout, ScreenQuadGeometry::iVertexCount>>
            vertexData = {});

    /**
     * Creates a VAO from the specified geometry.
     *
     * @param geometry Geometry to load.
     *
     * @return Created VAO.
     */
    static std::unique_ptr<VertexArrayObject> createVertexArrayObject(const MeshNodeGeometry& geometry);

    /**
     * Creates a VAO from the specified geometry.
     *
     * @param geometry Geometry to load.
     *
     * @return Created VAO.
     */
    static std::unique_ptr<VertexArrayObject>
    createVertexArrayObject(const SkeletalMeshNodeGeometry& geometry);

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
     * @return Created buffer.
     */
    static std::unique_ptr<Buffer> createUniformBuffer(unsigned int iSizeInBytes, bool bIsDynamic);

    /**
     * Creates a new shader storage buffer object (SSBO).
     *
     * @param iSizeInBytes Size of the buffer in bytes.
     *
     * @return Created buffer.
     */
    static std::unique_ptr<Buffer> createStorageBuffer(unsigned int iSizeInBytes);

    /**
     * Creates a new storage image (image to write to from shaders).
     *
     * @param iWidth  Width of the texture in pixels.
     * @param iHeight Height of the texture in pixels.
     * @param iFormat Format of the texture, for example `GL_R32UI`.
     *
     * @return Created texture.
     */
    static std::unique_ptr<Texture>
    createStorageTexture(unsigned int iWidth, unsigned int iHeight, int iFormat);

    /**
     * Mutex to guard OpenGL context modification.
     *
     * @remark Made public so that you can use it outside of the manager in some cases.
     */
    static std::recursive_mutex mtx;
};
