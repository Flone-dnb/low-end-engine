#include "render/wrapper/ShaderProgram.h"

// Standard.
#include <array>

// Custom.
#include "render/shader/ShaderManager.h"

ShaderProgram::~ShaderProgram() {
    {
        // Make sure no node is using us.
        std::scoped_lock guard(mtxMeshNodesUsingThisProgram.first);
        const auto iUsageCount = mtxMeshNodesUsingThisProgram.second.size();
        if (iUsageCount != 0) [[unlikely]] {
            Error::showErrorAndThrowException(std::format(
                "shader program \"{}\" is being destroyed but there are still {} nodes that use it",
                sShaderProgramName,
                iUsageCount));
        }
    }

    pShaderManager->onShaderProgramBeingDestroyed(sShaderProgramName, usage);

    GL_CHECK_ERROR(glDeleteProgram(iShaderProgramId));
}

ShaderProgram::ShaderProgram(
    ShaderManager* pShaderManager,
    const std::vector<std::shared_ptr<Shader>>& vLinkedShaders,
    unsigned int iShaderProgramId,
    std::string sShaderProgramName,
    ShaderProgramUsage usage)
    : pShaderManager(pShaderManager), iShaderProgramId(iShaderProgramId), vLinkedShaders(vLinkedShaders),
      sShaderProgramName(sShaderProgramName), usage(usage) {
    // Get total uniform count.
    int iUniformCount = 0;
    GL_CHECK_ERROR(glGetProgramiv(iShaderProgramId, GL_ACTIVE_UNIFORMS, &iUniformCount));

    std::array<char, 1024> vNameBuffer{};
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

void ShaderProgram::onMeshNodeStartedUsingProgram(MeshNode* pMeshNode) {
    std::scoped_lock guard(mtxMeshNodesUsingThisProgram.first);

    // Add.
    const auto [it, isAdded] = mtxMeshNodesUsingThisProgram.second.insert(pMeshNode);
    if (!isAdded) [[unlikely]] {
        Error::showErrorAndThrowException(
            std::format("shader program \"{}\" already has this node added", sShaderProgramName));
    }
}

void ShaderProgram::onMeshNodeStoppedUsingProgram(MeshNode* pMeshNode) {
    std::scoped_lock guard(mtxMeshNodesUsingThisProgram.first);

    // Remove.
    const auto it = mtxMeshNodesUsingThisProgram.second.find(pMeshNode);
    if (it == mtxMeshNodesUsingThisProgram.second.end()) [[unlikely]] {
        Error::showErrorAndThrowException(
            std::format("shader program \"{}\" unable to find this node to be removed", sShaderProgramName));
    }
    mtxMeshNodesUsingThisProgram.second.erase(it);
}
