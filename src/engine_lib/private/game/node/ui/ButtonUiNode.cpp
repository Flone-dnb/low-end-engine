#include "game/node/ui/ButtonUiNode.h"

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

    return TypeReflectionInfo(
        RectUiNode::getTypeGuidStatic(),
        NAMEOF_SHORT_TYPE(ButtonUiNode).data(),
        []() -> std::unique_ptr<Serializable> { return std::make_unique<ButtonUiNode>(); },
        std::move(variables));
}

ButtonUiNode::ButtonUiNode() : RectUiNode("Button UI Node") {}
ButtonUiNode::ButtonUiNode(const std::string& sNodeName) : RectUiNode(sNodeName) {
    setColor(glm::vec4(1.0F, 0.26F, 0.0F, 1.0F)); // NOLINT
    setSize(glm::vec2(0.15F, 0.075F));            // NOLINT
    setIsReceivingInput(true);
}

void ButtonUiNode::setColorWhileHovered(const glm::vec4& color) { colorWhileHovered = color; }

void ButtonUiNode::setColorWhilePressed(const glm::vec4& color) { colorWhilePressed = color; }

void ButtonUiNode::setOnClicked(const std::function<void()>& onClicked) { this->onClicked = onClicked; }

void ButtonUiNode::onSpawning() {
    RectUiNode::onSpawning();

    tempDefaultColor = getColor();
}

void ButtonUiNode::onMouseClickOnUiNode(
    MouseButton button, KeyboardModifiers modifiers, bool bIsPressedDown) {
    RectUiNode::onMouseClickOnUiNode(button, modifiers, bIsPressedDown);

    if (bIsPressedDown) {
        setButtonColor(colorWhilePressed);
    } else {
        setButtonColor(bIsCurrentlyHovered ? colorWhileHovered : tempDefaultColor);
        if (onClicked) {
            onClicked();
        }
    }
}

void ButtonUiNode::onMouseEntered() {
    RectUiNode::onMouseEntered();

    bIsCurrentlyHovered = true;

    setButtonColor(colorWhileHovered);
}

void ButtonUiNode::onMouseLeft() {
    RectUiNode::onMouseLeft();

    bIsCurrentlyHovered = false;

    setButtonColor(tempDefaultColor);
}

void ButtonUiNode::onColorChangedWhileSpawned() {
    RectUiNode::onColorChangedWhileSpawned();

    if (bIsChangingColor) {
        return;
    }

    tempDefaultColor = getColor();
}

void ButtonUiNode::setButtonColor(const glm::vec4& color) {
    bIsChangingColor = true;
    setColor(color);
    bIsChangingColor = false;
}
