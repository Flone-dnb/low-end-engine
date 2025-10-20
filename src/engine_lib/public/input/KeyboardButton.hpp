#pragma once

// Standard.
#include <string>

// External.
#include "SDL3/SDL_keycode.h"
#include "SDL3/SDL_keyboard.h"

/** Provides a mapping from SDL keyboard modifiers to a class. */
class KeyboardModifiers {
public:
    KeyboardModifiers() = delete;

    /**
     * Constructor.
     *
     * @param iModifiers SDL modifiers value.
     */
    explicit KeyboardModifiers(uint16_t iModifiers) { this->iModifiers = iModifiers; }

    /**
     * Whether the Shift key is pressed or not.
     *
     * @return 'true' if the Shift key is pressed, 'false' otherwise.
     */
    bool isShiftPressed() const { return iModifiers & SDL_KMOD_LSHIFT; }

    /**
     * Whether the Control (Ctrl) key is pressed or not.
     *
     * @return 'true' if the Control (Ctrl) key is pressed, 'false' otherwise.
     */
    bool isControlPressed() const { return iModifiers & SDL_KMOD_LCTRL; }

    /**
     * Whether the Alt key is pressed or not.
     *
     * @return 'true' if the Alt key is pressed, 'false' otherwise.
     */
    bool isAltPressed() const { return iModifiers & SDL_KMOD_LALT; }

    /**
     * Whether the Caps Lock key is pressed or not.
     *
     * @return 'true' if the Caps Lock key is pressed, 'false' otherwise.
     */
    bool isCapsLockPressed() const { return iModifiers & SDL_KMOD_CAPS; }

    /**
     * Whether the Num Lock key is pressed or not.
     *
     * @return 'true' if the Num Lock key is pressed, 'false' otherwise.
     */
    bool isNumLockPressed() const { return iModifiers & SDL_KMOD_NUM; }

private:
    /** SDL modifiers value. */
    uint16_t iModifiers;
};

#undef DELETE
/**
 * Mapping from SDL keyboard keys.
 *
 * @remark Also see @ref getKeyName.
 */
enum class KeyboardButton {
    SPACE = SDL_SCANCODE_SPACE,
    COMMA = SDL_SCANCODE_COMMA,
    MINUS = SDL_SCANCODE_MINUS,
    PERIOD = SDL_SCANCODE_PERIOD,
    SLASH = SDL_SCANCODE_SLASH,
    TILDE = SDL_SCANCODE_GRAVE,
    NUM_0 = SDL_SCANCODE_0, //< Can't use 0 as name so adding a NUM here.
    NUM_1 = SDL_SCANCODE_1,
    NUM_2 = SDL_SCANCODE_2,
    NUM_3 = SDL_SCANCODE_3,
    NUM_4 = SDL_SCANCODE_4,
    NUM_5 = SDL_SCANCODE_5,
    NUM_6 = SDL_SCANCODE_6,
    NUM_7 = SDL_SCANCODE_7,
    NUM_8 = SDL_SCANCODE_8,
    NUM_9 = SDL_SCANCODE_9,
    SEMICOLON = SDL_SCANCODE_SEMICOLON,
    EQUALS = SDL_SCANCODE_EQUALS,
    A = SDL_SCANCODE_A,
    B = SDL_SCANCODE_B,
    C = SDL_SCANCODE_C,
    D = SDL_SCANCODE_D,
    E = SDL_SCANCODE_E,
    F = SDL_SCANCODE_F,
    G = SDL_SCANCODE_G,
    H = SDL_SCANCODE_H,
    I = SDL_SCANCODE_I,
    J = SDL_SCANCODE_J,
    K = SDL_SCANCODE_K,
    L = SDL_SCANCODE_L,
    M = SDL_SCANCODE_M,
    N = SDL_SCANCODE_N,
    O = SDL_SCANCODE_O,
    P = SDL_SCANCODE_P,
    Q = SDL_SCANCODE_Q,
    R = SDL_SCANCODE_R,
    S = SDL_SCANCODE_S,
    T = SDL_SCANCODE_T,
    U = SDL_SCANCODE_U,
    V = SDL_SCANCODE_V,
    W = SDL_SCANCODE_W,
    X = SDL_SCANCODE_X,
    Y = SDL_SCANCODE_Y,
    Z = SDL_SCANCODE_Z,
    BACKSLASH = SDL_SCANCODE_BACKSLASH,
    ESCAPE = SDL_SCANCODE_ESCAPE,
    ENTER = SDL_SCANCODE_RETURN,
    TAB = SDL_SCANCODE_TAB,
    BACKSPACE = SDL_SCANCODE_BACKSPACE,
    INSERT = SDL_SCANCODE_INSERT,
    DELETE = SDL_SCANCODE_DELETE,
    RIGHT = SDL_SCANCODE_RIGHT,
    LEFT = SDL_SCANCODE_LEFT,
    DOWN = SDL_SCANCODE_DOWN,
    UP = SDL_SCANCODE_UP,
    HOME = SDL_SCANCODE_HOME,
    END = SDL_SCANCODE_END,
    CAPS_LOCK = SDL_SCANCODE_CAPSLOCK,
    PRINT_SCREEN = SDL_SCANCODE_PRINTSCREEN,
    PAUSE = SDL_SCANCODE_PAUSE,
    F1 = SDL_SCANCODE_F1,
    F2 = SDL_SCANCODE_F2,
    F3 = SDL_SCANCODE_F3,
    F4 = SDL_SCANCODE_F4,
    F5 = SDL_SCANCODE_F5,
    F6 = SDL_SCANCODE_F6,
    F7 = SDL_SCANCODE_F7,
    F8 = SDL_SCANCODE_F8,
    F9 = SDL_SCANCODE_F9,
    F10 = SDL_SCANCODE_F10,
    F11 = SDL_SCANCODE_F11,
    F12 = SDL_SCANCODE_F12,
    NUMPAD_0 = SDL_SCANCODE_KP_0,
    NUMPAD_1 = SDL_SCANCODE_KP_1,
    NUMPAD_2 = SDL_SCANCODE_KP_2,
    NUMPAD_3 = SDL_SCANCODE_KP_3,
    NUMPAD_4 = SDL_SCANCODE_KP_4,
    NUMPAD_5 = SDL_SCANCODE_KP_5,
    NUMPAD_6 = SDL_SCANCODE_KP_6,
    NUMPAD_7 = SDL_SCANCODE_KP_7,
    NUMPAD_8 = SDL_SCANCODE_KP_8,
    NUMPAD_9 = SDL_SCANCODE_KP_9,
    LEFT_SHIFT = SDL_SCANCODE_LSHIFT,
    LEFT_CONTROL = SDL_SCANCODE_LCTRL,
    LEFT_ALT = SDL_SCANCODE_LALT,
    RIGHT_SHIFT = SDL_SCANCODE_RSHIFT,
    RIGHT_CONTROL = SDL_SCANCODE_RCTRL,
    RIGHT_ALT = SDL_SCANCODE_RALT,
};

/**
 * Converts keyboard button enum value to a string.
 *
 * @param button Keyboard button.
 *
 * @return Button name.
 */
inline std::string getKeyboardButtonName(KeyboardButton button) {
    return SDL_GetKeyName(SDL_SCANCODE_TO_KEYCODE(static_cast<SDL_Scancode>(button)));
}
