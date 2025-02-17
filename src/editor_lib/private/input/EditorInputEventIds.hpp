#pragma once

/** Stores unique IDs of input events. */
struct EditorInputEventIds {
    /** Groups action events. */
    enum class Action : unsigned char {
        CLOSE,
    };
};
