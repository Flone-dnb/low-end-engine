#include "../Base.glsl"
#include "../Light.glsl"

in vec3 fragmentPosition;
in vec3 fragmentNormal;
in vec2 fragmentUv;

uniform vec4 diffuseColor;

uniform bool bIsUsingDiffuseTexture;
layout(location = 0) uniform sampler2D diffuseTexture;

out vec4 color;

/// Entry point.
void main() {
    // Normals may be unnormalized after the rasterization (when they are interpolated).
    vec3 fragmentNormalUnit = normalize(fragmentNormal);

    // Diffuse color.
    vec4 fragmentDiffuseColor = diffuseColor;
    if (bIsUsingDiffuseTexture) {
        fragmentDiffuseColor *= texture(diffuseTexture, fragmentUv);
    }

    // Light.
    vec3 lightColor = calculateColorFromLights(fragmentPosition, fragmentNormalUnit, fragmentDiffuseColor.rgb);

    color = vec4(lightColor, fragmentDiffuseColor.a);
} 
