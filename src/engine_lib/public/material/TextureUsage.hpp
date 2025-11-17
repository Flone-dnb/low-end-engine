#pragma once

// Standard.
#include <cstdint>

/** Describes usage of a texture. */
enum class TextureUsage : uint8_t {
    DIFFUSE, //< Texture is going to be used as a diffuse texture for a 3D object.
    UI,      //< Texture is going to be used in the user interface.
    CUBEMAP, //< Cubemap texture.
    OTHER,   //< If nothing from above.
};
