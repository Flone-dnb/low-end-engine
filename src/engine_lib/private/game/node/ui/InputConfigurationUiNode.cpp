#include "game/node/ui/InputConfigurationUiNode.h"

// Custom.
#include "game/node/ui/LayoutUiNode.h"
#include "game/node/ui/TextUiNode.h"
#include "game/node/ui/ButtonUiNode.h"
#include "game/GameInstance.h"
#include "game/World.h"
#include "render/UiNodeManager.h"
#include "input/InputManager.h"

// External.
#include "nameof.hpp"
#include "utf/utf.hpp"

namespace {
    constexpr unsigned int iNameColumnWidthPortion = 6;
    constexpr unsigned int iValueColumnWidthPortion = 4;
    constexpr std::string_view sTypeGuid = "c8b1432c-4792-4048-acdd-41d408c40fe2";
}

/// @cond UNDOCUMENTED
// Small button type that we use.
class InputConfigurationButtonUiNode : public ButtonUiNode {
public:
    InputConfigurationButtonUiNode() : ButtonUiNode() {};
    ~InputConfigurationButtonUiNode() override = default;

    bool bIsCapturingInput = false;

    std::function<void(MouseButton)> onMouseButtonPressed;
    std::function<void(KeyboardButton)> onKeyboardButtonPressed;
    std::function<void(GamepadButton)> onGamepadButtonPressed;

protected:
    virtual bool onMouseButtonReleasedOnUiNode(MouseButton button, KeyboardModifiers modifiers) override {
        if (bIsCapturingInput && onMouseButtonPressed) {
            onMouseButtonPressed(button);
        }
        ButtonUiNode::onMouseButtonReleasedOnUiNode(button, modifiers);
        return true;
    }

    virtual void
    onKeyboardButtonReleasedWhileFocused(KeyboardButton button, KeyboardModifiers modifiers) override {
        if (bIsCapturingInput && onKeyboardButtonPressed) {
            onKeyboardButtonPressed(button);
        }
        ButtonUiNode::onKeyboardButtonReleasedWhileFocused(button, modifiers);
    }

    virtual void onGamepadButtonReleasedWhileFocused(GamepadButton button) override {
        if (bIsCapturingInput && onGamepadButtonPressed) {
            onGamepadButtonPressed(button);
        }
        ButtonUiNode::onGamepadButtonReleasedWhileFocused(button);
    }
};
/// @endcond

std::string InputConfigurationUiNode::getTypeGuidStatic() { return sTypeGuid.data(); }
std::string InputConfigurationUiNode::getTypeGuid() const { return sTypeGuid.data(); }

void InputConfigurationUiNode::setOnInputChanged(const std::function<void()>& onChanged) {
    onInputChanged = onChanged;
}

TypeReflectionInfo InputConfigurationUiNode::getReflectionInfo() {
    ReflectedVariables variables;

    variables.floats[NAMEOF_MEMBER(&InputConfigurationUiNode::textHeight).data()] =
        ReflectedVariableInfo<float>{
            .setter =
                [](Serializable* pThis, const float& newValue) {
                    reinterpret_cast<InputConfigurationUiNode*>(pThis)->setTextHeight(newValue);
                },
            .getter = [](Serializable* pThis) -> float {
                return reinterpret_cast<InputConfigurationUiNode*>(pThis)->getTextHeight();
            }};

    variables.bools[NAMEOF_MEMBER(&InputConfigurationUiNode::bIsManagingKeyboardInput).data()] =
        ReflectedVariableInfo<bool>{
            .setter =
                [](Serializable* pThis, const bool& bNewValue) {
                    reinterpret_cast<InputConfigurationUiNode*>(pThis)->setIsManagingKeyboardInput(bNewValue);
                },
            .getter = [](Serializable* pThis) -> bool {
                return reinterpret_cast<InputConfigurationUiNode*>(pThis)->isManagingKeyboardInput();
            }};

    return TypeReflectionInfo(
        LayoutUiNode::getTypeGuidStatic(),
        NAMEOF_SHORT_TYPE(InputConfigurationUiNode).data(),
        []() -> std::unique_ptr<Serializable> { return std::make_unique<InputConfigurationUiNode>(); },
        std::move(variables));
}

InputConfigurationUiNode::InputConfigurationUiNode()
    : InputConfigurationUiNode("Input Configuration UI Node") {}
InputConfigurationUiNode::InputConfigurationUiNode(const std::string& sNodeName) : LayoutUiNode(sNodeName) {
    setIsScrollBarEnabled(true);
    setChildNodeSpacing(0.05F);
    setPadding(0.025F);
    setChildNodeExpandRule(ChildNodeExpandRule::EXPAND_ALONG_SECONDARY_AXIS);
}

void InputConfigurationUiNode::setActionEvents(
    const std::unordered_map<unsigned int, std::string>& actionEvents) {
    actionEventIdsToName = actionEvents;
    refreshDisplayedEvents();
}

std::string
InputConfigurationUiNode::getButtonName(std::variant<KeyboardButton, MouseButton, GamepadButton> button) {
    std::string sButtonName;
    if (std::holds_alternative<KeyboardButton>(button)) {
        sButtonName = getKeyboardButtonName(std::get<KeyboardButton>(button));
    } else if (std::holds_alternative<MouseButton>(button)) {
        sButtonName = getMouseButtonName(std::get<MouseButton>(button));
    } else if (std::holds_alternative<GamepadButton>(button)) {
        sButtonName = getGamepadButtonName(std::get<GamepadButton>(button));
    } else [[unlikely]] {
        Error::showErrorAndThrowException("unhandled case");
    }

    return sButtonName;
}

void InputConfigurationUiNode::setIsManagingKeyboardInput(bool bIsKeyboard) {
    bIsManagingKeyboardInput = bIsKeyboard;
    refreshDisplayedEvents();
}

void InputConfigurationUiNode::refreshDisplayedEvents() {
    if (!isSpawned()) {
        return;
    }

    {
        auto mtxChildNodes = getChildNodes();
        std::scoped_lock guard(*mtxChildNodes.first);
        for (const auto& pChildNode : mtxChildNodes.second) {
            pChildNode->unsafeDetachFromParentAndDespawn(true);
        }
    }

    auto& inputManager = getGameInstanceWhileSpawned()->getInputManager();
    for (const auto& [iActionEventId, sDisplayedName] : actionEventIdsToName) {
        const auto vButtons = inputManager.getActionEventButtons(iActionEventId);
        if (vButtons.empty()) {
            continue;
        }

        std::vector<std::variant<KeyboardButton, MouseButton, GamepadButton>> vFilteredButtons;
        for (const auto& button : vButtons) {
            if ((std::holds_alternative<KeyboardButton>(button) ||
                 std::holds_alternative<MouseButton>(button)) &&
                !bIsManagingKeyboardInput) {
                continue;
            }
            if (std::holds_alternative<GamepadButton>(button) && bIsManagingKeyboardInput) {
                continue;
            }

            vFilteredButtons.push_back(button);
        }
        if (vFilteredButtons.empty()) {
            continue;
        }

        const auto pHorizontalLayout = addChildNode(std::make_unique<LayoutUiNode>());
        pHorizontalLayout->setIsHorizontal(true);
        pHorizontalLayout->setSize(glm::vec2(pHorizontalLayout->getSize().x, textHeight * 1.5F));
        pHorizontalLayout->setChildNodeExpandRule(ChildNodeExpandRule::EXPAND_ALONG_BOTH_AXIS);
        pHorizontalLayout->setPadding(0.025F);
        {
            const auto pEventNameText = pHorizontalLayout->addChildNode(std::make_unique<TextUiNode>());
            pEventNameText->setTextHeight(textHeight);
            pEventNameText->setExpandPortionInLayout(iNameColumnWidthPortion);
            pEventNameText->setText(utf::as_u16(sDisplayedName));

            const auto pButtonsLayout = pHorizontalLayout->addChildNode(std::make_unique<LayoutUiNode>());
            pButtonsLayout->setIsHorizontal(true);
            pButtonsLayout->setExpandPortionInLayout(iValueColumnWidthPortion);
            pButtonsLayout->setChildNodeSpacing(0.1F);
            pButtonsLayout->setChildNodeExpandRule(ChildNodeExpandRule::EXPAND_ALONG_BOTH_AXIS);
            {
                for (const auto& button : vFilteredButtons) {
                    const std::string sButtonName = getButtonName(button);

#define HANDLE_INPUT                                                                                         \
    pButtonNameText->setText(utf::as_u16(sButtonName));                                                      \
    pUiButton->bIsCapturingInput = false;                                                                    \
    auto& inputManager = getGameInstanceWhileSpawned()->getInputManager();                                   \
    auto optError = inputManager.modifyActionEvent(iActionEventId, oldButton, newButton);                    \
    if (optError.has_value()) [[unlikely]] {                                                                 \
        Log::warn(optError->getInitialMessage());                                                            \
        return;                                                                                              \
    }                                                                                                        \
    pButtonNameText->setText(utf::as_u16(getButtonName(newButton)));                                         \
    if (onInputChanged != nullptr) {                                                                         \
        onInputChanged();                                                                                    \
    }

                    const auto pUiButton =
                        pButtonsLayout->addChildNode(std::make_unique<InputConfigurationButtonUiNode>());
                    pUiButton->setPadding(0.05F);
                    {
                        const auto pButtonNameText = pUiButton->addChildNode(std::make_unique<TextUiNode>());
                        pButtonNameText->setTextHeight(textHeight * 0.9F);
                        pButtonNameText->setText(utf::as_u16(sButtonName));

                        pUiButton->setOnClicked([pUiButton, pButtonNameText]() {
                            if (pUiButton->bIsCapturingInput) {
                                return;
                            }
                            pUiButton->bIsCapturingInput = true;
                            pUiButton->setFocused();
                            pButtonNameText->setText(u"...");
                        });

                        if (bIsManagingKeyboardInput) {
                            pUiButton->onKeyboardButtonPressed = [this,
                                                                  iActionEventId,
                                                                  oldButton = button,
                                                                  sButtonName,
                                                                  pUiButton,
                                                                  pButtonNameText](KeyboardButton newButton) {
                                if (newButton == KeyboardButton::ESCAPE) {
                                    return;
                                }
                                HANDLE_INPUT;
                            };
                            pUiButton->onMouseButtonPressed = [this,
                                                               iActionEventId,
                                                               oldButton = button,
                                                               sButtonName,
                                                               pUiButton,
                                                               pButtonNameText](MouseButton newButton) {
                                HANDLE_INPUT;
                            };
                        } else {
                            pUiButton->onGamepadButtonPressed = [this,
                                                                 iActionEventId,
                                                                 oldButton = button,
                                                                 sButtonName,
                                                                 pUiButton,
                                                                 pButtonNameText](GamepadButton newButton) {
                                HANDLE_INPUT;
                            };
                        }
                    }
                }
            }
        }
    }
}

void InputConfigurationUiNode::onSpawning() {
    LayoutUiNode::onSpawning();

    refreshDisplayedEvents();
}
