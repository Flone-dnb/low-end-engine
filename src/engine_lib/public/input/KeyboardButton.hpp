#pragma once

// Standard.
#include <string>

// External.
#include "SDL_keycode.h"
#include "SDL_keyboard.h"

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
    bool isShiftPressed() const { return iModifiers & KMOD_LSHIFT; }

    /**
     * Whether the Control (Ctrl) key is pressed or not.
     *
     * @return 'true' if the Control (Ctrl) key is pressed, 'false' otherwise.
     */
    bool isControlPressed() const { return iModifiers & KMOD_LCTRL; }

    /**
     * Whether the Alt key is pressed or not.
     *
     * @return 'true' if the Alt key is pressed, 'false' otherwise.
     */
    bool isAltPressed() const { return iModifiers & KMOD_LALT; }

    /**
     * Whether the Caps Lock key is pressed or not.
     *
     * @return 'true' if the Caps Lock key is pressed, 'false' otherwise.
     */
    bool isCapsLockPressed() const { return iModifiers & KMOD_CAPS; }

    /**
     * Whether the Num Lock key is pressed or not.
     *
     * @return 'true' if the Num Lock key is pressed, 'false' otherwise.
     */
    bool isNumLockPressed() const { return iModifiers & KMOD_NUM; }

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
    SPACE = SDLK_SPACE,
    COMMA = SDLK_COMMA,
    MINUS = SDLK_MINUS,
    PERIOD = SDLK_PERIOD,
    SLASH = SDLK_SLASH,
    NUM_0 = SDLK_0, //< Can't use 0 as name so adding a NUM here.
    NUM_1 = SDLK_1,
    NUM_2 = SDLK_2,
    NUM_3 = SDLK_3,
    NUM_4 = SDLK_4,
    NUM_5 = SDLK_5,
    NUM_6 = SDLK_6,
    NUM_7 = SDLK_7,
    NUM_8 = SDLK_8,
    NUM_9 = SDLK_9,
    SEMICOLON = SDLK_SEMICOLON,
    EQUALS = SDLK_EQUALS,
    A = SDLK_a,
    B = SDLK_b,
    C = SDLK_c,
    D = SDLK_d,
    E = SDLK_e,
    F = SDLK_f,
    G = SDLK_g,
    H = SDLK_h,
    I = SDLK_i,
    J = SDLK_j,
    K = SDLK_k,
    L = SDLK_l,
    M = SDLK_m,
    N = SDLK_n,
    O = SDLK_o,
    P = SDLK_p,
    Q = SDLK_q,
    R = SDLK_r,
    S = SDLK_s,
    T = SDLK_t,
    U = SDLK_u,
    V = SDLK_v,
    W = SDLK_w,
    X = SDLK_x,
    Y = SDLK_y,
    Z = SDLK_z,
    BACKSLASH = SDLK_BACKSLASH,
    ESCAPE = SDLK_ESCAPE,
    ENTER = SDLK_RETURN,
    TAB = SDLK_TAB,
    BACKSPACE = SDLK_BACKSPACE,
    INSERT = SDLK_INSERT,
    DELETE = SDLK_DELETE,
    RIGHT = SDLK_RIGHT,
    LEFT = SDLK_LEFT,
    DOWN = SDLK_DOWN,
    UP = SDLK_UP,
    HOME = SDLK_HOME,
    END = SDLK_END,
    CAPS_LOCK = SDLK_CAPSLOCK,
    PRINT_SCREEN = SDLK_PRINTSCREEN,
    PAUSE = SDLK_PAUSE,
    F1 = SDLK_F1,
    F2 = SDLK_F2,
    F3 = SDLK_F3,
    F4 = SDLK_F4,
    F5 = SDLK_F5,
    F6 = SDLK_F6,
    F7 = SDLK_F7,
    F8 = SDLK_F8,
    F9 = SDLK_F9,
    F10 = SDLK_F10,
    F11 = SDLK_F11,
    F12 = SDLK_F12,
    NUMPAD_0 = SDLK_KP_0,
    NUMPAD_1 = SDLK_KP_1,
    NUMPAD_2 = SDLK_KP_2,
    NUMPAD_3 = SDLK_KP_3,
    NUMPAD_4 = SDLK_KP_4,
    NUMPAD_5 = SDLK_KP_5,
    NUMPAD_6 = SDLK_KP_6,
    NUMPAD_7 = SDLK_KP_7,
    NUMPAD_8 = SDLK_KP_8,
    NUMPAD_9 = SDLK_KP_9,
    LEFT_SHIFT = SDLK_LSHIFT,
    LEFT_CONTROL = SDLK_LCTRL,
    LEFT_ALT = SDLK_LALT,
    RIGHT_SHIFT = SDLK_RSHIFT,
    RIGHT_CONTROL = SDLK_RCTRL,
    RIGHT_ALT = SDLK_RALT,
};

/**
 * Converts keyboard button enum value to a string.
 *
 * @param button Keyboard button.
 *
 * @return Button name.
 */
inline std::string getKeyboardButtonName(KeyboardButton button) {
    return SDL_GetKeyName(static_cast<SDL_Keycode>(button));
}
