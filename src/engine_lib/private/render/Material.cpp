#include "render/Material.h"

// Standard.
#include <format>

// Custom.
#include "render/Renderer.h"
#include "render/shader/ShaderManager.h"
#include "game/node/MeshNode.h"
#include "render/wrapper/ShaderProgram.h"

// External.
#include "nameof.hpp"

Material::Material(const std::string& pathToCustomVertexShader, const std::string& pathToCustomFragmentShader)
    : sPathToCustomVertexShader(pathToCustomVertexShader),
      sPathToCustomFragmentShader(pathToCustomFragmentShader) {}

void Material::setDiffuseColor(const glm::vec3 color) { diffuseColor = color; }

void Material::onNodeSpawning(
    MeshNode* pNode,
    Renderer* pRenderer,
    const std::function<void(ShaderProgram*)>& onShaderProgramReceived) {
    // Self check:
    if (shaderProgram != nullptr) [[unlikely]] {
        Error::showErrorAndThrowException(
            std::format("material on node \"{}\" already requested shaders", pNode->getNodeName()));
    }

    // Get program.
    auto& shaderManager = pRenderer->getShaderManager();
    shaderProgram = shaderManager.getShaderProgram(
        sPathToCustomVertexShader.empty() ? MeshNode::getDefaultVertexShaderForMeshNode().data()
                                          : sPathToCustomVertexShader,
        sPathToCustomFragmentShader.empty() ? MeshNode::getDefaultFragmentShaderForMeshNode().data()
                                            : sPathToCustomFragmentShader);
    onShaderProgramReceived(shaderProgram.get());

    // Set shader constants.
    pNode->getShaderConstantsSetterWhileSpawned().addSetterFunction(
        [this, diffuseColorLocation = shaderProgram->getShaderUniformLocation(NAMEOF(diffuseColor))](
            ShaderProgram* pShaderProgram) {
            pShaderProgram->setVector3ToShader(diffuseColorLocation, diffuseColor);
        });

    if (pNode->isVisible()) {
        // Add node to be rendered.
        shaderProgram->onMeshNodeStartedUsingProgram(pNode);
    }
}

void Material::onNodeDespawning(MeshNode* pNode, Renderer* pRenderer) {
    // Self check:
    if (shaderProgram == nullptr) [[unlikely]] {
        Error::showErrorAndThrowException(
            std::format("material on node \"{}\" not requested shaders yet", pNode->getNodeName()));
    }

    if (pNode->isVisible()) {
        // Remove node from rendering.
        shaderProgram->onMeshNodeStoppedUsingProgram(pNode);
    }

    // Remove program.
    shaderProgram = {};
}

void Material::onNodeChangedVisibilityWhileSpawned(bool bIsVisible, MeshNode* pNode, Renderer* pRenderer) {
    if (bIsVisible) {
        // Add node to be rendered.
        shaderProgram->onMeshNodeStartedUsingProgram(pNode);
    } else {
        // Remove node from rendering.
        shaderProgram->onMeshNodeStoppedUsingProgram(pNode);
    }
}
