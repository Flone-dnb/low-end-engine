#include "game/node/ui/CheckboxUiNode.h"

// Custom.
#include "game/World.h"
#include "render/UiNodeManager.h"

// External.
#include "nameof.hpp"

namespace {
    constexpr std::string_view sTypeGuid = "b31c01ea-a513-4f30-858f-73f867ad35a4";
}

std::string CheckboxUiNode::getTypeGuidStatic() { return sTypeGuid.data(); }
std::string CheckboxUiNode::getTypeGuid() const { return sTypeGuid.data(); }

CheckboxUiNode::CheckboxUiNode() : CheckboxUiNode("Checkbox UI Node") {}
CheckboxUiNode::CheckboxUiNode(const std::string& sNodeName) : UiNode(sNodeName) {
    setIsReceivingInput(true);
    setSize(glm::vec2(0.04f, 0.04f));
}

TypeReflectionInfo CheckboxUiNode::getReflectionInfo() {
    ReflectedVariables variables;

    variables.vec4s[NAMEOF_MEMBER(&CheckboxUiNode::backgroundColor).data()] =
        ReflectedVariableInfo<glm::vec4>{
            .setter =
                [](Serializable* pThis, const glm::vec4& newValue) {
                    reinterpret_cast<CheckboxUiNode*>(pThis)->setBackgroundColor(newValue);
                },
            .getter = [](Serializable* pThis) -> glm::vec4 {
                return reinterpret_cast<CheckboxUiNode*>(pThis)->getBackgroundColor();
            }};

    variables.vec4s[NAMEOF_MEMBER(&CheckboxUiNode::foregroundColor).data()] =
        ReflectedVariableInfo<glm::vec4>{
            .setter =
                [](Serializable* pThis, const glm::vec4& newValue) {
                    reinterpret_cast<CheckboxUiNode*>(pThis)->setForegroundColor(newValue);
                },
            .getter = [](Serializable* pThis) -> glm::vec4 {
                return reinterpret_cast<CheckboxUiNode*>(pThis)->getForegroundColor();
            }};

    variables.bools[NAMEOF_MEMBER(&CheckboxUiNode::bIsChecked).data()] = ReflectedVariableInfo<bool>{
        .setter =
            [](Serializable* pThis, const bool& bNewValue) {
                reinterpret_cast<CheckboxUiNode*>(pThis)->setIsChecked(bNewValue);
            },
        .getter = [](Serializable* pThis) -> bool {
            return reinterpret_cast<CheckboxUiNode*>(pThis)->isChecked();
        }};

    return TypeReflectionInfo(
        UiNode::getTypeGuidStatic(),
        NAMEOF_SHORT_TYPE(CheckboxUiNode).data(),
        []() -> std::unique_ptr<Serializable> { return std::make_unique<CheckboxUiNode>(); },
        std::move(variables));
}

void CheckboxUiNode::setBackgroundColor(const glm::vec4& color) { backgroundColor = color; }

void CheckboxUiNode::setForegroundColor(const glm::vec4& color) { foregroundColor = color; }

void CheckboxUiNode::setIsChecked(bool bIsChecked, bool bTriggerOnChangedCallback) {
    this->bIsChecked = bIsChecked;

    if (bTriggerOnChangedCallback && onStateChanged) {
        onStateChanged(this->bIsChecked);
    }
}

void CheckboxUiNode::setOnStateChanged(const std::function<void(bool)>& callback) {
    onStateChanged = callback;
}

void CheckboxUiNode::onSpawning() {
    UiNode::onSpawning();

    // Notify manager.
    getWorldWhileSpawned()->getUiNodeManager().onNodeSpawning(this);
}

void CheckboxUiNode::onDespawning() {
    UiNode::onDespawning();

    // Notify manager.
    getWorldWhileSpawned()->getUiNodeManager().onNodeDespawning(this);
}

void CheckboxUiNode::onVisibilityChanged() {
    UiNode::onVisibilityChanged();

    if (isSpawned()) {
        // Notify manager.
        getWorldWhileSpawned()->getUiNodeManager().onSpawnedNodeChangedVisibility(this);
    }
}

bool CheckboxUiNode::onMouseButtonPressedOnUiNode(MouseButton button, KeyboardModifiers modifiers) {
    UiNode::onMouseButtonPressedOnUiNode(button, modifiers);

    if (button != MouseButton::LEFT) {
        return true;
    }

    setIsChecked(!bIsChecked);

    return true;
}

void CheckboxUiNode::onAfterNewDirectChildAttached(Node* pNewDirectChild) {
    UiNode::onAfterNewDirectChildAttached(pNewDirectChild);
    Error::showErrorAndThrowException(
        std::format("checkbox node \"{}\" can't have child nodes", getNodeName()));
}
