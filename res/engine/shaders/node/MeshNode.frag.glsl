#include "../Base.glsl"
#include "../Light.glsl"

in vec3 fragmentPosition;
in vec3 fragmentNormal;
in vec2 fragmentUv;

uniform vec3 diffuseColor;

out vec4 color;

layout(early_fragment_tests) in;

/// Entry point.
void main() {
    // Normals may be unnormalized after the rasterization (when they are interpolated).
    vec3 fragmentNormalUnit = normalize(fragmentNormal);

    vec3 lightColor = calculateColorFromLights(fragmentPosition, fragmentNormalUnit, diffuseColor);

    color = vec4(lightColor, 1.0F);
} 
