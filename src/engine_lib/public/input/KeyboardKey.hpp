#pragma once

// Standard.
#include <string>

// External.
#include "SDL_keycode.h"
#include "SDL_events.h"

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

/**
 * Mapping from SDL keyboard keys.
 *
 * @remark Also see @ref getKeyName.
 */
enum class KeyboardKey {
    KEY_UNKNOWN = SDLK_UNKNOWN,
    KEY_SPACE = SDLK_SPACE,
    KEY_COMMA = SDLK_COMMA,
    KEY_MINUS = SDLK_MINUS,
    KEY_PERIOD = SDLK_PERIOD,
    KEY_SLASH = SDLK_SLASH,
    KEY_0 = SDLK_0,
    KEY_1 = SDLK_1,
    KEY_2 = SDLK_2,
    KEY_3 = SDLK_3,
    KEY_4 = SDLK_4,
    KEY_5 = SDLK_5,
    KEY_6 = SDLK_6,
    KEY_7 = SDLK_7,
    KEY_8 = SDLK_8,
    KEY_9 = SDLK_9,
    KEY_SEMICOLON = SDLK_SEMICOLON,
    KEY_EQUALS = SDLK_EQUALS,
    KEY_A = SDLK_a,
    KEY_B = SDLK_b,
    KEY_C = SDLK_c,
    KEY_D = SDLK_d,
    KEY_E = SDLK_e,
    KEY_F = SDLK_f,
    KEY_G = SDLK_g,
    KEY_H = SDLK_h,
    KEY_I = SDLK_i,
    KEY_J = SDLK_j,
    KEY_K = SDLK_k,
    KEY_L = SDLK_l,
    KEY_M = SDLK_m,
    KEY_N = SDLK_n,
    KEY_O = SDLK_o,
    KEY_P = SDLK_p,
    KEY_Q = SDLK_q,
    KEY_R = SDLK_r,
    KEY_S = SDLK_s,
    KEY_T = SDLK_t,
    KEY_U = SDLK_u,
    KEY_V = SDLK_v,
    KEY_W = SDLK_w,
    KEY_X = SDLK_x,
    KEY_Y = SDLK_y,
    KEY_Z = SDLK_z,
    KEY_BACKSLASH = SDLK_BACKSLASH,
    KEY_ESCAPE = SDLK_ESCAPE,
    KEY_ENTER = SDLK_RETURN,
    KEY_TAB = SDLK_TAB,
    KEY_BACKSPACE = SDLK_BACKSPACE,
    KEY_INSERT = SDLK_INSERT,
    KEY_DELETE = SDLK_DELETE,
    KEY_RIGHT = SDLK_RIGHT,
    KEY_LEFT = SDLK_LEFT,
    KEY_DOWN = SDLK_DOWN,
    KEY_UP = SDLK_UP,
    KEY_HOME = SDLK_HOME,
    KEY_END = SDLK_END,
    KEY_CAPS_LOCK = SDLK_CAPSLOCK,
    KEY_PRINT_SCREEN = SDLK_PRINTSCREEN,
    KEY_PAUSE = SDLK_PAUSE,
    KEY_F1 = SDLK_F1,
    KEY_F2 = SDLK_F2,
    KEY_F3 = SDLK_F3,
    KEY_F4 = SDLK_F4,
    KEY_F5 = SDLK_F5,
    KEY_F6 = SDLK_F6,
    KEY_F7 = SDLK_F7,
    KEY_F8 = SDLK_F8,
    KEY_F9 = SDLK_F9,
    KEY_F10 = SDLK_F10,
    KEY_F11 = SDLK_F11,
    KEY_F12 = SDLK_F12,
    KEY_KP_0 = SDLK_KP_0,
    KEY_KP_1 = SDLK_KP_1,
    KEY_KP_2 = SDLK_KP_2,
    KEY_KP_3 = SDLK_KP_3,
    KEY_KP_4 = SDLK_KP_4,
    KEY_KP_5 = SDLK_KP_5,
    KEY_KP_6 = SDLK_KP_6,
    KEY_KP_7 = SDLK_KP_7,
    KEY_KP_8 = SDLK_KP_8,
    KEY_KP_9 = SDLK_KP_9,
    KEY_LEFT_SHIFT = SDLK_LSHIFT,
    KEY_LEFT_CONTROL = SDLK_LCTRL,
    KEY_LEFT_ALT = SDLK_LALT,
    KEY_RIGHT_SHIFT = SDLK_RSHIFT,
    KEY_RIGHT_CONTROL = SDLK_RCTRL,
    KEY_RIGHT_ALT = SDLK_RALT,
};

/**
 * Returns the UTF-8 encoded, layout-specific name of the key
 * or, in some rare cases, "?" string when we can't translate the key.
 *
 * @param key Keyboard key.
 *
 * @return Key name.
 */
inline std::string getKeyName(KeyboardKey key) { return SDL_GetKeyName(static_cast<SDL_Keycode>(key)); }
