#include "../Base.glsl"
#include "../Light.glsl"

#include "../vertex_format/MeshNodeVertexFormat.glsl"

uniform vec4 diffuseColor;
uniform bool bIsAffectedByLightSources;

uniform bool bIsUsingDiffuseTexture;
layout(location = 0) uniform sampler2D diffuseTexture;
uniform vec2 textureTilingMultiplier;

#ifdef ENGINE_EDITOR
    // Used for GPU picking.
    layout(r32ui) uniform highp uimage2D nodeIdTexture;
    uniform uint iNodeId;
#endif

out vec4 color;

layout(early_fragment_tests) in;
void main() {
    #ifdef ENGINE_EDITOR
        imageStore(nodeIdTexture, ivec2(gl_FragCoord.xy), uvec4(iNodeId, 0u, 0u, 0u));
    #endif

    // Normals may be unnormalized after the rasterization (when they are interpolated).
    vec3 fragmentNormalUnit = normalize(fragmentNormal);

    // Diffuse color.
    vec4 fragmentDiffuseColor = diffuseColor;
    if (bIsUsingDiffuseTexture) {
        fragmentDiffuseColor *= texture(diffuseTexture, fragmentUv * textureTilingMultiplier);
    }

    // Light.
    vec3 lightColor = fragmentDiffuseColor.rgb;
    if (bIsAffectedByLightSources) {
        lightColor = calculateColorFromLights(fragmentPosition, fragmentNormalUnit, fragmentDiffuseColor.rgb);
    }

    color = vec4(lightColor, fragmentDiffuseColor.a);
} 
