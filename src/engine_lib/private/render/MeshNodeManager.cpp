#include "MeshNodeManager.h"

// Custom.
#include "game/node/MeshNode.h"
#include "game/camera/CameraProperties.h"
#include "render/LightSourceManager.h"
#include "render/wrapper/ShaderProgram.h"

// External.
#include "glad/glad.h"

MeshNodeManager::~MeshNodeManager() {
    std::scoped_lock guard(mtxSpawnedVisibleNodes.first);

    if (!mtxSpawnedVisibleNodes.second.opaqueMeshes.empty() ||
        !mtxSpawnedVisibleNodes.second.transparentMeshes.empty()) [[unlikely]] {
        Error::showErrorAndThrowException(
            "mesh node manager is being destroyed but there are still some nodes registered");
    }
}

void MeshNodeManager::drawMeshes(
    CameraProperties* pCameraProperties, LightSourceManager& lightSourceManager) {
    std::scoped_lock guard(mtxSpawnedVisibleNodes.first);

    if (!mtxSpawnedVisibleNodes.second.opaqueMeshes.empty()) {
        drawMeshes(mtxSpawnedVisibleNodes.second.opaqueMeshes, pCameraProperties, lightSourceManager);
    }

    if (!mtxSpawnedVisibleNodes.second.transparentMeshes.empty()) {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        {
            drawMeshes(
                mtxSpawnedVisibleNodes.second.transparentMeshes, pCameraProperties, lightSourceManager);
        }
        glDisable(GL_BLEND);
    }
}

void MeshNodeManager::drawMeshes(
    std::unordered_map<ShaderProgram*, std::unordered_set<MeshNode*>>& meshes,
    CameraProperties* pCameraProperties,
    LightSourceManager& lightSourceManager) {
    for (auto& [pShaderProgram, meshNodes] : meshes) {
        PROFILE_SCOPE("render mesh nodes of shader program")
        PROFILE_ADD_SCOPE_TEXT(pShaderProgram->getName().data(), pShaderProgram->getName().size());

        // Set shader program.
        GL_CHECK_ERROR(glUseProgram(pShaderProgram->getShaderProgramId()));

        // Set camera uniforms.
        pCameraProperties->getShaderConstantsSetter().setConstantsToShader(pShaderProgram);

        // Set light arrays.
        lightSourceManager.setArrayPropertiesToShader(pShaderProgram);

        // Get mesh nodes.
        auto& mtxMeshNodes = pShaderProgram->getMeshNodesUsingThisProgram();
        std::scoped_lock guard(mtxMeshNodes.first);

        for (const auto& pMeshNode : mtxMeshNodes.second) {
#if defined(ENGINE_EDITOR)
            pShaderProgram->setUintToShader("iNodeId", static_cast<unsigned int>(*pMeshNode->getNodeId()));
#endif

            // Set mesh.
            auto& vao = pMeshNode->getVertexArrayObjectWhileSpawned();
            glBindVertexArray(vao.getVertexArrayObjectId());

            // Set mesh uniforms.
            pMeshNode->getShaderConstantsSetterWhileSpawned().setConstantsToShader(pShaderProgram);

            // Draw.
            glDrawElements(GL_TRIANGLES, vao.getIndexCount(), GL_UNSIGNED_SHORT, nullptr);
        }

        // Clear texture slots (if they where used).
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, 0);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, 0);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, 0);
    }
}

void MeshNodeManager::onMeshNodeSpawning(MeshNode* pNode) {
    if (!pNode->isVisible()) {
        return;
    }

    addMeshNodeToRendering(pNode);
}

void MeshNodeManager::onMeshNodeDespawning(MeshNode* pNode) {
    if (!pNode->isVisible()) {
        return;
    }

    removeMeshNodeFromRendering(pNode);
}

void MeshNodeManager::onSpawnedMeshNodeChangingVisibility(MeshNode* pNode, bool bNewVisibility) {
    if (bNewVisibility) {
        addMeshNodeToRendering(pNode);
    } else {
        removeMeshNodeFromRendering(pNode);
    }
}

void MeshNodeManager::addMeshNodeToRendering(MeshNode* pNode) {
    std::scoped_lock guard(mtxSpawnedVisibleNodes.first);

    std::unordered_map<ShaderProgram*, std::unordered_set<MeshNode*>>* pShaderToNodes = nullptr;

    if (pNode->getMaterial().isTransparencyEnabled()) {
        pShaderToNodes = &mtxSpawnedVisibleNodes.second.transparentMeshes;
    } else {
        pShaderToNodes = &mtxSpawnedVisibleNodes.second.opaqueMeshes;
    }

    // Get shader.
    const auto pShaderProgram = pNode->getMaterial().getShaderProgram();
    if (pShaderProgram == nullptr) [[unlikely]] {
        Error::showErrorAndThrowException(
            std::format("node \"{}\" material has invalid shader program", pNode->getNodeName()));
    }

    // Find array.
    auto meshesIt = pShaderToNodes->find(pShaderProgram);
    if (meshesIt == pShaderToNodes->end()) {
        bool bIsInserted = false;
        std::tie(meshesIt, bIsInserted) = pShaderToNodes->insert({pShaderProgram, {}});
    }

    // Add node.
    if (!meshesIt->second.insert(pNode).second) [[unlikely]] {
        Error::showErrorAndThrowException(
            std::format("node \"{}\" was already registered", pNode->getNodeName()));
    }
}

void MeshNodeManager::removeMeshNodeFromRendering(MeshNode* pNode) {
    std::scoped_lock guard(mtxSpawnedVisibleNodes.first);

    std::unordered_map<ShaderProgram*, std::unordered_set<MeshNode*>>* pShaderToNodes = nullptr;

    if (pNode->getMaterial().isTransparencyEnabled()) {
        pShaderToNodes = &mtxSpawnedVisibleNodes.second.transparentMeshes;
    } else {
        pShaderToNodes = &mtxSpawnedVisibleNodes.second.opaqueMeshes;
    }

    // Get shader.
    const auto pShaderProgram = pNode->getMaterial().getShaderProgram();
    if (pShaderProgram == nullptr) [[unlikely]] {
        Error::showErrorAndThrowException(
            std::format("node \"{}\" material has invalid shader program", pNode->getNodeName()));
    }

    // Find array.
    auto meshesIt = pShaderToNodes->find(pShaderProgram);
    if (meshesIt == pShaderToNodes->end()) [[unlikely]] {
        Error::showErrorAndThrowException(std::format(
            "unable to find shader program of node \"{}\" to unregister it", pNode->getNodeName()));
    }

    // Remove node.
    const auto nodeIt = meshesIt->second.find(pNode);
    if (nodeIt == meshesIt->second.end()) [[unlikely]] {
        Error::showErrorAndThrowException(
            std::format("unable to find node \"{}\" to unregister it", pNode->getNodeName()));
    }
    meshesIt->second.erase(nodeIt);

    if (meshesIt->second.empty()) {
        // Remove shader program.
        pShaderToNodes->erase(pShaderProgram);
    }
}
