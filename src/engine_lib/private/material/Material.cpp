#include "material/Material.h"

// Standard.
#include <format>

// Custom.
#include "render/Renderer.h"
#include "render/ShaderManager.h"
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
    if (pShaderProgram == nullptr) {
        bIsTransparencyEnabled = bEnable;
        return;
    }

    if (pOwnerNode == nullptr) [[unlikely]] {
        Error::showErrorAndThrowException("expected owner node to be valid");
    }

    // Transparency is not expected to change while the mesh is registered for rendering
    // (shader program and mesh node manager do registration based on transparency), thus:
    const auto pNode = pOwnerNode;
    pNode->unregisterFromRendering(); // removes shader program and `pOwnerNode`
    {
        bIsTransparencyEnabled = bEnable;
    }
    pNode->registerToRendering();
}

void Material::setOpacity(float opacity) { diffuseColor.w = opacity; }

void Material::setPathToDiffuseTexture(std::string sPathToTextureRelativeRes) {
    // Normalize slash.
    for (size_t i = 0; i < sPathToTextureRelativeRes.size(); i++) {
        if (sPathToTextureRelativeRes[i] == '\\') {
            sPathToTextureRelativeRes[i] = '/';
        }
    }

    if (sPathToDiffuseTextureRelativeRes == sPathToTextureRelativeRes) {
        return;
    }

    // Make sure the path is valid.
    const auto pathToTexture =
        ProjectPaths::getPathToResDirectory(ResourceDirectory::ROOT) / sPathToTextureRelativeRes;
    if (!std::filesystem::exists(pathToTexture)) {
        Logger::get().error(std::format("path \"{}\" does not exist", pathToTexture.string()));
        return;
    }
    if (std::filesystem::is_directory(pathToTexture)) {
        Logger::get().error(
            std::format("expected the path \"{}\" to point to a file", pathToTexture.string()));
        return;
    }

    if (pShaderProgram == nullptr) {
        sPathToDiffuseTextureRelativeRes = sPathToTextureRelativeRes;
        return;
    }

    if (pOwnerNode == nullptr) [[unlikely]] {
        Error::showErrorAndThrowException("expected owner node to be valid");
    }

    // Re-run shader program initialization.
    const auto pNode = pOwnerNode;
    pNode->unregisterFromRendering(); // removes shader program and `pOwnerNode`
    {
        sPathToDiffuseTextureRelativeRes = sPathToTextureRelativeRes;
    }
    pNode->registerToRendering();
}

void Material::setPathToCustomVertexShader(std::string sPathToCustomVertexShader) {
    // Normalize slash.
    for (size_t i = 0; i < sPathToCustomVertexShader.size(); i++) {
        if (sPathToCustomVertexShader[i] == '\\') {
            sPathToCustomVertexShader[i] = '/';
        }
    }

    if (this->sPathToCustomVertexShader == sPathToCustomVertexShader) {
        return;
    }

    const auto pathToFile =
        ProjectPaths::getPathToResDirectory(ResourceDirectory::ROOT) / sPathToCustomVertexShader;
    if (!std::filesystem::exists(pathToFile)) {
        Logger::get().error(std::format("path \"{}\" does not exist", pathToFile.string()));
        return;
    }
    if (std::filesystem::is_directory(pathToFile)) {
        Logger::get().error(std::format("expected the path \"{}\" to point to a file", pathToFile.string()));
        return;
    }

    if (pShaderProgram == nullptr) {
        this->sPathToCustomVertexShader = sPathToCustomVertexShader;
        return;
    }

    if (pOwnerNode == nullptr) [[unlikely]] {
        Error::showErrorAndThrowException("expected owner node to be valid");
    }

    // Vertex shader is not expected to change while the mesh is registered for rendering thus:
    const auto pNode = pOwnerNode;
    pNode->unregisterFromRendering(); // removes shader program and `pOwnerNode`
    {
        this->sPathToCustomVertexShader = sPathToCustomVertexShader;
    }
    pNode->registerToRendering();
}

void Material::setPathToCustomFragmentShader(std::string sPathToCustomFragmentShader) {
    // Normalize slash.
    for (size_t i = 0; i < sPathToCustomFragmentShader.size(); i++) {
        if (sPathToCustomFragmentShader[i] == '\\') {
            sPathToCustomFragmentShader[i] = '/';
        }
    }

    if (this->sPathToCustomFragmentShader == sPathToCustomFragmentShader) {
        return;
    }

    const auto pathToFile =
        ProjectPaths::getPathToResDirectory(ResourceDirectory::ROOT) / sPathToCustomFragmentShader;
    if (!std::filesystem::exists(pathToFile)) {
        Logger::get().error(std::format("path \"{}\" does not exist", pathToFile.string()));
        return;
    }
    if (std::filesystem::is_directory(pathToFile)) {
        Logger::get().error(std::format("expected the path \"{}\" to point to a file", pathToFile.string()));
        return;
    }

    if (pShaderProgram == nullptr) {
        this->sPathToCustomFragmentShader = sPathToCustomFragmentShader;
        return;
    }

    if (pOwnerNode == nullptr) [[unlikely]] {
        Error::showErrorAndThrowException("expected owner node to be valid");
    }

    // Fragment shader is not expected to change while the mesh is registered for rendering thus:
    const auto pNode = pOwnerNode;
    pNode->unregisterFromRendering(); // removes shader program and `pOwnerNode`
    {
        this->sPathToCustomFragmentShader = sPathToCustomFragmentShader;
    }
    pNode->registerToRendering();
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
                                            : sPathToCustomFragmentShader);
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

    pOwnerNode = pNode;
}

void Material::onNodeDespawning(MeshNode* pNode, Renderer* pRenderer) {
    PROFILE_FUNC

    // Self check:
    if (pShaderProgram == nullptr) [[unlikely]] {
        Error::showErrorAndThrowException(
            std::format("material on node \"{}\" not requested shaders yet", pNode->getNodeName()));
    }

    // Unload stuff.
    pShaderProgram = {};
    if (pDiffuseTexture != nullptr) {
        pDiffuseTexture = nullptr;
    }

    pOwnerNode = nullptr;
}
