#pragma once

// Standard.
#include <string>
#include <memory>
#include <functional>

// Custom.
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
    /** Creates material with default shaders. */
    Material() = default;

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
    void setDiffuseColor(const glm::vec3 color);

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
    glm::vec3 diffuseColor = glm::vec3(1.0F, 1.0F, 1.0F);

    /** Not `nullptr` if this material is used on a spawned renderable node. */
    std::shared_ptr<ShaderProgram> shaderProgram;

    /** Empty if using default shader, otherwise path to custom .glsl file (relative `res` directory). */
    std::string sPathToCustomVertexShader;

    /** Empty if using default shader, otherwise path to custom .glsl file (relative `res` directory). */
    std::string sPathToCustomFragmentShader;
};
