#pragma once

// Standard.
#include <memory>
#include <mutex>
#include <optional>

// Custom.
#include "game/geometry/MeshNodeGeometry.h"
#include "game/geometry/SkeletalMeshNodeGeometry.h"
#include "render/wrapper/VertexArrayObject.h"
#include "render/wrapper/Framebuffer.h"
#include "render/wrapper/Buffer.h"
#include "render/wrapper/Texture.h"
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
     * @param bIsVertexDataDynamic Specify `true` if vertex positions will change, otherwise
     * specify `false`.
     * @param vVertexPositions     Specify non-empty to copy the data to the vertex buffer.
     * @param vIndices             Specify empty array to avoid creating index buffer.
     *
     * @return VAO.
     */
    static std::unique_ptr<VertexArrayObject> createVertexArrayObject(
        bool bIsVertexDataDynamic,
        const std::vector<glm::vec3>& vVertexPositions = {},
        const std::vector<unsigned short>& vIndices = {});

    /**
     * Creates a new quad.
     *
     * @param vertexData Optionally specify initial positions of quad vertices. If empty creates
     * a full screen quad with data in normalized device coordinates.
     * @param indexData  Optionally specify indices (otherwise default will be used).
     *
     * @return Quad.
     */
    static std::unique_ptr<ScreenQuadGeometry> createScreenQuad(
        std::optional<std::array<ScreenQuadGeometry::VertexLayout, ScreenQuadGeometry::iVertexCount>>
            vertexData = {},
        std::optional<std::array<unsigned short, ScreenQuadGeometry::iIndexCount>> indexData = {});

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
     * @param iColorGlFormat Specify 0 to create a framebuffer without color. GL format of the color texture
     * in the framebuffer.
     * @param iDepthGlFormat Specify 0 to create a framebuffer without depth. Otherwise GL format of the
     * depth/stencil buffer in the framebuffer.
     *
     * @return Created framebuffer.
     */
    static std::unique_ptr<Framebuffer>
    createFramebuffer(unsigned int iWidth, unsigned int iHeight, int iColorGlFormat, int iDepthGlFormat);

    /**
     * Creates a new framebuffer for shadow pass.
     *
     * @param shadowMapArray Texture array of shadow maps.
     * @param iTextureIndex  Index of the texture (in the array) to attach to framebuffer.
     *
     * @return Created framebuffer.
     */
    static std::unique_ptr<Framebuffer>
    createShadowMapFramebuffer(Texture& shadowMapArray, unsigned int iTextureIndex);

    /**
     * Creates texture array object.
     *
     * @param iWidth     Width of the textures.
     * @param iHeight    Height of the textures.
     * @param iGlFormat  GL format of the textures.
     * @param iArraySize Size of the array.
     * @param bIsShadowMaps `true` to enable hardware anti-aliasing of shadow maps.
     *
     * @return Created texture array.
     */
    static std::unique_ptr<Texture> createTextureArray(
        unsigned int iWidth,
        unsigned int iHeight,
        int iGlFormat,
        unsigned int iArraySize,
        bool bIsShadowMaps);

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
