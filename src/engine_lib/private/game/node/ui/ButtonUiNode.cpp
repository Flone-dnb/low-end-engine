#include "game/node/ui/ButtonUiNode.h"

// Custom.
#include "game/GameInstance.h"
#include "render/Renderer.h"
#include "material/TextureManager.h"

// External.
#include "nameof.hpp"

namespace {
    constexpr std::string_view sTypeGuid = "2e907e00-d8fe-4c02-a3dd-2479d3cf9d2e";
}

std::string ButtonUiNode::getTypeGuidStatic() { return sTypeGuid.data(); }
std::string ButtonUiNode::getTypeGuid() const { return sTypeGuid.data(); }

TypeReflectionInfo ButtonUiNode::getReflectionInfo() {
    ReflectedVariables variables;

    variables.vec4s[NAMEOF_MEMBER(&ButtonUiNode::colorWhileHovered).data()] =
        ReflectedVariableInfo<glm::vec4>{
            .setter =
                [](Serializable* pThis, const glm::vec4& newValue) {
                    reinterpret_cast<ButtonUiNode*>(pThis)->setColorWhileHovered(newValue);
                },
            .getter = [](Serializable* pThis) -> glm::vec4 {
                return reinterpret_cast<ButtonUiNode*>(pThis)->getColorWhileHovered();
            }};

    variables.vec4s[NAMEOF_MEMBER(&ButtonUiNode::colorWhilePressed).data()] =
        ReflectedVariableInfo<glm::vec4>{
            .setter =
                [](Serializable* pThis, const glm::vec4& newValue) {
                    reinterpret_cast<ButtonUiNode*>(pThis)->setColorWhilePressed(newValue);
                },
            .getter = [](Serializable* pThis) -> glm::vec4 {
                return reinterpret_cast<ButtonUiNode*>(pThis)->getColorWhilePressed();
            }};

    variables.strings[NAMEOF_MEMBER(&ButtonUiNode::sPathToTextureWhileHovered).data()] =
        ReflectedVariableInfo<std::string>{
            .setter =
                [](Serializable* pThis, const std::string& sNewValue) {
                    reinterpret_cast<ButtonUiNode*>(pThis)->setPathToTextureWhileHovered(sNewValue);
                },
            .getter = [](Serializable* pThis) -> std::string {
                return reinterpret_cast<ButtonUiNode*>(pThis)->getPathToTextureWhileHovered();
            }};

    variables.strings[NAMEOF_MEMBER(&ButtonUiNode::sPathToTextureWhilePressed).data()] =
        ReflectedVariableInfo<std::string>{
            .setter =
                [](Serializable* pThis, const std::string& sNewValue) {
                    reinterpret_cast<ButtonUiNode*>(pThis)->setPathToTextureWhilePressed(sNewValue);
                },
            .getter = [](Serializable* pThis) -> std::string {
                return reinterpret_cast<ButtonUiNode*>(pThis)->getPathToTextureWhilePressed();
            }};

    return TypeReflectionInfo(
        RectUiNode::getTypeGuidStatic(),
        NAMEOF_SHORT_TYPE(ButtonUiNode).data(),
        []() -> std::unique_ptr<Serializable> { return std::make_unique<ButtonUiNode>(); },
        std::move(variables));
}

ButtonUiNode::ButtonUiNode() : ButtonUiNode("Button UI Node") {}
ButtonUiNode::ButtonUiNode(const std::string& sNodeName) : RectUiNode(sNodeName) {
    setSize(glm::vec2(0.15F, 0.075F)); // NOLINT
    setIsReceivingInput(true);
}

void ButtonUiNode::setColorWhileHovered(const glm::vec4& color) {
    colorWhileHovered = glm::clamp(color, 0.0F, 1.0F);
}

void ButtonUiNode::setColorWhilePressed(const glm::vec4& color) {
    colorWhilePressed = glm::clamp(color, 0.0F, 1.0F);
}

void ButtonUiNode::setPathToTextureWhileHovered(std::string sPathToTextureRelativeRes) {
    // Normalize slash.
    for (size_t i = 0; i < sPathToTextureRelativeRes.size(); i++) {
        if (sPathToTextureRelativeRes[i] == '\\') {
            sPathToTextureRelativeRes[i] = '/';
        }
    }

    if (sPathToTextureWhileHovered == sPathToTextureRelativeRes) {
        return;
    }
    sPathToTextureWhileHovered = sPathToTextureRelativeRes;

    if (isSpawned()) {
        if (sPathToTextureWhileHovered.empty()) {
            pHoveredTexture = nullptr;
        } else {
            pHoveredTexture = getTextureHandle(sPathToTextureWhileHovered);
        }
    }
}

void ButtonUiNode::setPathToTextureWhilePressed(std::string sPathToTextureRelativeRes) {
    // Normalize slash.
    for (size_t i = 0; i < sPathToTextureRelativeRes.size(); i++) {
        if (sPathToTextureRelativeRes[i] == '\\') {
            sPathToTextureRelativeRes[i] = '/';
        }
    }

    if (sPathToTextureWhilePressed == sPathToTextureRelativeRes) {
        return;
    }
    sPathToTextureWhilePressed = sPathToTextureRelativeRes;

    if (isSpawned()) {
        if (sPathToTextureWhilePressed.empty()) {
            pPressedTexture = nullptr;
        } else {
            pPressedTexture = getTextureHandle(sPathToTextureWhilePressed);
        }
    }
}

void ButtonUiNode::setOnClicked(const std::function<void()>& onClicked) { this->onClicked = onClicked; }

void ButtonUiNode::onSpawning() {
    RectUiNode::onSpawning();

    tempDefaultColor = getColor();
    sTempPathToDefaultTexture = getPathToTexture();

    if (!sTempPathToDefaultTexture.empty()) {
        pDefaultTexture = getTextureHandle(sTempPathToDefaultTexture);
    }
    if (!sPathToTextureWhileHovered.empty()) {
        pHoveredTexture = getTextureHandle(sPathToTextureWhileHovered);
    }
    if (!sPathToTextureWhilePressed.empty()) {
        pPressedTexture = getTextureHandle(sPathToTextureWhilePressed);
    }
}

void ButtonUiNode::onDespawning() {
    RectUiNode::onDespawning();

    pDefaultTexture = nullptr;
    pHoveredTexture = nullptr;
    pPressedTexture = nullptr;
}

bool ButtonUiNode::onMouseClickOnUiNode(
    MouseButton button, KeyboardModifiers modifiers, bool bIsPressedDown) {
    RectUiNode::onMouseClickOnUiNode(button, modifiers, bIsPressedDown);

    if (bIsPressedDown) {
        setButtonTexture(sPathToTextureWhilePressed);
        setButtonColor(colorWhilePressed);
    } else {
        setButtonTexture(bIsCurrentlyHovered ? sPathToTextureWhileHovered : sTempPathToDefaultTexture);
        setButtonColor(bIsCurrentlyHovered ? colorWhileHovered : tempDefaultColor);
        if (onClicked) {
            onClicked();
        }
    }

    return true;
}

void ButtonUiNode::onMouseEntered() {
    RectUiNode::onMouseEntered();

    bIsCurrentlyHovered = true;

    setButtonTexture(sPathToTextureWhileHovered);
    setButtonColor(colorWhileHovered);
}

void ButtonUiNode::onMouseLeft() {
    RectUiNode::onMouseLeft();

    bIsCurrentlyHovered = false;

    setButtonTexture(sTempPathToDefaultTexture);
    setButtonColor(tempDefaultColor);
}

void ButtonUiNode::onColorChangedWhileSpawned() {
    RectUiNode::onColorChangedWhileSpawned();

    if (bIsChangingColorTexture) {
        return;
    }

    tempDefaultColor = getColor();
}

void ButtonUiNode::onTextureChangedWhileSpawned() {
    RectUiNode::onTextureChangedWhileSpawned();

    if (bIsChangingColorTexture) {
        return;
    }

    sTempPathToDefaultTexture = getPathToTexture();

    if (isSpawned()) {
        if (sTempPathToDefaultTexture.empty()) {
            pDefaultTexture = nullptr;
        } else {
            pDefaultTexture = getTextureHandle(sTempPathToDefaultTexture);
        }
    }
}

void ButtonUiNode::setButtonColor(const glm::vec4& color) {
    bIsChangingColorTexture = true;
    setColor(color);
    bIsChangingColorTexture = false;
}

void ButtonUiNode::setButtonTexture(const std::string& sPathToTexture) {
    bIsChangingColorTexture = true;
    setPathToTexture(sPathToTexture);
    bIsChangingColorTexture = false;
}

std::unique_ptr<TextureHandle> ButtonUiNode::getTextureHandle(const std::string& sPathToTexture) {
    auto result = getGameInstanceWhileSpawned()->getRenderer()->getTextureManager().getTexture(
        sPathToTexture, TextureUsage::UI);
    if (std::holds_alternative<Error>(result)) [[unlikely]] {
        auto error = std::get<Error>(std::move(result));
        error.addCurrentLocationToErrorStack();
        error.showErrorAndThrowException();
    }

    return std::get<std::unique_ptr<TextureHandle>>(std::move(result));
}
