#include "Base.glsl"

in vec3 fragmentPosition;
in vec3 fragmentNormal;
in vec2 fragmentUv;

uniform vec3 diffuseColor;

out vec4 color;

/// Entry point.
void main() {
    // Normals may be unnormalized after the rasterization (when they are interpolated).
    vec3 fragmentNormalUnit = normalize(fragmentNormal);

    color = vec4(diffuseColor, 1.0F);
} 
