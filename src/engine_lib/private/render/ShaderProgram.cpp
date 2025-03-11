#include "render/ShaderProgram.h"

// Custom.
#include "render/ShaderManager.h"

ShaderProgram::~ShaderProgram() {
    {
        // Make sure no node is using us.
        std::scoped_lock guard(mtxMeshNodesUsingThisProgram.first);
        const auto iUsageCount = mtxMeshNodesUsingThisProgram.second.size();
        if (iUsageCount != 0) [[unlikely]] {
            Error error(std::format(
                "shader program \"{}\" is being destroyed but there are still {} nodes that use it",
                sShaderProgramName,
                iUsageCount));
            error.showErrorAndThrowException();
        }
    }

    pShaderManager->onShaderProgramBeingDestroyed(sShaderProgramName);

    GL_CHECK_ERROR(glDeleteProgram(iShaderProgramId));
}

ShaderProgram::ShaderProgram(
    ShaderManager* pShaderManager,
    const std::vector<std::shared_ptr<Shader>>& vLinkedShaders,
    unsigned int iShaderProgramId,
    std::string sShaderProgramName)
    : pShaderManager(pShaderManager), iShaderProgramId(iShaderProgramId), vLinkedShaders(vLinkedShaders),
      sShaderProgramName(sShaderProgramName) {}

void ShaderProgram::onMeshNodeStartedUsingProgram(MeshNode* pMeshNode) {
    std::scoped_lock guard(mtxMeshNodesUsingThisProgram.first);

    // Add.
    const auto [it, isAdded] = mtxMeshNodesUsingThisProgram.second.insert(pMeshNode);
    if (!isAdded) [[unlikely]] {
        Error error(std::format("shader program \"{}\" already has this node added", sShaderProgramName));
        error.showErrorAndThrowException();
    }
}

void ShaderProgram::onMeshNodeStoppedUsingProgram(MeshNode* pMeshNode) {
    std::scoped_lock guard(mtxMeshNodesUsingThisProgram.first);

    // Remove.
    const auto it = mtxMeshNodesUsingThisProgram.second.find(pMeshNode);
    if (it == mtxMeshNodesUsingThisProgram.second.end()) [[unlikely]] {
        Error error(
            std::format("shader program \"{}\" unable to find this node to be removed", sShaderProgramName));
        error.showErrorAndThrowException();
    }
    mtxMeshNodesUsingThisProgram.second.erase(it);
}
