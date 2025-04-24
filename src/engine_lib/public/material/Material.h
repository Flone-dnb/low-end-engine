#pragma once

// Standard.
#include <string>
#include <memory>
#include <functional>

// Custom.
#include "material/TextureHandle.h"
#include "math/GLMath.hpp"

class MeshNode;
class Renderer;
class Shader;
class ShaderProgram;

/** Material is a thin layer between a mesh and a shader. */
class Material {
    // Notifies us about node being visible.
    friend class MeshNode;

public:
    Material() = default;
    ~Material() = default;

    Material(const Material&) = delete;
    Material& operator=(const Material&) = delete;

    /** Move constructor. */
    Material(Material&&) noexcept = default;

    /**
     * Move assignment.
     * @return This.
     */
    Material& operator=(Material&&) noexcept = default;

    /**
     * Creates material with custom shaders.
     *
     * @param sPathToCustomVertexShader   Path to .glsl file relative `res` directory.
     * @param sPathToCustomFragmentShader Path to .glsl file relative `res` directory.
     */
    Material(const std::string& sPathToCustomVertexShader, const std::string& sPathToCustomFragmentShader);

    /**
     * Sets color of the diffuse light.
     *
     * @param color Color in the RGB format.
     */
    void setDiffuseColor(const glm::vec3& color);

    /**
     * Sets value in range [0.0; 1.0] where 1.0 means opaque and 0.0 transparent.
     *
     * @param opacity Opacity.
     */
    void setOpacity(float opacity);

    /**
     * Sets path to diffuse texture to load (if not loaded yet by some other spawned object) when spawning and
     * use.
     *
     * @param sPathToTextureRelativeRes Path to the texture file relative to the `res` directory.
     */
    void setPathToDiffuseTexture(const std::string& sPathToTextureRelativeRes);

    /**
     * Sets GLSL vertex shader to use instead of the default one.
     *
     * @param sPathToCustomVertexShader Path to .glsl file relative `res` directory.
     */
    void setPathToCustomVertexShader(const std::string& sPathToCustomVertexShader);

    /**
     * Sets GLSL fragment shader to use instead of the default one.
     *
     * @param sPathToCustomFragmentShader Path to .glsl file relative `res` directory.
     */
    void setPathToCustomFragmentShader(const std::string& sPathToCustomFragmentShader);

    /**
     * Returns color of the diffuse light.
     *
     * @return Diffuse light color.
     */
    glm::vec3 getDiffuseColor() { return diffuseColor; }

    /**
     * Returns value in range [0.0; 1.0] where 1 means opaque and 0 means transparent.
     *
     * @return Opacity.
     */
    float getOpacity() const { return diffuseColor.w; }

    /**
     * Returns GLSL shader that the material uses instead of the default one.
     *
     * @return Empty string if default shader is used, otherwise path to .glsl file relative `res` directory.
     */
    std::string getPathToCustomVertexShader() const { return sPathToCustomVertexShader; }

    /**
     * Returns GLSL shader that the material uses instead of the default one.
     *
     * @return Empty string if default shader is used, otherwise path to .glsl file relative `res` directory.
     */
    std::string getPathToCustomFragmentShader() const { return sPathToCustomFragmentShader; }

    /**
     * Returns path to diffuse texture to use.
     *
     * @return Path relative to the `res` directory.
     */
    std::string getPathToDiffuseTexture() const { return sPathToDiffuseTextureRelativeRes; }

private:
    /**
     * Called after node that owns this material was spawned.
     *
     * @param pNode      Node that triggered this event.
     * @param pRenderer Renderer.
     * @param onShaderProgramReceived Callback to query shader `uniform` locations from the program that will
     * be used to render this mesh node.
     */
    void onNodeSpawning(
        MeshNode* pNode,
        Renderer* pRenderer,
        const std::function<void(ShaderProgram*)>& onShaderProgramReceived);

    /**
     * Called before node that owns this material is despawned.
     *
     * @param pNode     Node that triggered this event.
     * @param pRenderer Renderer.
     */
    void onNodeDespawning(MeshNode* pNode, Renderer* pRenderer);

    /**
     * Called from node that owns this material after it changed its visibility.
     *
     * @param bIsVisible New visibility of the node.
     * @param pNode      Node that triggered this event.
     * @param pRenderer  Renderer.
     */
    void onNodeChangedVisibilityWhileSpawned(bool bIsVisible, MeshNode* pNode, Renderer* pRenderer);

    /** Diffuse light color. */
    glm::vec4 diffuseColor = glm::vec4(1.0F, 1.0F, 1.0F, 1.0F);

    /** Not `nullptr` if this material is used on a spawned renderable node. */
    std::shared_ptr<ShaderProgram> pShaderProgram;

    /** Not `nullptr` if texture from @ref sPathToDiffuseTextureRelativeRes is loaded. */
    std::unique_ptr<TextureHandle> pDiffuseTexture;

    /** Path to the texture (relative the `res` directory) to load. */
    std::string sPathToDiffuseTextureRelativeRes;

    /** Empty if using default shader, otherwise path to custom .glsl file (relative `res` directory). */
    std::string sPathToCustomVertexShader;

    /** Empty if using default shader, otherwise path to custom .glsl file (relative `res` directory). */
    std::string sPathToCustomFragmentShader;
};
