#include "game/node/ui/ProgressBarUiNode.h"

// Custom.
#include "game/GameInstance.h"
#include "render/Renderer.h"
#include "material/TextureManager.h"

// External.
#include "nameof.hpp"

namespace {
    constexpr std::string_view sTypeGuid = "0295e0b6-d6bd-4810-8e02-6033ae084b5b";
}

std::string ProgressBarUiNode::getTypeGuidStatic() { return sTypeGuid.data(); }
std::string ProgressBarUiNode::getTypeGuid() const { return sTypeGuid.data(); }

TypeReflectionInfo ProgressBarUiNode::getReflectionInfo() {
    ReflectedVariables variables;

    variables.strings[NAMEOF_MEMBER(&ProgressBarUiNode::sPathToForegroundTextureRelativeRes).data()] =
        ReflectedVariableInfo<std::string>{
            .setter =
                [](Serializable* pThis, const std::string& sNewValue) {
                    reinterpret_cast<ProgressBarUiNode*>(pThis)->setPathToForegroundTexture(sNewValue);
                },
            .getter = [](Serializable* pThis) -> std::string {
                return reinterpret_cast<ProgressBarUiNode*>(pThis)->getPathToForegroundTexture();
            }};

    variables.vec4s[NAMEOF_MEMBER(&ProgressBarUiNode::foregroundColor).data()] =
        ReflectedVariableInfo<glm::vec4>{
            .setter =
                [](Serializable* pThis, const glm::vec4& newValue) {
                    reinterpret_cast<ProgressBarUiNode*>(pThis)->setForegroundColor(newValue);
                },
            .getter = [](Serializable* pThis) -> glm::vec4 {
                return reinterpret_cast<ProgressBarUiNode*>(pThis)->getForegroundColor();
            }};

    variables.floats[NAMEOF_MEMBER(&ProgressBarUiNode::progressFactor).data()] = ReflectedVariableInfo<float>{
        .setter =
            [](Serializable* pThis, const float& newValue) {
                reinterpret_cast<ProgressBarUiNode*>(pThis)->setProgressFactor(newValue);
            },
        .getter = [](Serializable* pThis) -> float {
            return reinterpret_cast<ProgressBarUiNode*>(pThis)->getProgressFactor();
        }};

    return TypeReflectionInfo(
        RectUiNode::getTypeGuidStatic(),
        NAMEOF_SHORT_TYPE(ProgressBarUiNode).data(),
        []() -> std::unique_ptr<Serializable> { return std::make_unique<ProgressBarUiNode>(); },
        std::move(variables));
}

ProgressBarUiNode::ProgressBarUiNode() : ProgressBarUiNode("Progress Bar UI Node") {}
ProgressBarUiNode::ProgressBarUiNode(const std::string& sNodeName) : RectUiNode(sNodeName) {}

void ProgressBarUiNode::setForegroundColor(const glm::vec4& foregroundColor) {
    this->foregroundColor = glm::clamp(foregroundColor, 0.0F, 1.0F);
}

void ProgressBarUiNode::setProgressFactor(float progress) {
    progressFactor = std::clamp(progress, 0.0F, 1.0F);
}

void ProgressBarUiNode::setPathToForegroundTexture(std::string sNewPathToForegroundTextureRelativeRes) {
    // Normalize slash.
    for (size_t i = 0; i < sNewPathToForegroundTextureRelativeRes.size(); i++) {
        if (sNewPathToForegroundTextureRelativeRes[i] == '\\') {
            sNewPathToForegroundTextureRelativeRes[i] = '/';
        }
    }

    if (sPathToForegroundTextureRelativeRes == sNewPathToForegroundTextureRelativeRes) {
        return;
    }
    this->sPathToForegroundTextureRelativeRes = sNewPathToForegroundTextureRelativeRes;

    // Make sure the path is valid.
    const auto pathToTexture =
        ProjectPaths::getPathToResDirectory(ResourceDirectory::ROOT) / sPathToForegroundTextureRelativeRes;
    if (!std::filesystem::exists(pathToTexture)) {
        Log::error(std::format("path \"{}\" does not exist", pathToTexture.string()));
        return;
    }
    if (std::filesystem::is_directory(pathToTexture)) {
        Log::error(std::format("expected the path \"{}\" to point to a file", pathToTexture.string()));
        return;
    }

    if (isSpawned()) {
        if (sPathToForegroundTextureRelativeRes.empty()) {
            pForegroundTexture = nullptr;
        } else {
            // Get texture.
            auto result = getGameInstanceWhileSpawned()->getRenderer()->getTextureManager().getTexture(
                sPathToForegroundTextureRelativeRes, TextureUsage::UI);
            if (std::holds_alternative<Error>(result)) [[unlikely]] {
                auto error = std::get<Error>(std::move(result));
                error.addCurrentLocationToErrorStack();
                error.showErrorAndThrowException();
            }
            pForegroundTexture = std::get<std::unique_ptr<TextureHandle>>(std::move(result));
        }
    }
}

void ProgressBarUiNode::onSpawning() {
    RectUiNode::onSpawning();

    if (!sPathToForegroundTextureRelativeRes.empty()) {
        // Get texture.
        auto result = getGameInstanceWhileSpawned()->getRenderer()->getTextureManager().getTexture(
            sPathToForegroundTextureRelativeRes, TextureUsage::UI);
        if (std::holds_alternative<Error>(result)) [[unlikely]] {
            auto error = std::get<Error>(std::move(result));
            error.addCurrentLocationToErrorStack();
            error.showErrorAndThrowException();
        }
        pForegroundTexture = std::get<std::unique_ptr<TextureHandle>>(std::move(result));
    }

    // Parent already notified UI manager.
}

void ProgressBarUiNode::onDespawning() {
    RectUiNode::onDespawning();

    // Parent already notified UI manager.

    pForegroundTexture = nullptr;
}