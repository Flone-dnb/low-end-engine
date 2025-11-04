#include "../Light.glsl"

#include "../vertex_format/MeshNodeVertexFormat.glsl"

uniform vec4 diffuseColor;

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
    #ifdef EDITOR_GIZMO
        // Editor's gizmo mesh should not have lighting.
        vec3 lightColor = fragmentDiffuseColor.rgb;

        // TODO: because gizmo shader does not use normal for lighting the CPU code that sets
        // normalMatrix will fail because it's also not used and is optimized out. To avoid this:
        if (fragmentNormalUnit.y > 100.0) {
            discard;
        }
    #else
        vec3 lightColor = calculateColorFromLights(fragmentPosition, fragmentNormalUnit, fragmentDiffuseColor.rgb);
    #endif

    color = vec4(lightColor, fragmentDiffuseColor.a);
} 
