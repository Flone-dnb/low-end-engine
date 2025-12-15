#pragma once

// Standard.
#include <cstdint>

/** Describes usage of a texture. */
enum class TextureUsage : uint8_t {
    DIFFUSE,           //< Texture is going to be used as a diffuse texture for a 3D object. Mipmaps will be
                       //auto-generated.
    UI,                //< Texture is going to be used in the user interface. No mipmaps generated.
    CUBEMAP_NO_MIPMAP, //< Cubemap texture. No mipmaps generated.
    OTHER_NO_MIPMAP,   //< If nothing from above. No mipmaps generated.
};
