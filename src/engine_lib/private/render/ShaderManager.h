#pragma once

// Standard.
#include <string>
#include <unordered_map>
#include <memory>
#include <mutex>

class Shader;
class ShaderProgram;

/** Loads, compiles GLSL code and keeps track of all loaded shaders. */
class ShaderManager {
    // Only renderer is expected to create objects of this type.
    friend class Renderer;

    // Shader and program notify the manager when being destructed.
    friend class Shader;
    friend class ShaderProgram;

public:
    ~ShaderManager();

    /**
     * Looks if a shader from the specified path was already requested previously (cached) to return it,
     * otherwise loads the shader from disk, compiles and returns it.
     *
     * @param sPathToVertexShaderRelativeRes   Path to .glsl vertex shader file relative `res` directory.
     * @param sPathToFragmentShaderRelativeRes Path to .glsl fragment shader file relative `res` directory.
     *
     * @return Compiled shader program.
     */
    std::shared_ptr<ShaderProgram> getShaderProgram(
        const std::string& sPathToVertexShaderRelativeRes,
        const std::string& sPathToFragmentShaderRelativeRes);

    /**
     * Returns all loaded shader programs.
     *
     * @return Shader programs.
     */
    std::pair<
        std::mutex,
        std::unordered_map<std::string, std::pair<std::weak_ptr<ShaderProgram>, ShaderProgram*>>>&
    getShaderPrograms() {
        return mtxShaderPrograms;
    }

private:
    ShaderManager() = default;

    /**
     * Compiles a .glsl shader file.
     *
     * @param sPathToShaderRelativeRes Path to .glsl file relative `res` directory.
     *
     * @return Compiled shader.
     */
    std::shared_ptr<Shader> compileShader(const std::string& sPathToShaderRelativeRes);

    /**
     * Compiles a shader program from 1 or more shaders.
     *
     * @param sProgramName   Unique name of the shader program.
     * @param vLinkedShaders Shaders to link to this program.
     *
     * @return Compiled shader program.
     */
    std::shared_ptr<ShaderProgram> compileShaderProgram(
        const std::string& sProgramName, const std::vector<std::shared_ptr<Shader>>& vLinkedShaders);

    /**
     * Looks if a shader from the specified path was already requested previously (cached) to return it,
     * otherwise loads the shader from disk, compiles and returns it.
     *
     * @param sPathToShaderRelativeRes Path to .glsl file relative `res` directory.
     *
     * @return Loaded and compiled shader.
     */
    std::shared_ptr<Shader> getShader(const std::string& sPathToShaderRelativeRes);

    /**
     * Called from shader's destructor.
     *
     * @param sPathToShaderRelativeRes Path to .glsl file relative `res` directory.
     */
    void onShaderBeingDestroyed(const std::string& sPathToShaderRelativeRes);

    /**
     * Called from shader program's destructor.
     *
     * @param sShaderProgramId Unique identifier of a shader program.
     */
    void onShaderProgramBeingDestroyed(const std::string& sShaderProgramId);

    /**
     * Stores pairs of "path to .glsl file relative `res` directory" - "loaded shader".
     *
     * @remark Storing weak_ptr here is safe because the Shader object will notify the manager in destructor.
     */
    std::pair<std::mutex, std::unordered_map<std::string, std::weak_ptr<Shader>>> mtxPathsToShaders;

    /**
     * Stores pairs of "linked shader names" - "shader program".
     *
     * @remark Storing weak_ptr here is safe because the ShaderProgram object will notify the manager in
     * destructor.
     *
     * @remark Additionally storing a raw pointer for fast access without creating a shared_ptr out of
     * weak_ptr in cases where we guarantee that weak_ptr will be valid.
     */
    std::pair<
        std::mutex,
        std::unordered_map<std::string, std::pair<std::weak_ptr<ShaderProgram>, ShaderProgram*>>>
        mtxShaderPrograms;
};
