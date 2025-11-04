#pragma once

/** Stores unique IDs of input events. */
struct EditorInputEventIds {
    /** Groups action events. */
    enum class Action : unsigned char {
        CAPTURE_MOUSE_CURSOR = 0,
        GAMEPAD_TOGGLE_STATS,
        INCREASE_CAMERA_MOVEMENT_SPEED,
        DECREASE_CAMERA_MOVEMENT_SPEED,
    };

    /** Groups axis events. */
    enum class Axis : unsigned char {
        MOVE_CAMERA_FORWARD = 0,
        MOVE_CAMERA_RIGHT,
        MOVE_CAMERA_UP,
        GAMEPAD_MOVE_CAMERA_FORWARD,
        GAMEPAD_MOVE_CAMERA_RIGHT,
        GAMEPAD_MOVE_CAMERA_UP,
        GAMEPAD_MOVE_CAMERA_DOWN,
        GAMEPAD_LOOK_RIGHT,
        GAMEPAD_LOOK_UP,
    };
};
