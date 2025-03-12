#include "render/wrapper/ShaderProgram.h"

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
