#include "game/node/ui/SliderUiNode.h"

// Standard.
#include <cmath>

// Custom.
#include "game/GameInstance.h"
#include "render/UiNodeManager.h"
#include "game/World.h"
#include "game/camera/CameraManager.h"

// External.
#include "nameof.hpp"

namespace {
    constexpr std::string_view sTypeGuid = "63c8413c-2dab-47e3-9539-ffecaa5e72e4";
}

std::string SliderUiNode::getTypeGuidStatic() { return sTypeGuid.data(); }
std::string SliderUiNode::getTypeGuid() const { return sTypeGuid.data(); }

SliderUiNode::SliderUiNode() : SliderUiNode("Slider UI Node") {}
SliderUiNode::SliderUiNode(const std::string& sNodeName) : UiNode(sNodeName) {
    setIsReceivingInput(true);

    setSize(glm::vec2(0.1F, 0.04F)); // NOLINT: sliders are generally small.
}

TypeReflectionInfo SliderUiNode::getReflectionInfo() {
    ReflectedVariables variables;

    variables.vec4s[NAMEOF_MEMBER(&SliderUiNode::sliderColor).data()] = ReflectedVariableInfo<glm::vec4>{
        .setter =
            [](Serializable* pThis, const glm::vec4& newValue) {
                reinterpret_cast<SliderUiNode*>(pThis)->setSliderColor(newValue);
            },
        .getter = [](Serializable* pThis) -> glm::vec4 {
            return reinterpret_cast<SliderUiNode*>(pThis)->getSliderColor();
        }};

    variables.vec4s[NAMEOF_MEMBER(&SliderUiNode::sliderHandleColor).data()] =
        ReflectedVariableInfo<glm::vec4>{
            .setter =
                [](Serializable* pThis, const glm::vec4& newValue) {
                    reinterpret_cast<SliderUiNode*>(pThis)->setSliderHandleColor(newValue);
                },
            .getter = [](Serializable* pThis) -> glm::vec4 {
                return reinterpret_cast<SliderUiNode*>(pThis)->getSliderHandleColor();
            }};

    variables.floats[NAMEOF_MEMBER(&SliderUiNode::handlePosition).data()] = ReflectedVariableInfo<float>{
        .setter =
            [](Serializable* pThis, const float& newValue) {
                reinterpret_cast<SliderUiNode*>(pThis)->setHandlePosition(newValue);
            },
        .getter = [](Serializable* pThis) -> float {
            return reinterpret_cast<SliderUiNode*>(pThis)->getHandlePosition();
        }};

    variables.floats[NAMEOF_MEMBER(&SliderUiNode::sliderStep).data()] = ReflectedVariableInfo<float>{
        .setter =
            [](Serializable* pThis, const float& newValue) {
                reinterpret_cast<SliderUiNode*>(pThis)->setSliderStep(newValue);
            },
        .getter = [](Serializable* pThis) -> float {
            return reinterpret_cast<SliderUiNode*>(pThis)->getSliderStep();
        }};

    return TypeReflectionInfo(
        UiNode::getTypeGuidStatic(),
        NAMEOF_SHORT_TYPE(SliderUiNode).data(),
        []() -> std::unique_ptr<Serializable> { return std::make_unique<SliderUiNode>(); },
        std::move(variables));
}

void SliderUiNode::setSliderColor(const glm::vec4& color) { sliderColor = color; }

void SliderUiNode::setSliderHandleColor(const glm::vec4& color) { sliderHandleColor = color; }

void SliderUiNode::setHandlePosition(float position, bool bTriggerOnChangedCallback) {
    handlePosition = std::clamp(position, 0.0F, 1.0F);
    if (bTriggerOnChangedCallback && onHandlePositionChanged) {
        onHandlePositionChanged(handlePosition);
    }
}

void SliderUiNode::setOnHandlePositionChanged(const std::function<void(float)>& onChanged) {
    onHandlePositionChanged = onChanged;
}

void SliderUiNode::onSpawning() {
    UiNode::onSpawning();

    // Notify manager.
    getWorldWhileSpawned()->getUiNodeManager().onNodeSpawning(this);
}

void SliderUiNode::onDespawning() {
    UiNode::onDespawning();

    // Notify manager.
    getWorldWhileSpawned()->getUiNodeManager().onNodeDespawning(this);
}

void SliderUiNode::onVisibilityChanged() {
    UiNode::onVisibilityChanged();

    // Notify manager.
    getWorldWhileSpawned()->getUiNodeManager().onSpawnedNodeChangedVisibility(this);
}

float SliderUiNode::snapToNearest(float value, float step) { return std::round(value / step) * step; }

void SliderUiNode::setSliderStep(float stepSize) {
    sliderStep = stepSize;

    if (sliderStep > 0.0F) {
        handlePosition = snapToNearest(handlePosition, sliderStep);
    }
    handlePosition = std::clamp(handlePosition, 0.0F, 1.0F); // just in case
    if (onHandlePositionChanged) {
        onHandlePositionChanged(handlePosition);
    }
}

bool SliderUiNode::onMouseButtonPressedOnUiNode(MouseButton button, KeyboardModifiers modifiers) {
    UiNode::onMouseButtonPressedOnUiNode(button, modifiers);

    if (button != MouseButton::LEFT) {
        return true;
    }

    bIsMouseCursorDraggingHandle = true;

    // Move handle according to the cursor.
    const auto optCursorPos = getWorldWhileSpawned()->getCameraManager().getCursorPosOnViewport();
    if (!optCursorPos.has_value()) {
        return true;
    }
    const auto cursorPos = *optCursorPos;

    handlePosition = (cursorPos.x - getPosition().x) / getSize().x;
    if (sliderStep > 0.0F) {
        handlePosition = snapToNearest(handlePosition, sliderStep);
    }
    handlePosition = std::clamp(handlePosition, 0.0F, 1.0F); // just in case
    if (onHandlePositionChanged) {
        onHandlePositionChanged(handlePosition);
    }

    return true;
}

bool SliderUiNode::onMouseButtonReleasedOnUiNode(MouseButton button, KeyboardModifiers modifiers) {
    UiNode::onMouseButtonReleasedOnUiNode(button, modifiers);

    if (button != MouseButton::LEFT) {
        return true;
    }

    bIsMouseCursorDraggingHandle = false;

    return true;
}

void SliderUiNode::onMouseMove(double xOffset, double yOffset) {
    UiNode::onMouseMove(xOffset, yOffset);

    if (!bIsMouseCursorDraggingHandle) {
        return;
    }

    const auto posBefore = handlePosition;

    // Move handle according to the cursor.
    const auto optCursorPos = getWorldWhileSpawned()->getCameraManager().getCursorPosOnViewport();
    if (!optCursorPos.has_value()) {
        return;
    }
    const auto cursorPos = *optCursorPos;

    handlePosition = (cursorPos.x - getPosition().x) / getSize().x;
    if (sliderStep > 0.0F) {
        handlePosition = snapToNearest(handlePosition, sliderStep);
    }
    handlePosition = std::clamp(handlePosition, 0.0F, 1.0F); // just in case
    if (handlePosition != posBefore && onHandlePositionChanged) {
        onHandlePositionChanged(handlePosition);
    }
}

void SliderUiNode::onMouseLeft() {
    UiNode::onMouseLeft();

    bIsMouseCursorDraggingHandle = false;
}

void SliderUiNode::onGamepadButtonPressedWhileFocused(GamepadButton button) {
    UiNode::onGamepadButtonPressedWhileFocused(button);

    float step = std::max(sliderStep, 0.05F);
    if (button == GamepadButton::DPAD_LEFT) {
        step = -step;
    }

    handlePosition = std::clamp(snapToNearest(handlePosition + step, step), 0.0F, 1.0F);
}

void SliderUiNode::onAfterNewDirectChildAttached(Node* pNewDirectChild) {
    UiNode::onAfterNewDirectChildAttached(pNewDirectChild);
    Error::showErrorAndThrowException(
        std::format("slider node \"{}\" can't have child nodes", getNodeName()));
}
