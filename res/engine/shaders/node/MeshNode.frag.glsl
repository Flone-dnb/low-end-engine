#include "../Light.glsl"

in vec3 fragmentPosition;
in vec3 fragmentNormal;
in vec2 fragmentUv;
in vec3 viewSpacePosition;

uniform vec4 diffuseColor;

uniform sampler2D diffuseTexture;
uniform vec2 textureTilingMultiplier; // stores -1 if diffuseTexture is not set
uniform vec2 textureUvOffset;

// Distance fog settings.
uniform vec3 distanceFogColor;
uniform vec2 distanceFogRange; // -1 if distance fog disabled

#ifdef ENGINE_EDITOR
    // Used for GPU picking.
    layout(r32ui) uniform highp uimage2D nodeIdTexture;
    uniform uint iNodeId;
#endif

out vec4 color;

#ifndef DISABLE_EARLY_Z
    layout(early_fragment_tests) in;
#endif
void main() {
    #ifdef ENGINE_EDITOR
        imageStore(nodeIdTexture, ivec2(gl_FragCoord.xy), uvec4(iNodeId, 0u, 0u, 0u));
    #endif

    // Normals may be unnormalized after the rasterization (when they are interpolated).
    vec3 fragmentNormalUnit = normalize(fragmentNormal);

    // Diffuse color.
    vec4 fragmentDiffuseColor = diffuseColor;
    bool bIsUsingDiffuseTexture = textureTilingMultiplier.x >= 0.0F;
    if (bIsUsingDiffuseTexture) {
        fragmentDiffuseColor *= texture(diffuseTexture, (fragmentUv + textureUvOffset) * textureTilingMultiplier);
    }

    // Light.
    vec3 lightColor = calculateColorFromLights(fragmentPosition, fragmentNormalUnit, fragmentDiffuseColor.rgb);

    // Distance fog.
    if (distanceFogRange.x >= 0.0F) {
        float fogPortion = smoothstep(distanceFogRange.x, distanceFogRange.y, length(viewSpacePosition));
        lightColor = mix(lightColor, distanceFogColor, fogPortion);
    }

    // Define this macro before including this file to add custom logic in a simple way:
    #ifdef CUSTOM_CODE
        CUSTOM_CODE
    #endif

    color = vec4(lightColor, fragmentDiffuseColor.a);
} 
