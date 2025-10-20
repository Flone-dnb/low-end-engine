#pragma once

// Standard.
#include <string>

// External.
#include "SDL3/SDL_gamepad.h"

/**
 * Mapping from SDL game controller buttons.
 *
 * @remark Also see @ref getGamepadButtonName.
 */
enum class GamepadButton {
    BUTTON_LEFT = SDL_GAMEPAD_BUTTON_WEST, //< one of the 4 buttons on the right side of the gamepad, X button
                                           //< on xbox, square on sony and so on
    BUTTON_UP = SDL_GAMEPAD_BUTTON_NORTH,
    BUTTON_RIGHT = SDL_GAMEPAD_BUTTON_EAST,
    BUTTON_DOWN = SDL_GAMEPAD_BUTTON_SOUTH,
    START = SDL_GAMEPAD_BUTTON_START,
    BACK = SDL_GAMEPAD_BUTTON_BACK,
    DPAD_LEFT = SDL_GAMEPAD_BUTTON_DPAD_LEFT,
    DPAD_UP = SDL_GAMEPAD_BUTTON_DPAD_UP,
    DPAD_RIGHT = SDL_GAMEPAD_BUTTON_DPAD_RIGHT,
    DPAD_DOWN = SDL_GAMEPAD_BUTTON_DPAD_DOWN,
    LEFT_STICK = SDL_GAMEPAD_BUTTON_LEFT_STICK,
    RIGHT_STICK = SDL_GAMEPAD_BUTTON_RIGHT_STICK,
    LEFT_SHOULDER = SDL_GAMEPAD_BUTTON_LEFT_SHOULDER,
    RIGHT_SHOULDER = SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER
};

/**
 * Mapping from SDL game controller axes.
 *
 * @remark Also see @ref getGamepadAxisName.
 */
enum class GamepadAxis {
    RIGHT_TRIGGER = SDL_GAMEPAD_AXIS_RIGHT_TRIGGER,
    LEFT_TRIGGER = SDL_GAMEPAD_AXIS_LEFT_TRIGGER,
    RIGHT_STICK_X = SDL_GAMEPAD_AXIS_RIGHTX,
    RIGHT_STICK_Y = SDL_GAMEPAD_AXIS_RIGHTY,
    LEFT_STICK_X = SDL_GAMEPAD_AXIS_LEFTX,
    LEFT_STICK_Y = SDL_GAMEPAD_AXIS_LEFTY,
};

/**
 * Converts gamepad button enum value to a string.
 *
 * @param button Gamepad button.
 *
 * @return Button name (for ex. "leftshoulder", "dpleft").
 */
inline std::string getGamepadButtonName(GamepadButton button) {
    return SDL_GetGamepadStringForButton(static_cast<SDL_GamepadButton>(button));
}

/**
 * Converts gamepad axis enum value to a string.
 *
 * @param axis Gamepad axis.
 *
 * @return Axis name.
 */
inline std::string getGamepadAxisName(GamepadAxis axis) {
    return SDL_GetGamepadStringForAxis(static_cast<SDL_GamepadAxis>(axis));
}
