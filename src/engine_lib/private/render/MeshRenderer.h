#pragma once

// Standard.
#include <mutex>
#include <array>
#include <vector>
#include <new>
#include <memory>

// Custom.
#include "render/shader/LightSourceShaderArray.h"
#include "render/ShaderConstantsSetter.hpp"
#include "math/GLMath.hpp"

class ShaderProgram;
class MeshRenderingHandle;
class CameraProperties;
class Renderer;
class LightSourceManager;

/// @cond UNDOCUMENTED

#ifdef __cpp_lib_hardware_interference_size
using std::hardware_constructive_interference_size;
#else
constexpr std::size_t hardware_constructive_interference_size = 64;
#endif

/** Groups data needed to submit a mesh for drawing. */
struct alignas(hardware_constructive_interference_size) MeshRenderData {
    glm::mat4 worldMatrix;
    glm::mat3 normalMatrix;
    glm::vec4 diffuseColor;
    glm::vec2 textureTilingMultiplier;
    unsigned int iDiffuseTextureId = 0; // 0 if not used

    unsigned int iVertexArrayObject = 0;
    int iIndexCount = 0;

    // for skeletal meshes:
    const float* pSkinningMatrices = nullptr;
    int iSkinningMatrixCount = 0;

#if defined(ENGINE_EDITOR)
    unsigned int iNodeId = 0;
#endif
};
static_assert(sizeof(MeshRenderData) == hardware_constructive_interference_size * 3);

/// @endcond

class MeshRenderer;

/** RAII-style type that keeps mesh renderer data locked while exists. */
class MeshRenderDataGuard {
    // Only mesh renderer is allowed create objects of this type.
    friend class MeshRenderer;

public:
    MeshRenderDataGuard() = delete;
    ~MeshRenderDataGuard();

    MeshRenderDataGuard(const MeshRenderDataGuard&) = delete;
    MeshRenderDataGuard& operator=(const MeshRenderDataGuard&) = delete;

    MeshRenderDataGuard(MeshRenderDataGuard&) noexcept = delete;
    MeshRenderDataGuard& operator=(MeshRenderDataGuard&) noexcept = delete;

    /**
     * Returns mesh data to modify.
     *
     * @return Data.
     */
    MeshRenderData& getData() { return *pData; }

private:
    /**
     * Creates a new object.
     *
     * @remark Expects that the render data mutex (in the mesh renderer) is already locked.
     *
     * @param pMeshRenderer Mesh renderer.
     * @param pData Data to modify.
     */
    MeshRenderDataGuard(MeshRenderer* pMeshRenderer, MeshRenderData* pData)
        : pData(pData), pMeshRenderer(pMeshRenderer) {};

    /** Data to modify. */
    MeshRenderData* const pData = nullptr;

    /** Mesh renderer. */
    MeshRenderer* const pMeshRenderer = nullptr;
};

/** Handles mesh rendering. */
class MeshRenderer {
    // Only world is expected to create mesh render for world's specific entities.
    friend class World;

    // Notifies us in destructor.
    friend class MeshRenderingHandle;

    // Unlocks render data mutex on destructor.
    friend class MeshRenderDataGuard;

public:
    /** Groups data for drawing. */
    struct RenderData {
        RenderData() = default;

        /**
         * Defines the maximum number of renderable meshes (both opaque and transparent) at the same time.
         *
         * You are free to change this value but note that we preallocate mesh data
         * array using this value as array size so if increasing this value expect more RAM usage.
         */
        static constexpr unsigned short MAX_MESH_NODE_COUNT_TO_RENDER = 2048;
        static_assert(
            MAX_MESH_NODE_COUNT_TO_RENDER <= std::numeric_limits<unsigned short>::max(),
            "some variables in the renderer won't be able to fit such big numbers");

        /** Marks some meshes that use the same shader. */
        struct ShaderInfo {
            ShaderInfo() = default;

            /**
             * Creates a new shader info and caches locations of all uniform variables that we need.
             *
             * @param pShaderProgram Shader program to use.
             *
             * @return New shader info.
             */
            static ShaderInfo create(ShaderProgram* pShaderProgram);

            /** Used shader program. */
            ShaderProgram* pShaderProgram = nullptr;

            /** First mesh in an array that uses the shader. */
            unsigned short iFirstMeshIndex = 0;

            /** The total number of meshes starting from @ref iFirstMeshIndex that use the same shader. */
            unsigned short iMeshCount = 0;

            /// @cond UNDOCUMENTED
            // uniform locations below:

            int iWorldMatrixUniform = 0;
            int iNormalMatrixUniform = 0;
            int iDiffuseColorUniform = 0;
            int iTextureTilingMultiplierUniform = 0;
            int iIsUsingDiffuseTextureUniform = 0;

            int iSkinningMatricesUniform = -1;

            int iAmbientLightColorUniform = 0;
            int iPointLightCountUniform = 0;
            int iSpotlightCountUniform = 0;
            int iDirectionalLightCountUniform = 0;
            unsigned int iPointLightsUniformBlockBindingIndex = 0;
            unsigned int iSpotlightsUniformBlockBindingIndex = 0;
            unsigned int iDirectionalLightsUniformBlockBindingIndex = 0;

            int iViewProjectionMatrixUniform = 0;

#if defined(ENGINE_EDITOR)
            int iNodeIdUniform = 0;
#endif
            /// @endcond
        };

        /** Stores indices into @ref vMeshRenderData, indices are sorted in the ascending order. */
        std::vector<ShaderInfo> vOpaqueShaders;

        /**
         * Stores indices into @ref vMeshRenderData, indices are sorted in the ascending order.
         * The first index always goes after the last index from @ref vOpaqueShaders.
         */
        std::vector<ShaderInfo> vTransparentShaders;

        /**
         * Data for opaque and transparent meshes to submit for drawing.
         * See @ref vOpaqueShaders and @ref vTransparentShaders.
         * Actual (used) size of this array is @ref iRegisteredMeshCount.
         */
        alignas(hardware_constructive_interference_size)
            std::array<MeshRenderData, MAX_MESH_NODE_COUNT_TO_RENDER> vMeshRenderData;

        /**
         * Maps indices from @ref vMeshRenderData to registered handles.
         * It's safe to store raw pointers here because handles notify us in the destructor.
         * Actual (used) size of this array is @ref iRegisteredMeshCount.
         */
        std::array<MeshRenderingHandle*, MAX_MESH_NODE_COUNT_TO_RENDER> vIndexToHandle;

        /** Actual (used) size of arrays @ref vMeshRenderData and @ref vIndexToHandle. */
        unsigned short iRegisteredMeshCount = 0;
    };

    ~MeshRenderer();

    /**
     * Queues OpenGL draw commands to draw all spawned and visible meshes
     * on the currently set framebuffer.
     *
     * @param pRenderer            Renderer.
     * @param viewProjectionMatrix Camera's view projection matrix.
     * @param lightSourceManager   Light source manager.
     */
    void drawMeshes(
        Renderer* pRenderer, const glm::mat4& viewProjectionMatrix, LightSourceManager& lightSourceManager);

    /**
     * Registers a new mesh to be rendered.
     * Set mesh parameters using the returned handle.
     *
     * @param pShaderProgram      Shader program to use for rendering.
     * @param bEnableTransparency `true` to render the mesh as non-opaque.
     *
     * @return RAII-style handle for rendering.
     */
    std::unique_ptr<MeshRenderingHandle>
    addMeshForRendering(ShaderProgram* pShaderProgram, bool bEnableTransparency);

    /**
     * Returns render data of a mesh to initialize/modify.
     *
     * @param handle Handle of the mesh from @ref addMeshForRendering.
     *
     * @return Data guard used to modify the data.
     */
    MeshRenderDataGuard getMeshRenderData(MeshRenderingHandle& handle);

    /**
     * Returns shader constants setter that will be called for every shader program used for rendering to set
     * custom global parameters.
     *
     * @return Constants setter.
     */
    ShaderConstantsSetter& getGlobalShaderConstantsSetter() { return shaderConstantsSetter; }

private:
    MeshRenderer() = default;

    /**
     * Called from handle's destructor to remove a mesh from rendering.
     *
     * @param pHandle Handle.
     */
    void onBeforeHandleDestroyed(MeshRenderingHandle* pHandle);

#if defined(DEBUG)
    /**
     * Checks that all indices are correct.
     *
     * @remark Expects that @ref mtxRenderData is locked.
     */
    void runDebugIndexValidation();
#endif

    /**
     * Queues OpenGL draw commands to draw the specified meshes on the currently set framebuffer.
     *
     * @param vShaders             Shaders to draw.
     * @param data                 Render data.
     * @param viewProjectionMatrix Camera's view projection matrix.
     * @param ambientLightColor    Ambient light color.
     * @param pointLightData       Light array data.
     * @param spotlightData        Light array data.
     * @param directionalLightData Light array data.
     */
    void drawMeshes(
        const std::vector<RenderData::ShaderInfo>& vShaders,
        const RenderData& data,
        const glm::mat4& viewProjectionMatrix,
        const glm::vec3& ambientLightColor,
        LightSourceShaderArray::LightData& pointLightData,
        LightSourceShaderArray::LightData& spotlightData,
        LightSourceShaderArray::LightData& directionalLightData);

    /** Will be called for every shader program used for rendering to set custom global parameters. */
    ShaderConstantsSetter shaderConstantsSetter;

    /** Groups data for rendering. */
    std::pair<std::mutex, RenderData> mtxRenderData{};
};
