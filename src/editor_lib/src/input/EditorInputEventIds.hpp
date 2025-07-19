#pragma once

/** Stores unique IDs of input events. */
struct EditorInputEventIds {
    /** Groups action events. */
    enum class Action : unsigned char {
        CAPTURE_MOUSE_CURSOR = 0,
        INCREASE_CAMERA_MOVEMENT_SPEED,
        DECREASE_CAMERA_MOVEMENT_SPEED,
    };

    /** Groups axis events. */
    enum class Axis : unsigned char {
        MOVE_CAMERA_FORWARD = 0, //< Move editor's camera forward/back.
        MOVE_CAMERA_RIGHT,       //< Move editor's camera right/left.
        GAMEPAD_MOVE_CAMERA_FORWARD,
        GAMEPAD_MOVE_CAMERA_RIGHT,
        MOVE_CAMERA_UP, //< Move editor's camera up/down.
        GAMEPAD_LOOK_RIGHT,
        GAMEPAD_LOOK_UP
    };
};
