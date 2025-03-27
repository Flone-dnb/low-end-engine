#include "../Base.glsl"

in vec2 uv;
out vec4 color;

/** Single-channel bitmap. */
uniform sampler2D glyphBitmap;

layout(early_fragment_tests) in;

/// Entry.
void main() {
    vec4 sampled = vec4(1.0F, 1.0F, 1.0F, texture(glyphBitmap, uv).r);
    color = sampled;
}  