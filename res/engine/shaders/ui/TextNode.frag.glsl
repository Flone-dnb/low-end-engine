in vec2 fragmentUv;
out vec4 color;

/** Single-channel bitmap. */
uniform sampler2D glyphBitmap;
uniform vec4 textColor;

layout(early_fragment_tests) in;

/// Entry.
void main() {
    color = vec4(textColor.r, textColor.g, textColor.b, texture(glyphBitmap, fragmentUv).r * textColor.a);
}  
