#include "game/node/ui/InputConfigurationUiNode.h"

// Custom.
#include "game/node/ui/LayoutUiNode.h"
#include "game/node/ui/TextUiNode.h"
#include "game/node/ui/ButtonUiNode.h"
#include "game/GameInstance.h"
#include "game/World.h"
#include "input/GamepadButton.hpp"
#include "render/UiNodeManager.h"
#include "input/InputManager.h"

// External.
#include "nameof.hpp"
#include "utf/utf.hpp"

namespace {
    constexpr std::string_view sTypeGuid = "c8b1432c-4792-4048-acdd-41d408c40fe2";
}

/// @cond UNDOCUMENTED
// Small button type that we use.
class InputConfigurationButtonUiNode : public ButtonUiNode {
public:
    InputConfigurationButtonUiNode() : ButtonUiNode() {};
    ~InputConfigurationButtonUiNode() override = default;

    bool bIsCapturingInput = false;
    bool bIsButtonForKeyboardAndMouse = true;

    std::variant<KeyboardButton, MouseButton, GamepadButton> shownButton;

    std::function<void(MouseButton)> onMouseButtonCaptured;
    std::function<void(KeyboardButton)> onKeyboardButtonCaptured;
    std::function<void(GamepadButton)> onGamepadButtonCaptured;

protected:
    virtual bool onMouseButtonReleasedOnUiNode(MouseButton button, KeyboardModifiers modifiers) override {
        if (bIsCapturingInput) {
            onMouseButtonCaptured(button);
        }
        ButtonUiNode::onMouseButtonReleasedOnUiNode(button, modifiers);
        return true;
    }

    virtual void
    onKeyboardButtonReleasedWhileFocused(KeyboardButton button, KeyboardModifiers modifiers) override {
        if (bIsCapturingInput) {
            onKeyboardButtonCaptured(button);
        }
        ButtonUiNode::onKeyboardButtonReleasedWhileFocused(button, modifiers);
    }

    virtual void onGamepadButtonPressedWhileFocused(GamepadButton button) override {
        // Note: intensionally not calling ButtonUiNode overload to avoid mixup between UI navigation
        // and button capturing.
        RectUiNode::onGamepadButtonReleasedWhileFocused(button);

        if (bIsButtonForKeyboardAndMouse) {
            return;
        }

        onMouseButtonPressedOnUiNode(MouseButton::LEFT, KeyboardModifiers(0));
    }

    virtual void onGamepadButtonReleasedWhileFocused(GamepadButton button) override {
        // Note: intensionally not calling ButtonUiNode overload to avoid mixup between UI navigation
        // and button capturing.

        if (!bIsCapturingInput) {
            if (button == GamepadButton::DPAD_LEFT) {
                getWorldWhileSpawned()->getUiNodeManager().makeNextFocusedNode(this, false, false);
            } else if (button == GamepadButton::DPAD_RIGHT) {
                getWorldWhileSpawned()->getUiNodeManager().makeNextFocusedNode(this, false, true);
            } else if (button == GamepadButton::DPAD_UP) {
                getWorldWhileSpawned()->getUiNodeManager().makeNextFocusedNode(this, true, true);
            } else if (button == GamepadButton::DPAD_DOWN) {
                getWorldWhileSpawned()->getUiNodeManager().makeNextFocusedNode(this, true, false);
            }

            if (bIsButtonForKeyboardAndMouse) {
                return;
            }

            if (button == GamepadButton::BUTTON_LEFT || button == GamepadButton::BUTTON_RIGHT ||
                button == GamepadButton::BUTTON_UP || button == GamepadButton::BUTTON_DOWN) {
                ButtonUiNode::onMouseButtonReleasedOnUiNode(MouseButton::LEFT, KeyboardModifiers(0));
            }

            return;
        }

        if (bIsButtonForKeyboardAndMouse) {
            return;
        }

        ButtonUiNode::onMouseButtonReleasedOnUiNode(MouseButton::LEFT, KeyboardModifiers(0));
        onGamepadButtonCaptured(button);
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
    setChildNodeSpacing(0.05f);
    setPadding(0.025f);
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

    for (auto& ch : sButtonName) {
        ch = static_cast<char>(std::toupper(ch));
    }

    return sButtonName;
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

    // Check if we need the third column (for gamepad buttons).
    auto& inputManager = getGameInstanceWhileSpawned()->getInputManager();
    bool bHaveGamepadButtons = false;
    for (const auto& [iActionEventId, sDisplayedName] : actionEventIdsToName) {
        const auto vButtons = inputManager.getActionEventButtons(iActionEventId);
        if (vButtons.empty()) {
            continue;
        }

        for (const auto& button : vButtons) {
            if (std::holds_alternative<GamepadButton>(button)) {
                bHaveGamepadButtons = true;
                break;
            }
        }
        if (bHaveGamepadButtons) {
            break;
        }
    }

    bool bFocusedFirstButton = false;

    for (const auto& [iActionEventId, sDisplayedName] : actionEventIdsToName) {
        const auto vButtons = inputManager.getActionEventButtons(iActionEventId);
        if (vButtons.empty()) {
            continue;
        }

        std::vector<std::variant<KeyboardButton, MouseButton, GamepadButton>> vKeyboardMouseButtons;
        std::vector<std::variant<KeyboardButton, MouseButton, GamepadButton>> vGamepadButtons;
        for (const auto& button : vButtons) {
            if (std::holds_alternative<KeyboardButton>(button)) {
                vKeyboardMouseButtons.push_back(std::get<KeyboardButton>(button));
            } else if (std::holds_alternative<MouseButton>(button)) {
                vKeyboardMouseButtons.push_back(std::get<MouseButton>(button));
            } else if (std::holds_alternative<GamepadButton>(button)) {
                vGamepadButtons.push_back(std::get<GamepadButton>(button));
            } else [[unlikely]] {
                Error::showErrorAndThrowException("unhandled case");
            }
        }

        const auto pHorizontalLayout = addChildNode(std::make_unique<LayoutUiNode>());
        pHorizontalLayout->setIsHorizontal(true);
        pHorizontalLayout->setSize(glm::vec2(pHorizontalLayout->getSize().x, textHeight * 1.5f));
        pHorizontalLayout->setChildNodeExpandRule(ChildNodeExpandRule::EXPAND_ALONG_BOTH_AXIS);
        pHorizontalLayout->setPadding(0.025f);
        pHorizontalLayout->setChildNodeSpacing(0.1f);
        {
            const auto pEventNameText = pHorizontalLayout->addChildNode(std::make_unique<TextUiNode>());
            pEventNameText->setTextHeight(textHeight);
            pEventNameText->setText(utf::as_u16(sDisplayedName));

            auto pDeviceButtonsLayout = pHorizontalLayout->addChildNode(std::make_unique<LayoutUiNode>());
            pDeviceButtonsLayout->setIsHorizontal(true);
            pDeviceButtonsLayout->setChildNodeSpacing(0.1f);
            pDeviceButtonsLayout->setChildNodeExpandRule(ChildNodeExpandRule::EXPAND_ALONG_BOTH_AXIS);
            const auto drawButtons =
                [&](bool bIsKeyboardAndMouse,
                    const std::vector<std::variant<KeyboardButton, MouseButton, GamepadButton>>&
                        vButtonsToDisplay) {
                    for (const auto& button : vButtonsToDisplay) {
                        const std::string sButtonName = getButtonName(button);

                        const auto updateInput =
                            [this](
                                InputConfigurationButtonUiNode* pUiButton,
                                TextUiNode* pButtonNameText,
                                unsigned int iActionEventId,
                                std::variant<KeyboardButton, MouseButton, GamepadButton> newButton) {
                                pUiButton->bIsCapturingInput = false;
                                auto& inputManager = getGameInstanceWhileSpawned()->getInputManager();
                                auto optError = inputManager.modifyActionEvent(
                                    iActionEventId, pUiButton->shownButton, newButton);
                                if (optError.has_value()) [[unlikely]] {
                                    Log::warn(optError->getInitialMessage());
                                    return;
                                }
                                pUiButton->shownButton = newButton;
                                pButtonNameText->setText(utf::as_u16(getButtonName(newButton)));
                                if (onInputChanged != nullptr) {
                                    onInputChanged();
                                }
                            };

                        const auto pUiButton = pDeviceButtonsLayout->addChildNode(
                            std::make_unique<InputConfigurationButtonUiNode>());
                        if (!bFocusedFirstButton) {
                            bFocusedFirstButton = true;
                            pUiButton->setFocused();
                        }
                        pUiButton->setPadding(0.05f);
                        {
                            const auto pButtonNameText =
                                pUiButton->addChildNode(std::make_unique<TextUiNode>());
                            pButtonNameText->setTextHeight(textHeight * 0.9f);
                            pButtonNameText->setText(utf::as_u16(sButtonName));

                            pUiButton->setOnClicked([pUiButton, pButtonNameText]() {
                                if (pUiButton->bIsCapturingInput) {
                                    return;
                                }
                                pUiButton->bIsCapturingInput = true;
                                pUiButton->setFocused();
                                pButtonNameText->setText(u"...");
                            });

                            pUiButton->bIsButtonForKeyboardAndMouse = bIsKeyboardAndMouse;
                            pUiButton->shownButton = button;

                            pUiButton->onKeyboardButtonCaptured =
                                [updateInput, iActionEventId, sButtonName, pUiButton, pButtonNameText](
                                    KeyboardButton newButton) {
                                    if (newButton == KeyboardButton::ESCAPE) {
                                        pUiButton->bIsCapturingInput = false;
                                        pButtonNameText->setText(utf::as_u16(sButtonName));
                                        return;
                                    }
                                    updateInput(pUiButton, pButtonNameText, iActionEventId, newButton);
                                };
                            pUiButton->onMouseButtonCaptured =
                                [updateInput, iActionEventId, sButtonName, pUiButton, pButtonNameText](
                                    MouseButton newButton) {
                                    updateInput(pUiButton, pButtonNameText, iActionEventId, newButton);
                                };
                            pUiButton->onGamepadButtonCaptured =
                                [updateInput, iActionEventId, sButtonName, pUiButton, pButtonNameText](
                                    GamepadButton newButton) {
                                    updateInput(pUiButton, pButtonNameText, iActionEventId, newButton);
                                };
                        }
                    }
                };

            drawButtons(true, vKeyboardMouseButtons);

            pDeviceButtonsLayout = pHorizontalLayout->addChildNode(std::make_unique<LayoutUiNode>());
            pDeviceButtonsLayout->setIsHorizontal(true);
            pDeviceButtonsLayout->setChildNodeSpacing(0.1f);
            pDeviceButtonsLayout->setChildNodeExpandRule(ChildNodeExpandRule::EXPAND_ALONG_BOTH_AXIS);
            drawButtons(false, vGamepadButtons);
        }
    }
}

void InputConfigurationUiNode::onSpawning() {
    LayoutUiNode::onSpawning();

    refreshDisplayedEvents();
}
