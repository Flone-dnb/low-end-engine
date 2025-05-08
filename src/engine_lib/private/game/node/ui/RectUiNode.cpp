#include "game/node/ui/RectUiNode.h"

// Custom.
#include "game/GameInstance.h"
#include "render/Renderer.h"
#include "render/UiManager.h"
#include "material/TextureManager.h"

// External.
#include "nameof.hpp"

namespace {
    constexpr std::string_view sTypeGuid = "ffd408f1-3e0b-4b2b-aa83-0e40d23d1769";
}

std::string RectUiNode::getTypeGuidStatic() { return sTypeGuid.data(); }
std::string RectUiNode::getTypeGuid() const { return sTypeGuid.data(); }

TypeReflectionInfo RectUiNode::getReflectionInfo() {
    ReflectedVariables variables;

    variables.vec4s[NAMEOF_MEMBER(&RectUiNode::color).data()] = ReflectedVariableInfo<glm::vec4>{
        .setter = [](Serializable* pThis,
                     const glm::vec4& newValue) { reinterpret_cast<RectUiNode*>(pThis)->setColor(newValue); },
        .getter = [](Serializable* pThis) -> glm::vec4 {
            return reinterpret_cast<RectUiNode*>(pThis)->getColor();
        }};

    variables.strings[NAMEOF_MEMBER(&RectUiNode::sPathToTextureRelativeRes).data()] =
        ReflectedVariableInfo<std::string>{
            .setter =
                [](Serializable* pThis, const std::string& sNewValue) {
                    reinterpret_cast<RectUiNode*>(pThis)->setPathToTexture(sNewValue);
                },
            .getter = [](Serializable* pThis) -> std::string {
                return reinterpret_cast<RectUiNode*>(pThis)->getPathToTexture();
            }};

    return TypeReflectionInfo(
        UiNode::getTypeGuidStatic(),
        NAMEOF_SHORT_TYPE(RectUiNode).data(),
        []() -> std::unique_ptr<Serializable> { return std::make_unique<RectUiNode>(); },
        std::move(variables));
}

RectUiNode::RectUiNode() : UiNode("Rect UI Node") {}
RectUiNode::RectUiNode(const std::string& sNodeName) : UiNode(sNodeName) {}

void RectUiNode::setColor(const glm::vec4& color) { this->color = color; }

void RectUiNode::setPathToTexture(std::string sPathToTextureRelativeRes) {
    // Normalize slash.
    for (size_t i = 0; i < sPathToTextureRelativeRes.size(); i++) {
        if (sPathToTextureRelativeRes[i] == '\\') {
            sPathToTextureRelativeRes[i] = '/';
        }
    }

    if (this->sPathToTextureRelativeRes == sPathToTextureRelativeRes) {
        return;
    }
    this->sPathToTextureRelativeRes = sPathToTextureRelativeRes;

    if (isSpawned()) {
        if (sPathToTextureRelativeRes.empty()) {
            pTexture = nullptr;
        } else {
            // Get texture.
            auto result = getGameInstanceWhileSpawned()->getRenderer()->getTextureManager().getTexture(
                this->sPathToTextureRelativeRes, TextureUsage::UI);
            if (std::holds_alternative<Error>(result)) [[unlikely]] {
                auto error = std::get<Error>(std::move(result));
                error.addCurrentLocationToErrorStack();
                error.showErrorAndThrowException();
            }
            pTexture = std::get<std::unique_ptr<TextureHandle>>(std::move(result));
        }
    }
}

void RectUiNode::onSpawning() {
    UiNode::onSpawning();

    if (!sPathToTextureRelativeRes.empty()) {
        // Get texture.
        auto result = getGameInstanceWhileSpawned()->getRenderer()->getTextureManager().getTexture(
            this->sPathToTextureRelativeRes, TextureUsage::UI);
        if (std::holds_alternative<Error>(result)) [[unlikely]] {
            auto error = std::get<Error>(std::move(result));
            error.addCurrentLocationToErrorStack();
            error.showErrorAndThrowException();
        }
        pTexture = std::get<std::unique_ptr<TextureHandle>>(std::move(result));
    }

    // Notify manager.
    auto& uiManager = getGameInstanceWhileSpawned()->getRenderer()->getUiManager();
    uiManager.onNodeSpawning(this);
}

void RectUiNode::onDespawning() {
    UiNode::onDespawning();

    // Notify manager.
    auto& uiManager = getGameInstanceWhileSpawned()->getRenderer()->getUiManager();
    uiManager.onNodeDespawning(this);

    pTexture = nullptr;
}

void RectUiNode::onVisibilityChanged() {
    UiNode::onVisibilityChanged();

    // Notify manager.
    auto& uiManager = getGameInstanceWhileSpawned()->getRenderer()->getUiManager();
    uiManager.onSpawnedNodeChangedVisibility(this);
}
