#include "material/Material.h"

// Standard.
#include <format>

// Custom.
#include "render/Renderer.h"
#include "render/shader/ShaderManager.h"
#include "game/node/MeshNode.h"
#include "render/wrapper/ShaderProgram.h"
#include "material/TextureManager.h"

// External.
#include "nameof.hpp"

Material::Material(
    const std::string& sPathToCustomVertexShader, const std::string& sPathToCustomFragmentShader)
    : sPathToCustomVertexShader(sPathToCustomVertexShader),
      sPathToCustomFragmentShader(sPathToCustomFragmentShader) {}

void Material::setDiffuseColor(const glm::vec3& color) {
    diffuseColor = glm::vec4(color.x, color.y, color.z, diffuseColor.w);
}

void Material::setEnableTransparency(bool bEnable) {
    if (pShaderProgram != nullptr) [[unlikely]] {
        // not allowed because this means we have to use something like `onNodeSpawning` so just won't allow
        // for simplicity, plus if this function is called from a non-main thread it will add more headache
        //
        // moreover, ShaderProgram expects that we don't change our transparency state while using it
        Error::showErrorAndThrowException(
            "changing material's transparency state (enabled/disabled) is not allowed while the material is "
            "used on a spawned node");
    }

    bIsTransparencyEnabled = bEnable;
}

void Material::setOpacity(float opacity) { diffuseColor.w = opacity; }

void Material::setPathToDiffuseTexture(const std::string& sPathToTextureRelativeRes) {
    if (pShaderProgram != nullptr) [[unlikely]] {
        // not allowed because this means we have to use something like `onNodeSpawning` so just won't allow
        // for simplicity, plus if this function is called from a non-main thread it will add more headache
        Error::showErrorAndThrowException(
            "changing material's textures is not allowed while the material is used on a spawned node");
    }

    if (!sPathToDiffuseTextureRelativeRes.empty()) {
        if (sPathToDiffuseTextureRelativeRes == sPathToTextureRelativeRes) {
            return;
        }

        sPathToDiffuseTextureRelativeRes = "";
        pDiffuseTexture = nullptr;
    }

    sPathToDiffuseTextureRelativeRes = sPathToTextureRelativeRes;

    if (pShaderProgram != nullptr) {
        // Get texture.
        auto result = pShaderProgram->getShaderManager().getRenderer().getTextureManager().getTexture(
            sPathToDiffuseTextureRelativeRes, TextureUsage::DIFFUSE);
        if (std::holds_alternative<Error>(result)) [[unlikely]] {
            auto error = std::get<Error>(std::move(result));
            error.addCurrentLocationToErrorStack();
            error.showErrorAndThrowException();
        }
        pDiffuseTexture = std::get<std::unique_ptr<TextureHandle>>(std::move(result));
    }
}

void Material::setPathToCustomVertexShader(const std::string& sPathToCustomVertexShader) {
    if (pShaderProgram != nullptr) [[unlikely]] {
        // not allowed because this means we have to use something like `onNodeSpawning` so just won't allow
        // for simplicity
        Error::showErrorAndThrowException(
            "changing material's shaders is not allowed while the material is used on a spawned node");
    }

    this->sPathToCustomVertexShader = sPathToCustomVertexShader;
}

void Material::setPathToCustomFragmentShader(const std::string& sPathToCustomFragmentShader) {
    if (pShaderProgram != nullptr) [[unlikely]] {
        // not allowed because this means we have to use something like `onNodeSpawning` so just won't allow
        // for simplicity
        Error::showErrorAndThrowException(
            "changing material's shaders is not allowed while the material is used on a spawned node");
    }

    this->sPathToCustomFragmentShader = sPathToCustomFragmentShader;
}

void Material::onNodeSpawning(
    MeshNode* pNode,
    Renderer* pRenderer,
    const std::function<void(ShaderProgram*)>& onShaderProgramReceived) {
    PROFILE_FUNC

    // Self check:
    if (pShaderProgram != nullptr) [[unlikely]] {
        Error::showErrorAndThrowException(
            std::format("material on node \"{}\" already requested shaders", pNode->getNodeName()));
    }

    // Get program.
    auto& shaderManager = pRenderer->getShaderManager();
    pShaderProgram = shaderManager.getShaderProgram(
        sPathToCustomVertexShader.empty() ? MeshNode::getDefaultVertexShaderForMeshNode().data()
                                          : sPathToCustomVertexShader,
        sPathToCustomFragmentShader.empty() ? MeshNode::getDefaultFragmentShaderForMeshNode().data()
                                            : sPathToCustomFragmentShader,
        bIsTransparencyEnabled ? ShaderProgramUsage::TRANSPARENT_MESH_NODE : ShaderProgramUsage::MESH_NODE);
    onShaderProgramReceived(pShaderProgram.get());

// Set general shader constants (branch independent constants).
#define SHADER_CONSTANTS_CODE                                                                                \
    pShaderProgram->setVector4ToShader(NAMEOF(diffuseColor).c_str(), diffuseColor);                          \
    pShaderProgram->setBoolToShader("bIsUsingDiffuseTexture", false);                                        \
    glActiveTexture(GL_TEXTURE0);                                                                            \
    glBindTexture(GL_TEXTURE_2D, 0);

    if (!sPathToDiffuseTextureRelativeRes.empty()) {
        // Get texture.
        auto result = pRenderer->getTextureManager().getTexture(
            sPathToDiffuseTextureRelativeRes, TextureUsage::DIFFUSE);
        if (std::holds_alternative<Error>(result)) [[unlikely]] {
            auto error = std::get<Error>(std::move(result));
            error.addCurrentLocationToErrorStack();
            error.showErrorAndThrowException();
        }
        pDiffuseTexture = std::get<std::unique_ptr<TextureHandle>>(std::move(result));

        pNode->getShaderConstantsSetterWhileSpawned().addSetterFunction(
            [this](ShaderProgram* pShaderProgram) {
                SHADER_CONSTANTS_CODE

                pShaderProgram->setBoolToShader("bIsUsingDiffuseTexture", true);
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, pDiffuseTexture->getTextureId());
            });
    } else {
        pNode->getShaderConstantsSetterWhileSpawned().addSetterFunction(
            [this](ShaderProgram* pShaderProgram) { SHADER_CONSTANTS_CODE });
    }

    if (pNode->isVisible()) {
        // Add node to be rendered.
        pShaderProgram->onMeshNodeStartedUsingProgram(pNode);
    }
}

void Material::onNodeDespawning(MeshNode* pNode, Renderer* pRenderer) {
    PROFILE_FUNC

    // Self check:
    if (pShaderProgram == nullptr) [[unlikely]] {
        Error::showErrorAndThrowException(
            std::format("material on node \"{}\" not requested shaders yet", pNode->getNodeName()));
    }

    if (pNode->isVisible()) {
        // Remove node from rendering.
        pShaderProgram->onMeshNodeStoppedUsingProgram(pNode);
    }

    // Unload stuff.
    pShaderProgram = {};
    if (pDiffuseTexture != nullptr) {
        pDiffuseTexture = nullptr;
    }
}

void Material::onNodeChangedVisibilityWhileSpawned(bool bIsVisible, MeshNode* pNode, Renderer* pRenderer) {
    if (bIsVisible) {
        // Add node to be rendered.
        pShaderProgram->onMeshNodeStartedUsingProgram(pNode);
    } else {
        // Remove node from rendering.
        pShaderProgram->onMeshNodeStoppedUsingProgram(pNode);
    }
}
