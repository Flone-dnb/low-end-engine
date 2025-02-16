#pragma once

// Standard.
#include <string>

// External.
#include "SDL_gamecontroller.h"

/**
 * Mapping from SDL game controller buttons.
 *
 * @remark Also see @ref getGamepadButtonName.
 */
enum class GamepadButton {
    X = SDL_CONTROLLER_BUTTON_X,
    Y = SDL_CONTROLLER_BUTTON_Y,
    B = SDL_CONTROLLER_BUTTON_B,
    A = SDL_CONTROLLER_BUTTON_A,
    START = SDL_CONTROLLER_BUTTON_START,
    BACK = SDL_CONTROLLER_BUTTON_BACK,
    DPAD_LEFT = SDL_CONTROLLER_BUTTON_DPAD_LEFT,
    DPAD_UP = SDL_CONTROLLER_BUTTON_DPAD_UP,
    DPAD_RIGHT = SDL_CONTROLLER_BUTTON_DPAD_RIGHT,
    DPAD_DOWN = SDL_CONTROLLER_BUTTON_DPAD_DOWN,
    LEFT_STICK = SDL_CONTROLLER_BUTTON_LEFTSTICK,
    RIGHT_STICK = SDL_CONTROLLER_BUTTON_RIGHTSTICK,
    LEFT_SHOULDER = SDL_CONTROLLER_BUTTON_LEFTSHOULDER,
    RIGHT_SHOULDER = SDL_CONTROLLER_BUTTON_RIGHTSHOULDER
};

/**
 * Mapping from SDL game controller axes.
 *
 * @remark Also see @ref getGamepadAxisName.
 */
enum class GamepadAxis {
    RIGHT_TRIGGER = SDL_CONTROLLER_AXIS_TRIGGERRIGHT,
    LEFT_TRIGGER = SDL_CONTROLLER_AXIS_TRIGGERLEFT,
    RIGHT_STICK_X = SDL_CONTROLLER_AXIS_RIGHTX,
    RIGHT_STICK_Y = SDL_CONTROLLER_AXIS_RIGHTY,
    LEFT_STICK_X = SDL_CONTROLLER_AXIS_LEFTX,
    LEFT_STICK_Y = SDL_CONTROLLER_AXIS_LEFTY,
};

/**
 * Converts gamepad button enum value to a string.
 *
 * @param button Gamepad button.
 *
 * @return Button name (for ex. "leftshoulder", "dpleft").
 */
inline std::string getGamepadButtonName(GamepadButton button) {
    return SDL_GameControllerGetStringForButton(static_cast<SDL_GameControllerButton>(button));
}

/**
 * Converts gamepad axis enum value to a string.
 *
 * @param axis Gamepad axis.
 *
 * @return Axis name.
 */
inline std::string getGamepadAxisName(GamepadAxis axis) {
    return SDL_GameControllerGetStringForAxis(static_cast<SDL_GameControllerAxis>(axis));
}
