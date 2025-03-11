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
     * @param pathToCustomVertexShader   Path to .glsl file relative `res` directory.
     * @param pathToCustomFragmentShader Path to .glsl file relative `res` directory.
     */
    Material(const std::string& pathToCustomVertexShader, const std::string& pathToCustomFragmentShader);

    /**
     * Sets color of the diffuse light.
     *
     * @param color Color in the RGB format.
     */
    void setDiffuseColor(const glm::vec3 color);

    /**
     * Returns color of the diffuse light.
     *
     * @return Diffuse light color.
     */
    glm::vec3 getDiffuseColor() { return diffuseColor; }

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

    /** Not `nullptr` if this material is used on a visible renderable node. */
    std::shared_ptr<ShaderProgram> shaderProgram;

    /** Empty if using default shader, otherwise path to custom .glsl file (relative `res` directory). */
    std::string sPathToCustomVertexShader;

    /** Empty if using default shader, otherwise path to custom .glsl file (relative `res` directory). */
    std::string sPathToCustomFragmentShader;
};
