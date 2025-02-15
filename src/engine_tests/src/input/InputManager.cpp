// Custom.
#include "input/InputManager.h"

// External.
#include "catch2/catch_test_macros.hpp"

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
        GamepadButton::X};

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
    REQUIRE(vEvent1Keys == vActionEvent1Buttons);
    REQUIRE(vEvent2Keys == vActionEvent2Buttons);
    REQUIRE(vEvent3Keys == vActionEvent3Buttons);
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
        GamepadButton::X};

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
    REQUIRE(vEvent2Keys == vActionEvent2Buttons);
    REQUIRE(vEvent3Keys == vActionEvent3Buttons);
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

TEST_CASE("add axis") {
    const unsigned int iAxis1Id = 0;
    const std::vector<std::pair<KeyboardButton, KeyboardButton>> vAxes1 = {
        std::make_pair<KeyboardButton, KeyboardButton>(KeyboardButton::W, KeyboardButton::S)};

    const unsigned int iAxis2Id = 1;
    const std::vector<std::pair<KeyboardButton, KeyboardButton>> vAxes2 = {
        std::make_pair<KeyboardButton, KeyboardButton>(KeyboardButton::R, KeyboardButton::A),
        std::make_pair<KeyboardButton, KeyboardButton>(KeyboardButton::RIGHT, KeyboardButton::LEFT)};

    InputManager manager;
    auto optional = manager.addAxisEvent(iAxis1Id, vAxes1);
    REQUIRE(!optional.has_value());
    optional = manager.addAxisEvent(iAxis2Id, vAxes2);
    REQUIRE(!optional.has_value());

    auto vAxisEvent1Keys = manager.getAxisEventTriggers(iAxis1Id);
    auto vAxisEvent2Keys = manager.getAxisEventTriggers(iAxis2Id);
    REQUIRE(!vAxisEvent1Keys.empty());
    REQUIRE(!vAxisEvent2Keys.empty());

    // Compare keys (order may be different).
    REQUIRE(vAxisEvent1Keys == vAxes1);
    REQUIRE(vAxisEvent2Keys.size() == vAxes2.size());
    for (const auto& wantAxis : vAxes2) {
        bool bFound = false;
        for (const auto& foundAxis : vAxisEvent2Keys) {
            if (foundAxis == wantAxis) {
                bFound = true;
                break;
            }
        }
        REQUIRE(bFound);
    }
}

TEST_CASE("remove axis") {
    const unsigned int iAxis1Id = 0;
    const std::vector<std::pair<KeyboardButton, KeyboardButton>> vAxes1 = {
        std::make_pair<KeyboardButton, KeyboardButton>(KeyboardButton::W, KeyboardButton::S)};

    const unsigned int iAxis2Id = 1;
    const std::vector<std::pair<KeyboardButton, KeyboardButton>> vAxes2 = {
        std::make_pair<KeyboardButton, KeyboardButton>(KeyboardButton::R, KeyboardButton::A),
        std::make_pair<KeyboardButton, KeyboardButton>(KeyboardButton::RIGHT, KeyboardButton::LEFT)};

    InputManager manager;
    auto optional = manager.addAxisEvent(iAxis1Id, vAxes1);
    REQUIRE(!optional.has_value());
    optional = manager.addAxisEvent(iAxis2Id, vAxes2);
    REQUIRE(!optional.has_value());

    REQUIRE(!manager.removeAxisEvent(iAxis1Id));

    REQUIRE(manager.getAllAxisEvents().size() == 1);

    auto vAxisEvent2keys = manager.getAxisEventTriggers(iAxis2Id);
    REQUIRE(!vAxisEvent2keys.empty());

    // Compare keys (order may be different).
    REQUIRE(vAxisEvent2keys.size() == vAxes2.size());
    for (const auto& wantAxis : vAxes2) {
        bool bFound = false;
        for (const auto& foundAxis : vAxisEvent2keys) {
            if (foundAxis == wantAxis) {
                bFound = true;
                break;
            }
        }
        REQUIRE(bFound);
    }
}

TEST_CASE("fail to add an axis event with already used ID") {
    const unsigned int iAxis1Id = 0;
    const std::vector<std::pair<KeyboardButton, KeyboardButton>> vAxes1 = {
        std::make_pair<KeyboardButton, KeyboardButton>(KeyboardButton::W, KeyboardButton::S)};

    const std::vector<std::pair<KeyboardButton, KeyboardButton>> vAxes2 = {
        std::make_pair<KeyboardButton, KeyboardButton>(KeyboardButton::R, KeyboardButton::A),
        std::make_pair<KeyboardButton, KeyboardButton>(KeyboardButton::RIGHT, KeyboardButton::LEFT)};

    InputManager manager;
    auto optional = manager.addAxisEvent(iAxis1Id, vAxes1);
    REQUIRE(!optional.has_value());

    optional = manager.addAxisEvent(iAxis1Id, vAxes2);
    REQUIRE(optional.has_value()); // should fail

    const auto vEventKeys = manager.getAxisEventTriggers(iAxis1Id);
    REQUIRE(!vEventKeys.empty());
    REQUIRE(vEventKeys == vAxes1);
}

TEST_CASE("modify axis") {
    const unsigned int iAxis1Id = 0;
    const std::vector<std::pair<KeyboardButton, KeyboardButton>> vAxes1 = {
        std::make_pair<KeyboardButton, KeyboardButton>(KeyboardButton::W, KeyboardButton::S),
        std::make_pair<KeyboardButton, KeyboardButton>(KeyboardButton::UP, KeyboardButton::DOWN)};

    const std::pair<KeyboardButton, KeyboardButton> oldPair = {
        std::make_pair<KeyboardButton, KeyboardButton>(KeyboardButton::W, KeyboardButton::S)};
    const std::pair<KeyboardButton, KeyboardButton> newPair = {
        std::make_pair<KeyboardButton, KeyboardButton>(KeyboardButton::A, KeyboardButton::D)};

    InputManager manager;
    auto optional = manager.addAxisEvent(iAxis1Id, vAxes1);
    REQUIRE(!optional.has_value());

    optional = manager.modifyAxisEvent(iAxis1Id, oldPair, newPair);
    REQUIRE(!optional.has_value());

    const std::vector<std::pair<KeyboardButton, KeyboardButton>> vExpectedKeys = {
        std::make_pair<KeyboardButton, KeyboardButton>(KeyboardButton::A, KeyboardButton::D),
        std::make_pair<KeyboardButton, KeyboardButton>(KeyboardButton::UP, KeyboardButton::DOWN)};

    const auto vEventKeys = manager.getAxisEventTriggers(iAxis1Id);
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
    auto optional = manager.addAxisEvent(iAxis1Id, vAxes1);
    REQUIRE(!optional.has_value());

    // flipped pair
    optional = manager.modifyAxisEvent(iAxis1Id, oldPair1, newPair);
    REQUIRE(optional.has_value()); // should fail

    // wrong key
    optional = manager.modifyAxisEvent(iAxis1Id, oldPair2, newPair);
    REQUIRE(optional.has_value()); // should fail

    const auto vEventKeys = manager.getAxisEventTriggers(iAxis1Id);
    REQUIRE(!vEventKeys.empty());

    // Compare keys (order may be different).
    REQUIRE(vEventKeys.size() == vAxes1.size());
    for (const auto& wantKey : vAxes1) {
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

    // Modify some events.
    const std::variant<KeyboardButton, MouseButton, GamepadButton> oldAction2Key = MouseButton::RIGHT;
    const std::variant<KeyboardButton, MouseButton, GamepadButton> newAction2Key = KeyboardButton::A;

    constexpr std::pair<KeyboardButton, KeyboardButton> oldAxis1Key =
        std::make_pair<KeyboardButton, KeyboardButton>(KeyboardButton::UP, KeyboardButton::DOWN);
    constexpr std::pair<KeyboardButton, KeyboardButton> newAxis1Key =
        std::make_pair<KeyboardButton, KeyboardButton>(KeyboardButton::T, KeyboardButton::G);

    // Expected.
    const std::vector<std::variant<KeyboardButton, MouseButton, GamepadButton>> vExpectedAction1Keys = {
        MouseButton::LEFT};

    const std::vector<std::variant<KeyboardButton, MouseButton, GamepadButton>> vExpectedAction2Keys = {
        KeyboardButton::A, KeyboardButton::R};

    const std::vector<std::pair<KeyboardButton, KeyboardButton>> vExpectedAxis1Keys = {
        std::make_pair<KeyboardButton, KeyboardButton>(KeyboardButton::A, KeyboardButton::D),
        std::make_pair<KeyboardButton, KeyboardButton>(KeyboardButton::T, KeyboardButton::G)};

    const auto sFileName = "input";

    {
        // Add default events to manager.
        InputManager manager;
        auto optional = manager.addActionEvent(iAction1Id, vDefaultAction1Keys);
        REQUIRE(!optional.has_value());
        optional = manager.addActionEvent(iAction2Id, vDefaultAction2Keys);
        REQUIRE(!optional.has_value());
        optional = manager.addAxisEvent(iAxis1Id, vDefaultAxis1Keys);
        REQUIRE(!optional.has_value());

        // The user modifies some keys.
        optional = manager.modifyActionEvent(iAction2Id, oldAction2Key, newAction2Key);
        REQUIRE(!optional.has_value());
        optional = manager.modifyAxisEvent(iAxis1Id, oldAxis1Key, newAxis1Key);
        REQUIRE(!optional.has_value());

        // Save modified events.
        auto result = manager.saveToFile(sFileName);
        if (result.has_value()) {
            result->addCurrentLocationToErrorStack();
            INFO(result->getFullErrorMessage());
            REQUIRE(false);
        }
    }

    {
        // Next startup, we add default keys first.
        // Add default events to manager.
        InputManager manager;
        auto optional = manager.addActionEvent(iAction1Id, vDefaultAction1Keys);
        REQUIRE(!optional.has_value());
        optional = manager.addActionEvent(iAction2Id, vDefaultAction2Keys);
        REQUIRE(!optional.has_value());
        optional = manager.addAxisEvent(iAxis1Id, vDefaultAxis1Keys);
        REQUIRE(!optional.has_value());

        // Load modified events.
        auto result = manager.overwriteExistingEventsButtonsFromFile(sFileName);
        if (result.has_value()) {
            result->addCurrentLocationToErrorStack();
            INFO(result->getFullErrorMessage());
            REQUIRE(false);
        }

        // Check that keys are correct.

        // Action 1.
        auto vReadAction = manager.getActionEventButtons(iAction1Id);
        REQUIRE(!vReadAction.empty());
        REQUIRE(vReadAction.size() == vExpectedAction1Keys.size());

        // Compare keys (order may be different).
        for (const auto& wantAction : vExpectedAction1Keys) {
            bool bFound = false;
            for (const auto& foundAction : vReadAction) {
                if (foundAction == wantAction) {
                    bFound = true;
                    break;
                }
            }
            REQUIRE(bFound);
        }

        // Action 2.
        vReadAction = manager.getActionEventButtons(iAction2Id);
        REQUIRE(!vReadAction.empty());
        REQUIRE(vReadAction.size() == vExpectedAction2Keys.size());

        // Compare keys (order may be different).
        for (const auto& wantAction : vExpectedAction2Keys) {
            bool bFound = false;
            for (const auto& foundAction : vReadAction) {
                if (foundAction == wantAction) {
                    bFound = true;
                    break;
                }
            }
            REQUIRE(bFound);
        }

        // Axis 1.
        auto vReadAxis = manager.getAxisEventTriggers(iAxis1Id);
        REQUIRE(!vReadAxis.empty());
        REQUIRE(vReadAxis.size() == vExpectedAxis1Keys.size());

        // Compare keys (order may be different).
        for (const auto& wantAxis : vExpectedAxis1Keys) {
            bool bFound = false;
            for (const auto& foundAxis : vReadAxis) {
                if (foundAxis == wantAxis) {
                    bFound = true;
                    break;
                }
            }
            REQUIRE(bFound);
        }
    }
}

TEST_CASE("is key used") {
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
    optional = manager.addAxisEvent(iAxis2Id, vAxes2);
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
