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
 * Converts gamepad button enum value to a string.
 *
 * @param button Gamepad button.
 *
 * @return Button name (for ex. "leftshoulder", "dpleft").
 */
inline std::string getGamepadButtonName(GamepadButton button) {
    return SDL_GameControllerGetStringForButton(static_cast<SDL_GameControllerButton>(button));
}
