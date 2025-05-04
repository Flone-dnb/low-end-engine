#include "../Base.glsl"

in vec2 fragmentUv;

uniform vec4 color;
uniform bool bIsUsingTexture;
uniform sampler2D colorTexture;

out vec4 outColor;

/// Rendering screen quad.
void main() {
    outColor = color;

    if (bIsUsingTexture) {
        outColor *= texture(colorTexture, fragmentUv).rgba;
    }
}