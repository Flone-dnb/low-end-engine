#pragma once

// Standard.
#include <mutex>
#include <unordered_set>
#include <memory>

// Custom.
#include "render/wrapper/Buffer.h"
#include "render/shader/ShaderArrayIndexManager.h"

class Renderer;
class Node;
class LightSourceShaderArray;
class ShaderProgram;
class LightSourceManager;

/** RAII-style type that removes the light source from rendering during destruction. */
class ActiveLightSourceHandle {
    // Only the array is allowed to create objects of this type.
    friend class LightSourceShaderArray;

public:
    ActiveLightSourceHandle() = delete;

    ~ActiveLightSourceHandle();

    ActiveLightSourceHandle(const ActiveLightSourceHandle&&) = delete;
    ActiveLightSourceHandle& operator=(const ActiveLightSourceHandle&) = delete;
    ActiveLightSourceHandle(ActiveLightSourceHandle&&) = delete;
    ActiveLightSourceHandle& operator=(ActiveLightSourceHandle&&) = delete;

    /**
     * Called by the light node after its shader properties changed.
     *
     * @param pData Pointer to light properties to copy.
     */
    void copyNewProperties(const void* pData);

private:
    /**
     * Constructs a new handle.
     *
     * @param pArrayIndex Index into the array.
     * @param pArray      Array that this handle references.
     * @param pLightNode  Active light source node.
     */
    ActiveLightSourceHandle(
        LightSourceShaderArray* pArray, std::unique_ptr<ShaderArrayIndex> pArrayIndex, Node* pLightNode)
        : pArrayIndex(std::move(pArrayIndex)), pArray(pArray), pLightNode(pLightNode) {}

    /** Index into the array. */
    std::unique_ptr<ShaderArrayIndex> pArrayIndex;

    /** Light manager. */
    LightSourceShaderArray* const pArray = nullptr;

    /** Active light source node. */
    Node* const pLightNode = nullptr;
};

// ------------------------------------------------------------------------------------------------

/**
 * Manages properties of active (spawned and visible) light sources (of the same type) that will be rendered
 * and provides data (light properties) to copy to shaders.
 */
class LightSourceShaderArray {
    // Only the manager can create objects of this type.
    friend class LightSourceManager;

    // Notifies the manager about changed shader properties of light sources.
    friend class ActiveLightSourceHandle;

public:
    LightSourceShaderArray() = delete;

    LightSourceShaderArray(const LightSourceShaderArray&) = delete;
    LightSourceShaderArray& operator=(const LightSourceShaderArray&) = delete;
    LightSourceShaderArray(LightSourceShaderArray&&) noexcept = delete;
    LightSourceShaderArray& operator=(LightSourceShaderArray&&) noexcept = delete;

    ~LightSourceShaderArray();

    /**
     * Called by spawned light sources that need to be rendered.
     *
     * @param pLightSource Light to add to rendering.
     * @param pProperties  Initial light properties data to copy to shaders.
     *
     * @return `nullptr` if the maximum number of visible lights was reached (try again later), otherwise
     * handle of the specified light node.
     */
    std::unique_ptr<ActiveLightSourceHandle>
    addLightSourceToRendering(Node* pLightSource, const void* pProperties);

    /**
     * Sets array to be used in the shader.
     *
     * @param pShaderProgram Shader program.
     */
    void setArrayPropertiesToShader(ShaderProgram* pShaderProgram);

    /**
     * Returns the number of visible light sources.
     *
     * @return Light source count.
     */
    size_t getVisibleLightSourceCount();

private:
    /** Groups mutex-guarded data. */
    struct LightData {
        /** Spawned and visible light nodes. */
        std::unordered_set<Node*> visibleLightNodes;

        /** UBO that stores array of light sources (data from @ref visibleLightNodes). */
        std::unique_ptr<Buffer> pUniformBufferObject;

        /** Provides indices into the array. */
        std::unique_ptr<ShaderArrayIndexManager> pArrayIndexManager;
    };

    /**
     * Constructs a new array.
     *
     * @param pLightSourceManager     Manager.
     * @param iLightStructSizeInBytes Size in bytes of one struct of the light source.
     * @param iArrayMaxSize           Maximum size of the shader array.
     * @param sUniformBlockName       Name of a uniform block that this array handles (from shader code).
     * @param sLightCountUniformName  Name of a uniform that stores light count (from shader code).
     */
    LightSourceShaderArray(
        LightSourceManager* pLightSourceManager,
        unsigned int iLightStructSizeInBytes,
        unsigned int iArrayMaxSize,
        const std::string& sUniformBlockName,
        const std::string& sLightCountUniformName);

    /**
     * Called by spawned light sources that no longer need to be rendered.
     *
     * @param pLightSource Light to remove from rendering.
     */
    void removeLightSourceFromRendering(Node* pLightSource);

    /** All spawned and visible light nodes. */
    std::pair<std::recursive_mutex, LightData> mtxData;

    /** Manager. */
    LightSourceManager* const pLightSourceManager = nullptr;

    /** Size in bytes of a struct of the light source. */
    const unsigned int iActualLightStructSize = 0;

    /** @ref iActualLightStructSize but optionally padded for correct alignment. */
    unsigned int iPaddedLightStructSize = 0;

    /** Maximum size of the shader array. */
    const unsigned int iArrayMaxSize = 0;

    /** Name of a uniform block that this array handles (from shader code). */
    const std::string sUniformBlockName;

    /** Name of a uniform that stores light count (from shader code). */
    const std::string sLightCountUniformName;
};
