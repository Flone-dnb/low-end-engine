// Custom.
#include "input/InputManager.h"

// External.
#include "catch2/catch_test_macros.hpp"

// Because order of elements in the arrays may differ we use this function.
void compareActionEventButtons(
    const std::vector<std::variant<KeyboardButton, MouseButton, GamepadButton>>& vItem1,
    const std::vector<std::variant<KeyboardButton, MouseButton, GamepadButton>>& vItem2) {
    for (size_t i = 0; i < vItem1.size(); i++) {
        bool bFound = false;
        for (size_t j = 0; j < vItem2.size(); j++) {
            if (vItem2[j] == vItem1[i]) {
                bFound = true;
                break;
            }
        }
        REQUIRE(bFound);
    }
}

TEST_CASE("add action") {
    // Prepare trigger buttons and action events.
    const unsigned int iActionEvent1Id = 0;
    const std::vector<std::variant<KeyboardButton, MouseButton, GamepadButton>> vActionEvent1Buttons = {
        KeyboardButton::NUM_0, KeyboardButton::Z};

    const unsigned int iActionEvent2Id = 1;
    const std::vector<std::variant<KeyboardButton, MouseButton, GamepadButton>> vActionEvent2Buttons = {
        MouseButton::LEFT};

    const unsigned int iActionEvent3Id = 2;
    const std::vector<std::variant<KeyboardButton, MouseButton, GamepadButton>> vActionEvent3Buttons = {
        GamepadButton::BUTTON_RIGHT};

    // Register action events.
    InputManager manager;
    auto optionalError = manager.addActionEvent(iActionEvent1Id, vActionEvent1Buttons);
    REQUIRE(!optionalError.has_value());
    optionalError = manager.addActionEvent(iActionEvent2Id, vActionEvent2Buttons);
    REQUIRE(!optionalError.has_value());
    optionalError = manager.addActionEvent(iActionEvent3Id, vActionEvent3Buttons);
    REQUIRE(!optionalError.has_value());

    // Make sure buttons were added.
    const auto vEvent1Keys = manager.getActionEventButtons(iActionEvent1Id);
    const auto vEvent2Keys = manager.getActionEventButtons(iActionEvent2Id);
    const auto vEvent3Keys = manager.getActionEventButtons(iActionEvent3Id);
    REQUIRE(!vEvent1Keys.empty());
    REQUIRE(!vEvent2Keys.empty());
    REQUIRE(!vEvent3Keys.empty());

    // Compare keys.
    compareActionEventButtons(vEvent1Keys, vActionEvent1Buttons);
    compareActionEventButtons(vEvent2Keys, vActionEvent2Buttons);
    compareActionEventButtons(vEvent3Keys, vActionEvent3Buttons);
}

TEST_CASE("remove action") {
    // Prepare trigger buttons and action events.
    const unsigned int iActionEvent1Id = 0;
    const std::vector<std::variant<KeyboardButton, MouseButton, GamepadButton>> vActionEvent1Buttons = {
        KeyboardButton::NUM_0, KeyboardButton::Z};

    const unsigned int iActionEvent2Id = 1;
    const std::vector<std::variant<KeyboardButton, MouseButton, GamepadButton>> vActionEvent2Buttons = {
        MouseButton::LEFT};

    const unsigned int iActionEvent3Id = 2;
    const std::vector<std::variant<KeyboardButton, MouseButton, GamepadButton>> vActionEvent3Buttons = {
        GamepadButton::BUTTON_RIGHT};

    // Register action events.
    InputManager manager;
    auto optionalError = manager.addActionEvent(iActionEvent1Id, vActionEvent1Buttons);
    REQUIRE(!optionalError.has_value());
    optionalError = manager.addActionEvent(iActionEvent2Id, vActionEvent2Buttons);
    REQUIRE(!optionalError.has_value());
    optionalError = manager.addActionEvent(iActionEvent3Id, vActionEvent3Buttons);
    REQUIRE(!optionalError.has_value());

    // Remove action event.
    REQUIRE(!manager.removeActionEvent(iActionEvent1Id));
    REQUIRE(manager.getAllActionEvents().size() == 2);

    // Check that other events are correct.
    const auto vEvent2Keys = manager.getActionEventButtons(iActionEvent2Id);
    const auto vEvent3Keys = manager.getActionEventButtons(iActionEvent3Id);
    REQUIRE(!vEvent2Keys.empty());
    REQUIRE(!vEvent3Keys.empty());
    compareActionEventButtons(vEvent2Keys, vActionEvent2Buttons);
    compareActionEventButtons(vEvent3Keys, vActionEvent3Buttons);
}

TEST_CASE("fail to add an action event with already used ID") {
    const unsigned int iAction1Id = 0;
    const std::vector<std::variant<KeyboardButton, MouseButton, GamepadButton>> vAction1Keys = {
        KeyboardButton::NUM_0, KeyboardButton::Z};

    const std::vector<std::variant<KeyboardButton, MouseButton, GamepadButton>> vAction2Keys = {
        MouseButton::LEFT};

    InputManager manager;
    auto optional = manager.addActionEvent(iAction1Id, vAction1Keys);
    REQUIRE(!optional.has_value());

    optional = manager.addActionEvent(iAction1Id, vAction2Keys);
    REQUIRE(optional.has_value()); // should fail

    const auto vEventKeys = manager.getActionEventButtons(iAction1Id);
    REQUIRE(!vEventKeys.empty());

    // Compare keys (order may be different).
    REQUIRE(vEventKeys.size() == vAction1Keys.size());
    for (const auto& wantKey : vAction1Keys) {
        bool bFound = false;
        for (const auto& foundKey : vEventKeys) {
            if (foundKey == wantKey) {
                bFound = true;
                break;
            }
        }
        REQUIRE(bFound);
    }
}

TEST_CASE("modify action") {
    const unsigned int iAction1Id = 0;
    const std::vector<std::variant<KeyboardButton, MouseButton, GamepadButton>> vAction1Keys = {
        KeyboardButton::NUM_0, KeyboardButton::Z};

    const std::variant<KeyboardButton, MouseButton, GamepadButton> oldKey = KeyboardButton::Z;
    const std::variant<KeyboardButton, MouseButton, GamepadButton> newKey = MouseButton::LEFT;

    InputManager manager;
    auto optional = manager.addActionEvent(iAction1Id, vAction1Keys);
    REQUIRE(!optional.has_value());

    optional = manager.modifyActionEvent(iAction1Id, oldKey, newKey);
    REQUIRE(!optional.has_value());

    const std::vector<std::variant<KeyboardButton, MouseButton, GamepadButton>> vExpectedKeys = {
        KeyboardButton::NUM_0, MouseButton::LEFT};

    const auto vEventKeys = manager.getActionEventButtons(iAction1Id);
    REQUIRE(!vEventKeys.empty());

    // Compare keys (order may be different).
    REQUIRE(vEventKeys.size() == vExpectedKeys.size());
    for (const auto& wantKey : vExpectedKeys) {
        bool bFound = false;
        for (const auto& foundKey : vEventKeys) {
            if (foundKey == wantKey) {
                bFound = true;
                break;
            }
        }
        REQUIRE(bFound);
    }
}

TEST_CASE("add axis events") {
    // Prepare triggers.
    const unsigned int iAxis1Id = 0;
    const std::vector<std::pair<KeyboardButton, KeyboardButton>> vInitKeyboardTriggers1 = {
        std::make_pair<KeyboardButton, KeyboardButton>(KeyboardButton::W, KeyboardButton::S)};
    const std::vector<GamepadAxis> vInitGamepadTriggers1 = {GamepadAxis::LEFT_STICK_X};

    const unsigned int iAxis2Id = 1;
    const std::vector<std::pair<KeyboardButton, KeyboardButton>> vInitKeyboardTriggers2 = {
        std::make_pair<KeyboardButton, KeyboardButton>(KeyboardButton::R, KeyboardButton::A),
        std::make_pair<KeyboardButton, KeyboardButton>(KeyboardButton::RIGHT, KeyboardButton::LEFT)};

    const unsigned int iAxis3Id = 2;
    const std::vector<GamepadAxis> vInitGamepadTriggers3 = {GamepadAxis::RIGHT_TRIGGER};

    // Add axis events.
    InputManager manager;
    auto optionalError = manager.addAxisEvent(iAxis1Id, vInitKeyboardTriggers1, vInitGamepadTriggers1);
    REQUIRE(!optionalError.has_value());
    optionalError = manager.addAxisEvent(iAxis2Id, vInitKeyboardTriggers2, {});
    REQUIRE(!optionalError.has_value());
    optionalError = manager.addAxisEvent(iAxis3Id, {}, vInitGamepadTriggers3);
    REQUIRE(!optionalError.has_value());

    // Check triggers.
    const auto [vKeyboardTriggers1, vGamepadTriggers1] = manager.getAxisEventTriggers(iAxis1Id);
    const auto [vKeyboardTriggers2, vGamepadTriggers2] = manager.getAxisEventTriggers(iAxis2Id);
    const auto [vKeyboardTriggers3, vGamepadTriggers3] = manager.getAxisEventTriggers(iAxis3Id);
    REQUIRE(!vKeyboardTriggers1.empty());
    REQUIRE(!vGamepadTriggers1.empty());

    REQUIRE(!vKeyboardTriggers2.empty());
    REQUIRE(vGamepadTriggers2.empty());

    REQUIRE(vKeyboardTriggers3.empty());
    REQUIRE(!vGamepadTriggers3.empty());

    // Compare triggers.
    REQUIRE(vKeyboardTriggers1 == vInitKeyboardTriggers1);
    REQUIRE(vGamepadTriggers1 == vInitGamepadTriggers1);

    REQUIRE(vKeyboardTriggers2 == vInitKeyboardTriggers2);

    REQUIRE(vGamepadTriggers3 == vInitGamepadTriggers3);
}

TEST_CASE("remove axis events") {
    // Prepare triggers.
    const unsigned int iAxis1Id = 0;
    const std::vector<std::pair<KeyboardButton, KeyboardButton>> vInitKeyboardTriggers1 = {
        std::make_pair<KeyboardButton, KeyboardButton>(KeyboardButton::W, KeyboardButton::S)};
    const std::vector<GamepadAxis> vInitGamepadTriggers1 = {GamepadAxis::LEFT_STICK_X};

    const unsigned int iAxis2Id = 1;
    const std::vector<std::pair<KeyboardButton, KeyboardButton>> vInitKeyboardTriggers2 = {
        std::make_pair<KeyboardButton, KeyboardButton>(KeyboardButton::R, KeyboardButton::A),
        std::make_pair<KeyboardButton, KeyboardButton>(KeyboardButton::RIGHT, KeyboardButton::LEFT)};

    const unsigned int iAxis3Id = 2;
    const std::vector<GamepadAxis> vInitGamepadTriggers3 = {GamepadAxis::RIGHT_TRIGGER};

    // Add axis events.
    InputManager manager;
    auto optionalError = manager.addAxisEvent(iAxis1Id, vInitKeyboardTriggers1, vInitGamepadTriggers1);
    REQUIRE(!optionalError.has_value());
    optionalError = manager.addAxisEvent(iAxis2Id, vInitKeyboardTriggers2, {});
    REQUIRE(!optionalError.has_value());
    optionalError = manager.addAxisEvent(iAxis3Id, {}, vInitGamepadTriggers3);
    REQUIRE(!optionalError.has_value());

    // Remove event.
    REQUIRE(!manager.removeAxisEvent(iAxis1Id));
    REQUIRE(manager.getAllAxisEvents().size() == 2);

    // Check left events.
    const auto [vKeyboardTriggers2, vGamepadTriggers2] = manager.getAxisEventTriggers(iAxis2Id);
    REQUIRE(!vKeyboardTriggers2.empty());
    REQUIRE(vGamepadTriggers2.empty());

    const auto [vKeyboardTriggers3, vGamepadTriggers3] = manager.getAxisEventTriggers(iAxis3Id);
    REQUIRE(vKeyboardTriggers3.empty());
    REQUIRE(!vGamepadTriggers3.empty());

    // Compare triggers.
    REQUIRE(vKeyboardTriggers2 == vInitKeyboardTriggers2);

    REQUIRE(vGamepadTriggers3 == vInitGamepadTriggers3);
}

TEST_CASE("fail to add an axis event with already used ID") {
    InputManager manager;
    auto optionalError = manager.addAxisEvent(0, {{KeyboardButton::W, KeyboardButton::S}}, {});
    REQUIRE(!optionalError.has_value());

    optionalError = manager.addAxisEvent(0, {{KeyboardButton::W, KeyboardButton::S}}, {});
    REQUIRE(optionalError.has_value()); // should fail
}

TEST_CASE("modify triggers for an axis event") {
    // Add axis event.
    InputManager manager;
    auto optionalError = manager.addAxisEvent(
        0,
        {{KeyboardButton::W, KeyboardButton::S}, {KeyboardButton::UP, KeyboardButton::DOWN}},
        {GamepadAxis::LEFT_STICK_Y, GamepadAxis::RIGHT_STICK_Y});
    REQUIRE(!optionalError.has_value());

    // Modify keyboard triggers.
    optionalError = manager.modifyAxisEvent(
        0, {KeyboardButton::UP, KeyboardButton::DOWN}, {KeyboardButton::T, KeyboardButton::G});
    REQUIRE(!optionalError.has_value());

    // Modify gamepad triggers.
    optionalError = manager.modifyAxisEvent(0, GamepadAxis::LEFT_STICK_Y, GamepadAxis::LEFT_TRIGGER);
    REQUIRE(!optionalError.has_value());

    // Check new triggers.
    const auto [vKeyboardTriggers, vGamepadTriggers] = manager.getAxisEventTriggers(0);
    REQUIRE(!vKeyboardTriggers.empty());
    REQUIRE(!vGamepadTriggers.empty());

    const std::vector<std::pair<KeyboardButton, KeyboardButton>> vExpectedKeyboardTriggers = {
        {KeyboardButton::W, KeyboardButton::S}, {KeyboardButton::T, KeyboardButton::G}};
    const std::vector<GamepadAxis> vExpectedGamepadTriggers = {
        GamepadAxis::LEFT_TRIGGER, GamepadAxis::RIGHT_STICK_Y};

    // Compare triggers.
    REQUIRE(vKeyboardTriggers == vExpectedKeyboardTriggers);
    REQUIRE(vGamepadTriggers == vExpectedGamepadTriggers);
}

TEST_CASE("fail modify axis with wrong/flipped keys") {
    const unsigned int iAxis1Id = 0;
    const std::vector<std::pair<KeyboardButton, KeyboardButton>> vAxes1 = {
        std::make_pair<KeyboardButton, KeyboardButton>(KeyboardButton::W, KeyboardButton::S),
        std::make_pair<KeyboardButton, KeyboardButton>(KeyboardButton::UP, KeyboardButton::DOWN)};

    const std::pair<KeyboardButton, KeyboardButton> oldPair1 = {
        // flipped keys
        std::make_pair<KeyboardButton, KeyboardButton>(KeyboardButton::S, KeyboardButton::W)};
    const std::pair<KeyboardButton, KeyboardButton> oldPair2 = {
        // wrong key
        std::make_pair<KeyboardButton, KeyboardButton>(KeyboardButton::W, KeyboardButton::D)};
    const std::pair<KeyboardButton, KeyboardButton> newPair = {
        std::make_pair<KeyboardButton, KeyboardButton>(KeyboardButton::A, KeyboardButton::D)};

    InputManager manager;
    auto optional = manager.addAxisEvent(iAxis1Id, vAxes1, {});
    REQUIRE(!optional.has_value());

    // flipped pair
    optional = manager.modifyAxisEvent(iAxis1Id, oldPair1, newPair);
    REQUIRE(optional.has_value()); // should fail

    // wrong key
    optional = manager.modifyAxisEvent(iAxis1Id, oldPair2, newPair);
    REQUIRE(optional.has_value()); // should fail

    const auto [vKeyboardTriggers, vGamepadTriggers] = manager.getAxisEventTriggers(iAxis1Id);
    REQUIRE(!vKeyboardTriggers.empty());

    // Compare keys (order may be different).
    REQUIRE(vKeyboardTriggers == vAxes1);
}

TEST_CASE("test saving and loading") {
    // Prepare default action/axis events.
    const unsigned int iAction1Id = 0;
    const std::vector<std::variant<KeyboardButton, MouseButton, GamepadButton>> vDefaultAction1Keys = {
        MouseButton::LEFT};

    const unsigned int iAction2Id = 1;
    const std::vector<std::variant<KeyboardButton, MouseButton, GamepadButton>> vDefaultAction2Keys = {
        MouseButton::RIGHT, KeyboardButton::R};

    const unsigned int iAxis1Id = 0;
    const std::vector<std::pair<KeyboardButton, KeyboardButton>> vDefaultAxis1Keys = {
        std::make_pair<KeyboardButton, KeyboardButton>(KeyboardButton::A, KeyboardButton::D),
        std::make_pair<KeyboardButton, KeyboardButton>(KeyboardButton::UP, KeyboardButton::DOWN)};
    const std::vector<GamepadAxis> vDefaultAxis1GamepadTriggers = {GamepadAxis::LEFT_STICK_X};

    const auto sFileName = "input";

    {
        // Add default events to manager.
        InputManager manager;
        auto optionalError = manager.addActionEvent(iAction1Id, vDefaultAction1Keys);
        REQUIRE(!optionalError.has_value());
        optionalError = manager.addActionEvent(iAction2Id, vDefaultAction2Keys);
        REQUIRE(!optionalError.has_value());
        optionalError = manager.addAxisEvent(iAxis1Id, vDefaultAxis1Keys, vDefaultAxis1GamepadTriggers);
        REQUIRE(!optionalError.has_value());

        // The user modifies some keys.
        optionalError = manager.modifyActionEvent(iAction2Id, MouseButton::RIGHT, KeyboardButton::A);
        REQUIRE(!optionalError.has_value());
        optionalError = manager.modifyAxisEvent(
            iAxis1Id, {KeyboardButton::UP, KeyboardButton::DOWN}, {KeyboardButton::T, KeyboardButton::G});
        REQUIRE(!optionalError.has_value());
        optionalError =
            manager.modifyAxisEvent(iAxis1Id, GamepadAxis::LEFT_STICK_X, GamepadAxis::RIGHT_STICK_X);
        REQUIRE(!optionalError.has_value());

        // Save modified events.
        auto result = manager.saveToFile(sFileName);
        if (result.has_value()) {
            result->addCurrentLocationToErrorStack();
            INFO(result->getFullErrorMessage());
            REQUIRE(false);
        }
    }

    {
        const std::vector<std::variant<KeyboardButton, MouseButton, GamepadButton>> vExpectedAction1Keys = {
            MouseButton::LEFT};

        const std::vector<std::variant<KeyboardButton, MouseButton, GamepadButton>> vExpectedAction2Keys = {
            KeyboardButton::A, KeyboardButton::R};

        const std::vector<std::pair<KeyboardButton, KeyboardButton>> vExpectedAxis1Keys = {
            std::make_pair<KeyboardButton, KeyboardButton>(KeyboardButton::A, KeyboardButton::D),
            std::make_pair<KeyboardButton, KeyboardButton>(KeyboardButton::T, KeyboardButton::G)};
        const std::vector<GamepadAxis> vExpectedAxis1GamepadTriggers = {GamepadAxis::RIGHT_STICK_X};

        // Next startup, we add default keys first.
        // Add default events to manager.
        InputManager manager;
        auto optional = manager.addActionEvent(iAction1Id, vDefaultAction1Keys);
        REQUIRE(!optional.has_value());
        optional = manager.addActionEvent(iAction2Id, vDefaultAction2Keys);
        REQUIRE(!optional.has_value());
        optional = manager.addAxisEvent(iAxis1Id, vDefaultAxis1Keys, vDefaultAxis1GamepadTriggers);
        REQUIRE(!optional.has_value());

        // Load modified events.
        auto result = manager.overwriteExistingEventsButtonsFromFile(sFileName);
        if (result.has_value()) [[unlikely]] {
            result->addCurrentLocationToErrorStack();
            INFO(result->getFullErrorMessage());
            REQUIRE(false);
        }

        // Check that buttons are correct.

        // Action 1.
        auto vReadAction = manager.getActionEventButtons(iAction1Id);
        REQUIRE(!vReadAction.empty());
        REQUIRE(vReadAction == vExpectedAction1Keys);

        vReadAction = manager.getActionEventButtons(iAction2Id);
        REQUIRE(!vReadAction.empty());
        compareActionEventButtons(vReadAction, vExpectedAction2Keys);

        const auto [vKeyboardTriggers, vGamepadTriggers] = manager.getAxisEventTriggers(iAxis1Id);
        REQUIRE(!vKeyboardTriggers.empty());
        REQUIRE(!vGamepadTriggers.empty());
        REQUIRE(vKeyboardTriggers == vExpectedAxis1Keys);
        REQUIRE(vGamepadTriggers == vExpectedAxis1GamepadTriggers);
    }
}

TEST_CASE("is button used") {
    const unsigned int iAction1Id = 0;
    const std::vector<std::variant<KeyboardButton, MouseButton, GamepadButton>> vAction1Keys = {
        KeyboardButton::NUM_0, KeyboardButton::Z};

    const unsigned int iAction2Id = 1;
    const std::vector<std::variant<KeyboardButton, MouseButton, GamepadButton>> vAction2Keys = {
        KeyboardButton::LEFT};

    const unsigned int iAxis2Id = 0;
    const std::vector<std::pair<KeyboardButton, KeyboardButton>> vAxes2 = {
        std::make_pair<KeyboardButton, KeyboardButton>(KeyboardButton::R, KeyboardButton::A),
        std::make_pair<KeyboardButton, KeyboardButton>(KeyboardButton::RIGHT, KeyboardButton::LEFT)};

    InputManager manager;
    auto optional = manager.addActionEvent(iAction1Id, vAction1Keys);
    REQUIRE(!optional.has_value());
    optional = manager.addActionEvent(iAction2Id, vAction2Keys);
    REQUIRE(!optional.has_value());
    optional = manager.addAxisEvent(iAxis2Id, vAxes2, {});
    REQUIRE(!optional.has_value());

    // First test.
    auto [vActionEvent1Ids, vAxisEvents] = manager.isButtonUsed(KeyboardButton::LEFT);
    REQUIRE(vActionEvent1Ids.size() == 1);
    REQUIRE(vAxisEvents.size() == 1);

    // Find registered action event.
    bool bFound = false;
    for (const auto& iFoundActionId : vActionEvent1Ids) {
        if (iFoundActionId == iAction2Id) {
            bFound = true;
            break;
        }
    }
    REQUIRE(bFound);

    // Find registered axis event.
    for (const auto& iFoundActionId : vAxisEvents) {
        if (iFoundActionId == iAxis2Id) {
            bFound = true;
            break;
        }
    }
    REQUIRE(bFound);

    // Another test.
    auto [vActionEvent2Ids, vAxisEvents2] = manager.isButtonUsed(KeyboardButton::NUM_0);
    REQUIRE(vActionEvent2Ids.size() == 1);
    REQUIRE(vAxisEvents2.empty());

    // Find registered action.
    bFound = false;
    for (const auto& iFoundActionId : vActionEvent2Ids) {
        if (iFoundActionId == iAction1Id) {
            bFound = true;
            break;
        }
    }
    REQUIRE(bFound);
}
