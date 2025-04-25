#pragma once

// Standard.
#include <string>
#include <unordered_map>
#include <memory>
#include <mutex>
#include <array>
#include <vector>

class Renderer;
class Shader;
class ShaderProgram;

/**
 * Determines the usage of a shader program.
 *
 * @remark Used to separate special types shaders.
 */
enum class ShaderProgramUsage : unsigned char {
    MESH_NODE = 0,
    TRANSPARENT_MESH_NODE, // TODO: not a really good solution because if we will have the same shader used
                           // for both opaque and transparent objects we will duplicate that shader
    OTHER,
    // ... new types go here ...

    COUNT, // marks the size of this enum
};

enum class EnginePredefinedMacro : unsigned char {
    MAX_POINT_LIGHT_COUNT = 0,
    MAX_SPOT_LIGHT_COUNT,
    MAX_DIRECTIONAL_LIGHT_COUNT,
    // ... new macros go here ...

    COUNT, // marks the size of this enum
};

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
     * Returns value of engine's predefined shader macro.
     *
     * @param macro Shader macro.
     *
     * @return Macro's value.
     */
    static size_t getEnginePredefinedMacroValue(EnginePredefinedMacro macro);

    /**
     * Looks if a shader from the specified path was already requested previously (cached) to return it,
     * otherwise loads the shader from disk, compiles and returns it.
     *
     * @param sPathToVertexShaderRelativeRes   Path to .glsl vertex shader file relative `res` directory.
     * @param sPathToFragmentShaderRelativeRes Path to .glsl fragment shader file relative `res` directory.
     * @param usage                            Usage.
     *
     * @return Compiled shader program.
     */
    std::shared_ptr<ShaderProgram> getShaderProgram(
        const std::string& sPathToVertexShaderRelativeRes,
        const std::string& sPathToFragmentShaderRelativeRes,
        ShaderProgramUsage usage);

    /**
     * Returns all loaded shader programs.
     *
     * @return Shader programs.
     */
    std::pair<
        std::mutex,
        std::array<
            std::unordered_map<std::string, std::pair<std::weak_ptr<ShaderProgram>, ShaderProgram*>>,
            static_cast<size_t>(ShaderProgramUsage::COUNT)>>&
    getShaderPrograms() {
        return mtxDatabase;
    }

    /**
     * Returns renderer that created this manager.
     *
     * @return Renderer.
     */
    Renderer& getRenderer() const { return *pRenderer; }

private:
    ShaderManager() = delete;

    /**
     * Creates a new manager.
     *
     * @param pRenderer Renderer.
     */
    ShaderManager(Renderer* pRenderer);

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
     * @param usage          Where this program is used.
     *
     * @return Compiled shader program.
     */
    std::shared_ptr<ShaderProgram> compileShaderProgram(
        const std::string& sProgramName,
        const std::vector<std::shared_ptr<Shader>>& vLinkedShaders,
        ShaderProgramUsage usage);

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
     * @param usage            Where this program is used.
     */
    void onShaderProgramBeingDestroyed(const std::string& sShaderProgramId, ShaderProgramUsage usage);

    /**
     * Stores pairs of "path to .glsl file relative `res` directory" - "loaded shader".
     *
     * @remark Storing weak_ptr here is safe because the Shader object will notify the manager in destructor.
     */
    std::pair<std::mutex, std::unordered_map<std::string, std::weak_ptr<Shader>>> mtxPathsToShaders;

    /**
     * All loaded shader programs.
     *
     * @remark Storing weak_ptr here is safe because the ShaderProgram object will notify the manager in
     * destructor.
     *
     * @remark Additionally storing a raw pointer for fast access without creating a shared_ptr out of
     * weak_ptr in cases where we guarantee that weak_ptr will be valid.
     */
    /** Stores pairs of "linked shader names" - "shader program". */
    std::pair<
        std::mutex,
        std::array<
            std::unordered_map<std::string, std::pair<std::weak_ptr<ShaderProgram>, ShaderProgram*>>,
            static_cast<size_t>(ShaderProgramUsage::COUNT)>>
        mtxDatabase;

    /** Do not delete/free. Renderer that created this manager. */
    Renderer* const pRenderer = nullptr;
};
