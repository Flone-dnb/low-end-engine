#include "game/node/ui/RectUiNode.h"

// Custom.
#include "game/GameInstance.h"
#include "render/Renderer.h"
#include "render/UiNodeManager.h"
#include "material/TextureManager.h"
#include "game/World.h"

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

    variables.floats[NAMEOF_MEMBER(&RectUiNode::padding).data()] = ReflectedVariableInfo<float>{
        .setter = [](Serializable* pThis,
                     const float& newValue) { reinterpret_cast<RectUiNode*>(pThis)->setPadding(newValue); },
        .getter = [](Serializable* pThis) -> float {
            return reinterpret_cast<RectUiNode*>(pThis)->getPadding();
        }};

    return TypeReflectionInfo(
        UiNode::getTypeGuidStatic(),
        NAMEOF_SHORT_TYPE(RectUiNode).data(),
        []() -> std::unique_ptr<Serializable> { return std::make_unique<RectUiNode>(); },
        std::move(variables));
}

RectUiNode::RectUiNode() : RectUiNode("Rect UI Node") {}
RectUiNode::RectUiNode(const std::string& sNodeName) : UiNode(sNodeName) {}

void RectUiNode::setColor(const glm::vec4& color) {
    this->color = glm::clamp(color, 0.0F, 1.0F);

    if (isSpawned()) {
        onColorChangedWhileSpawned();
    }
}

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

        onTextureChangedWhileSpawned();
    }
}

void RectUiNode::setPadding(float padding) {
    this->padding = std::clamp(padding, 0.0F, 0.5F); // NOLINT

    updateChildNodePosAndSize();
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
    getWorldWhileSpawned()->getUiNodeManager().onNodeSpawning(this);
}

void RectUiNode::onDespawning() {
    UiNode::onDespawning();

    // Notify manager.
    getWorldWhileSpawned()->getUiNodeManager().onNodeDespawning(this);

    pTexture = nullptr;
}

void RectUiNode::onVisibilityChanged() {
    UiNode::onVisibilityChanged();

    // Notify manager.
    if (isSpawned()) {
        getWorldWhileSpawned()->getUiNodeManager().onSpawnedNodeChangedVisibility(this);
    }
}

void RectUiNode::onAfterNewDirectChildAttached(Node* pNewDirectChild) {
    UiNode::onAfterNewDirectChildAttached(pNewDirectChild);

    updateChildNodePosAndSize();
}

void RectUiNode::onAfterSizeChanged() {
    UiNode::onAfterSizeChanged();

    updateChildNodePosAndSize();
}

void RectUiNode::onAfterPositionChanged() {
    UiNode::onAfterPositionChanged();

    updateChildNodePosAndSize();
}

void RectUiNode::updateChildNodePosAndSize() {
    const auto mtxChildNodes = getChildNodes();
    std::scoped_lock guard(*mtxChildNodes.first);

    if (mtxChildNodes.second.empty()) {
        return;
    }

    if (mtxChildNodes.second.size() >= 2) [[unlikely]] {
        // For simplicity of the UI system.
        Error::showErrorAndThrowException(
            std::format("rect ui nodes can only have 1 child node (rect node \"{}\")", getNodeName()));
    }

    const auto pUiChild = dynamic_cast<UiNode*>(mtxChildNodes.second[0]);
    if (pUiChild == nullptr) [[unlikely]] {
        Error::showErrorAndThrowException("expected a UI node");
    }

    const auto size = getSize();
    const auto paddingRealSize = std::min(size.x, size.y) * padding;

    pUiChild->setPosition(getPosition() + paddingRealSize);
    pUiChild->setSize(size - paddingRealSize * 2.0F);

    if (!isVisible() && pUiChild->isVisible()) {
        pUiChild->setIsVisible(false);
    }
}
