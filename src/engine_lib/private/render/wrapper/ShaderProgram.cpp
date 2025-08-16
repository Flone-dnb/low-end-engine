#include "render/wrapper/ShaderProgram.h"

// Standard.
#include <array>

// Custom.
#include "render/ShaderManager.h"

ShaderProgram::~ShaderProgram() {
    pShaderManager->onShaderProgramBeingDestroyed(sShaderProgramName);

    GL_CHECK_ERROR(glDeleteProgram(iShaderProgramId));
}

ShaderProgram::ShaderProgram(
    ShaderManager* pShaderManager,
    const std::vector<std::shared_ptr<Shader>>& vLinkedShaders,
    unsigned int iShaderProgramId,
    const std::string& sShaderProgramName)
    : pShaderManager(pShaderManager), iShaderProgramId(iShaderProgramId), vLinkedShaders(vLinkedShaders),
      sShaderProgramName(sShaderProgramName) {
    // Get total uniform count.
    int iUniformCount = 0;
    GL_CHECK_ERROR(glGetProgramiv(iShaderProgramId, GL_ACTIVE_UNIFORMS, &iUniformCount));

    std::array<char, 1024> vNameBuffer{}; // NOLINT: max name length
    for (int i = 0; i < iUniformCount; i++) {
        vNameBuffer = {};

        // Get name.
        int iSize = 0;
        unsigned int iType = 0;
        GL_CHECK_ERROR(glGetActiveUniform(
            iShaderProgramId,
            i,
            static_cast<int>(vNameBuffer.size()),
            nullptr,
            &iSize,
            &iType,
            vNameBuffer.data()));

        // Get location.
        const auto iLocation = glGetUniformLocation(iShaderProgramId, vNameBuffer.data());
        if (iLocation < 0) {
            // Ignore, maybe it's a member of a uniform block.
            continue;
        }

        // Cache location.
        cachedUniformLocations[vNameBuffer.data()] = iLocation;
    }

    // Get uniform block count.
    GL_CHECK_ERROR(glGetProgramiv(iShaderProgramId, GL_ACTIVE_UNIFORM_BLOCKS, &iUniformCount));
    for (int i = 0; i < iUniformCount; i++) {
        // Get name.
        GL_CHECK_ERROR(glGetActiveUniformBlockName(
            iShaderProgramId, i, static_cast<int>(vNameBuffer.size()), nullptr, vNameBuffer.data()));

        // Get location.
        const auto iLocation = glGetUniformBlockIndex(iShaderProgramId, vNameBuffer.data());
        if (iLocation == GL_INVALID_INDEX) [[unlikely]] {
            Error::showErrorAndThrowException(std::format(
                "unable to get location for shader uniform block named \"{}\"", vNameBuffer.data()));
        }

        // Set binding.
        const auto iBindingIndex = static_cast<unsigned int>(cachedUniformBlockBindingIndices.size());
        GL_CHECK_ERROR(glUniformBlockBinding(iShaderProgramId, iLocation, iBindingIndex));

        // Cache location.
        cachedUniformBlockBindingIndices[vNameBuffer.data()] = iBindingIndex;
    }
}
