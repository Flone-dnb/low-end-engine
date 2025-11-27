#pragma once

// External.
#include "SDL3/SDL_mouse.h"

enum class MouseButton {
    LEFT = SDL_BUTTON_LEFT,
    RIGHT = SDL_BUTTON_RIGHT,
    MIDDLE = SDL_BUTTON_MIDDLE,
    X1 = SDL_BUTTON_X1,
    X2 = SDL_BUTTON_X2,
};

/**
 * Converts mouse button enum value to a string.
 *
 * @param button Mouse button.
 *
 * @return Button name.
 */
inline std::string getMouseButtonName(MouseButton button) {
    switch (button) {
    case (MouseButton::LEFT):
        return "left mouse button";
    case (MouseButton::RIGHT):
        return "right mouse button";
    case (MouseButton::MIDDLE):
        return "middle mouse button";
    case (MouseButton::X1):
        return "X1 mouse button";
    case (MouseButton::X2):
        return "X2 mouse button";
    default:
        return "unknown mouse button";
    };
}
