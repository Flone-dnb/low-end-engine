#pragma once

// Standard.
#include <cstdint>

/**
 * Defines UI layers, this affects:
 * - rendering: layer1 is drawn first, layer2 is drawn on top of the layer1 and so on,
 * - input handling: layer2 receives input before layer1 and so on.
 */
enum class UiLayer : uint8_t {
    LAYER1 = 0,
    LAYER2,

    COUNT // mark the number of elements in this enum
};
