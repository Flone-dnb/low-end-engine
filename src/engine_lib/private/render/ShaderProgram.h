#pragma once

// Standard.
#include <memory>
#include <vector>
#include <string>
#include <mutex>
#include <unordered_set>
#include <string_view>
#include <format>

// Custom.
#include "misc/Error.h"
#include "math/GLMath.hpp"

// External.
#include "glad/glad.h"

class Shader;
class ShaderManager;
class MeshNode;

/**
 * Groups shaders used in an OpenGL shader program.
 *
 * @remark RAII-like object that automatically deletes OpenGL objects during destruction.
 */
class ShaderProgram {
    // Only the manager is allowed to create objects of this type.
    friend class ShaderManager;

    // Material notifies about nodes using this program.
    friend class Material;

public:
    ShaderProgram() = delete;

    ShaderProgram(const ShaderProgram&) = delete;
    ShaderProgram& operator=(const ShaderProgram&) = delete;

    ShaderProgram(ShaderProgram&&) noexcept = delete;
    ShaderProgram& operator=(ShaderProgram&&) noexcept = delete;

    ~ShaderProgram();

    /**
     * Returns location of a shader uniform with the specified name.
     *
     * @warning Shows an error if not found.
     *
     * @param sUniformName Name of a uniform.
     *
     * @return Location.
     */
    inline int getShaderUniformLocation(std::string_view sUniformName);

    /**
     * Sets the specified matrix to a `uniform` with the specified name in shaders.
     *
     * @param iUniformLocation Location from @ref getShaderUniformLocation.
     * @param matrix           Matrix to set.
     */
    inline void setMatrix4ToShader(int iUniformLocation, const glm::mat4x4& matrix);

    /**
     * Sets the specified matrix to a `uniform` with the specified name in shaders.
     *
     * @param iUniformLocation Location from @ref getShaderUniformLocation.
     * @param matrix           Matrix to set.
     */
    inline void setMatrix3ToShader(int iUniformLocation, const glm::mat3x3& matrix);

    /**
     * Sets the specified vector to a `uniform` with the specified name in shaders.
     *
     * @param iUniformLocation Location from @ref getShaderUniformLocation.
     * @param vector           Vector to set.
     */
    inline void setVector3ToShader(int iUniformLocation, const glm::vec3& vector);

    /**
     * Sets the specified float value to a `uniform` with the specified name in shaders.
     *
     * @param iUniformLocation Location from @ref getShaderUniformLocation.
     * @param value            Value to set.
     */
    inline void setFloatToShader(int iUniformLocation, float value);

    /**
     * Returns ID of this shader program.
     *
     * @return ID.
     */
    unsigned int getShaderProgramId() const { return iShaderProgramId; }

    /**
     * Returns all spawned mesh nodes that use this program.
     *
     * @return Mesh nodes.
     */
    std::pair<std::mutex, std::unordered_set<MeshNode*>>& getMeshNodesUsingThisProgram() {
        return mtxMeshNodesUsingThisProgram;
    }

private:
    /**
     * Creates a new shader program.
     *
     * @param pShaderManager     Manager that created this program.
     * @param vLinkedShaders     Linked shaders.
     * @param iShaderProgramId   ID of the compiled shader program.
     * @param sShaderProgramName Unique identifier of this shader program.
     */
    ShaderProgram(
        ShaderManager* pShaderManager,
        const std::vector<std::shared_ptr<Shader>>& vLinkedShaders,
        unsigned int iShaderProgramId,
        std::string sShaderProgramName);

    /**
     * Called after some material on a spawned mesh node started using this shader program.
     *
     * @param pMeshNode Node.
     */
    void onMeshNodeStartedUsingProgram(MeshNode* pMeshNode);

    /**
     * Called after some material on a spawned mesh node stopped using this shader program.
     *
     * @param pMeshNode Node.
     */
    void onMeshNodeStoppedUsingProgram(MeshNode* pMeshNode);

    /** Mesh nodes that use this shader program. */
    std::pair<std::mutex, std::unordered_set<MeshNode*>> mtxMeshNodesUsingThisProgram;

    /** Manager that created this program. */
    ShaderManager* const pShaderManager = nullptr;

    /** ID of the created shader program. */
    const unsigned int iShaderProgramId = 0;

    /** Shaders linked to the shader program, can be 1 or more shaders. */
    const std::vector<std::shared_ptr<Shader>> vLinkedShaders;

    /** Unique identifier of this shader program. */
    const std::string sShaderProgramName;
};

inline int ShaderProgram::getShaderUniformLocation(std::string_view sUniformName) {
    const auto iLocation = glGetUniformLocation(iShaderProgramId, sUniformName.data());
    if (iLocation < 0) [[unlikely]] {
        Error error(std::format("unable to get location for shader uniform named \"{}\"", sUniformName));
        error.showErrorAndThrowException();
    }
    return iLocation;
}

inline void ShaderProgram::setMatrix4ToShader(int iUniformLocation, const glm::mat4x4& matrix) {
    glUniformMatrix4fv(iUniformLocation, 1, GL_FALSE, glm::value_ptr(matrix));
}

inline void ShaderProgram::setMatrix3ToShader(int iUniformLocation, const glm::mat3x3& matrix) {
    glUniformMatrix3fv(iUniformLocation, 1, GL_FALSE, glm::value_ptr(matrix));
}

inline void ShaderProgram::setVector3ToShader(int iUniformLocation, const glm::vec3& vector) {
    glUniform3fv(iUniformLocation, 1, glm::value_ptr(vector));
}

inline void ShaderProgram::setFloatToShader(int iUniformLocation, float value) {
    glUniform1f(iUniformLocation, value);
}
