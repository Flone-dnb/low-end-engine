#pragma once

// Standard.
#include <memory>
#include <vector>
#include <string>
#include <mutex>
#include <unordered_set>
#include <format>

// Custom.
#include "misc/Error.h"
#include "math/GLMath.hpp"
#include "render/wrapper/Buffer.h"
#include "render/ShaderManager.h"

// External.
#include "glad/glad.h"

class Shader;
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
     * Sets the specified buffer to shader.
     *
     * @param sUniformBlockName Name of the uniform block from shader code.
     * @param pBuffer Buffer to set.
     */
    inline void setUniformBlockToActiveProgram(const std::string& sUniformBlockName, Buffer* pBuffer);

    /**
     * Sets the specified value to a `uniform` with the specified name in shaders.
     *
     * @param sUniformName Name of the uniform variable from shader code.
     * @param matrix       Matrix to set.
     */
    inline void setMatrix4ToActiveProgram(const std::string& sUniformName, const glm::mat4x4& matrix);

    /**
     * Sets the specified value to a `uniform` with the specified name in shaders.
     *
     * @param sUniformName Name of the uniform variable from shader code.
     * @param iArraySize   Size of the GLSL array.
     * @param pArrayStart  Start of the array's data to copy.
     */
    inline void
    setMatrix4ArrayToActiveProgram(const std::string& sUniformName, int iArraySize, const float* pArrayStart);

    /**
     * Sets the specified value to a `uniform` with the specified name in shaders.
     *
     * @param sUniformName Name of the uniform variable from shader code.
     * @param matrix       Matrix to set.
     */
    inline void setMatrix3ToActiveProgram(const std::string& sUniformName, const glm::mat3x3& matrix);

    /**
     * Sets the specified value to a `uniform` with the specified name in shaders.
     *
     * @param sUniformName Name of the uniform variable from shader code.
     * @param vector       Vector to set.
     */
    inline void setVector2ToActiveProgram(const std::string& sUniformName, const glm::vec2& vector);

    /**
     * Sets the specified value to a `uniform` with the specified name in shaders.
     *
     * @param sUniformName Name of the uniform variable from shader code.
     * @param vector       Vector to set.
     */
    inline void setUvector2ToActiveProgram(const std::string& sUniformName, const glm::uvec2& vector);

    /**
     * Sets the specified value to a `uniform` with the specified name in shaders.
     *
     * @param sUniformName Name of the uniform variable from shader code.
     * @param vector       Vector to set.
     */
    inline void setVector3ToActiveProgram(const std::string& sUniformName, const glm::vec3& vector);

    /**
     * Sets the specified value to a `uniform` with the specified name in shaders.
     *
     * @param sUniformName Name of the uniform variable from shader code.
     * @param vector       Vector to set.
     */
    inline void setVector4ToActiveProgram(const std::string& sUniformName, const glm::vec4& vector);

    /**
     * Sets the specified value to a `uniform` with the specified name in shaders.
     *
     * @param sUniformName Name of the uniform variable from shader code.
     * @param value        Value to set.
     */
    inline void setFloatToActiveProgram(const std::string& sUniformName, float value);

    /**
     * Sets the specified value to a `uniform` with the specified name in shaders.
     *
     * @param sUniformName Name of the uniform variable from shader code.
     * @param iValue       Value to set.
     */
    inline void setUintToActiveProgram(const std::string& sUniformName, unsigned int iValue);

    /**
     * Sets the specified value to a `uniform` with the specified name in shaders.
     *
     * @param sUniformName Name of the uniform variable from shader code.
     * @param iValue       Value to set.
     */
    inline void setIntToActiveProgram(const std::string& sUniformName, int iValue);

    /**
     * Sets the specified value to a `uniform` with the specified name in shaders.
     *
     * @param sUniformName Name of the uniform variable from shader code.
     * @param bValue       Value to set.
     */
    inline void setBoolToActiveProgram(const std::string& sUniformName, bool bValue);

    /**
     * Returns location of a shader uniform with the specified name or -1 if not found.
     *
     * @warning Shows an error if not found.
     *
     * @param sUniformName Name of a uniform.
     *
     * @return Location.
     */
    inline int tryGetShaderUniformLocation(const std::string& sUniformName);

    /**
     * Returns location of a shader uniform with the specified name.
     *
     * @warning Shows an error if not found.
     *
     * @param sUniformName Name of a uniform.
     *
     * @return Location.
     */
    inline int getShaderUniformLocation(const std::string& sUniformName);

    /**
     * Returns binding index of a shader uniform block with the specified name.
     *
     * @warning Shows an error if not found.
     *
     * @param sUniformBlockName Name of a uniform block.
     *
     * @return Binding index.
     */
    inline unsigned int getShaderUniformBlockBindingIndex(const std::string& sUniformBlockName);

    /**
     * Return manager that created this program.
     *
     * @return Manager.
     */
    ShaderManager& getShaderManager() const { return *pShaderManager; }

    /**
     * Returns name of the shader program.
     *
     * @remark Generally used for logging.
     *
     * @return Name.
     */
    std::string_view getName() const { return sShaderProgramName; }

    /**
     * Returns ID of this shader program.
     *
     * @return ID.
     */
    unsigned int getShaderProgramId() const { return iShaderProgramId; }

    /**
     * Returns ID of the shader program that only has vertex shader linked
     * (if the original shader program had vertex shader linked).
     *
     * @return ID (may be invalid if there was not vertex shader linked).
     */
    unsigned int getVertexOnlyShaderProgramId() const { return iVertexOnlyShaderProgramId; }

private:
    /**
     * Creates a new shader program.
     *
     * @param pShaderManager     Manager that created this program.
     * @param vLinkedShaders     Linked shaders.
     * @param iShaderProgramId   ID of the compiled shader program.
     * @param sShaderProgramName Unique identifier of this shader program.
     * @param iVertexOnlyShaderProgramId ID of the shader program which has only vertex shader linked.
     */
    ShaderProgram(
        ShaderManager* pShaderManager,
        const std::vector<std::shared_ptr<Shader>>& vLinkedShaders,
        unsigned int iShaderProgramId,
        const std::string& sShaderProgramName,
        unsigned int iVertexOnlyShaderProgramId);

    /** Locations of all uniform variables. */
    std::unordered_map<std::string, int> cachedUniformLocations;

    /** Binding indices of all uniform blocks. */
    std::unordered_map<std::string, unsigned int> cachedUniformBlockBindingIndices;

    /** Shaders linked to the shader program, can be 1 or more shaders. */
    const std::vector<std::shared_ptr<Shader>> vLinkedShaders;

    /** Unique identifier of this shader program. */
    const std::string sShaderProgramName;

    /** Manager that created this program. */
    ShaderManager* const pShaderManager = nullptr;

    /** ID of the created shader program. */
    const unsigned int iShaderProgramId = 0;

    /** ID of the shader program which has only vertex shader linked. */
    const unsigned int iVertexOnlyShaderProgramId = 0;
};

inline int ShaderProgram::getShaderUniformLocation(const std::string& sUniformName) {
    const auto cachedIt = cachedUniformLocations.find(sUniformName);
    if (cachedIt == cachedUniformLocations.end()) [[unlikely]] {
        Error::showErrorAndThrowException(
            std::format("unable to find uniform \"{}\" location", sUniformName));
    }

    return cachedIt->second;
}

inline int ShaderProgram::tryGetShaderUniformLocation(const std::string& sUniformName) {
    const auto cachedIt = cachedUniformLocations.find(sUniformName);
    if (cachedIt == cachedUniformLocations.end()) {
        return -1;
    }

    return cachedIt->second;
}

inline unsigned int ShaderProgram::getShaderUniformBlockBindingIndex(const std::string& sUniformBlockName) {
    const auto cachedIt = cachedUniformBlockBindingIndices.find(sUniformBlockName);
    if (cachedIt == cachedUniformBlockBindingIndices.end()) [[unlikely]] {
        Error::showErrorAndThrowException(
            std::format("unable to find uniform block \"{}\" binding index", sUniformBlockName));
    }

    return cachedIt->second;
}

inline void
ShaderProgram::setMatrix4ToActiveProgram(const std::string& sUniformName, const glm::mat4x4& matrix) {
    glUniformMatrix4fv(getShaderUniformLocation(sUniformName), 1, GL_FALSE, glm::value_ptr(matrix));
}

inline void ShaderProgram::setMatrix4ArrayToActiveProgram(
    const std::string& sUniformName, int iArraySize, const float* pArrayStart) {
    glUniformMatrix4fv(getShaderUniformLocation(sUniformName), iArraySize, GL_FALSE, pArrayStart);
}

inline void
ShaderProgram::setMatrix3ToActiveProgram(const std::string& sUniformName, const glm::mat3x3& matrix) {
    glUniformMatrix3fv(getShaderUniformLocation(sUniformName), 1, GL_FALSE, glm::value_ptr(matrix));
}

inline void
ShaderProgram::setVector2ToActiveProgram(const std::string& sUniformName, const glm::vec2& vector) {
    glUniform2fv(getShaderUniformLocation(sUniformName), 1, glm::value_ptr(vector));
}

inline void
ShaderProgram::setUvector2ToActiveProgram(const std::string& sUniformName, const glm::uvec2& vector) {
    glUniform2uiv(getShaderUniformLocation(sUniformName), 1, glm::value_ptr(vector));
}

inline void
ShaderProgram::setVector3ToActiveProgram(const std::string& sUniformName, const glm::vec3& vector) {
    glUniform3fv(getShaderUniformLocation(sUniformName), 1, glm::value_ptr(vector));
}

inline void
ShaderProgram::setVector4ToActiveProgram(const std::string& sUniformName, const glm::vec4& vector) {
    glUniform4fv(getShaderUniformLocation(sUniformName), 1, glm::value_ptr(vector));
}

inline void ShaderProgram::setBoolToActiveProgram(const std::string& sUniformName, bool bValue) {
    glUniform1i(getShaderUniformLocation(sUniformName), static_cast<int>(bValue));
}

inline void ShaderProgram::setFloatToActiveProgram(const std::string& sUniformName, float value) {
    glUniform1f(getShaderUniformLocation(sUniformName), value);
}

inline void ShaderProgram::setUintToActiveProgram(const std::string& sUniformName, unsigned int iValue) {
    glUniform1ui(getShaderUniformLocation(sUniformName), iValue);
}

inline void ShaderProgram::setIntToActiveProgram(const std::string& sUniformName, int iValue) {
    glUniform1i(getShaderUniformLocation(sUniformName), iValue);
}

inline void
ShaderProgram::setUniformBlockToActiveProgram(const std::string& sUniformBlockName, Buffer* pBuffer) {
    glBindBufferBase(
        GL_UNIFORM_BUFFER, getShaderUniformBlockBindingIndex(sUniformBlockName), pBuffer->getBufferId());
}
