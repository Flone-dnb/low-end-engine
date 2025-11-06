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

#ifndef EDITOR_GIZMO
    // because gizmo modifies the depth
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
    if (bIsUsingDiffuseTexture) {
        fragmentDiffuseColor *= texture(diffuseTexture, fragmentUv * textureTilingMultiplier);
    }

    // Light.
    vec3 lightColor = calculateColorFromLights(fragmentPosition, fragmentNormalUnit, fragmentDiffuseColor.rgb);
    #ifdef EDITOR_GIZMO
        // TODO: because gizmo shader does not use lighting the CPU code that sets
        // normalMatrix and lighting info will fail because it's also not used and is optimized out. To avoid this:
        if (lightColor.r > 100.0F) {
            discard;
        }

        // Editor's gizmo mesh should not have lighting.
        lightColor = fragmentDiffuseColor.rgb;

        // Draw gizmo in front (it's simpler to do it this way rather than add a new feature to our MeshRenderer).
        gl_FragDepth = 0.0F;
    #endif

    color = vec4(lightColor, fragmentDiffuseColor.a);
} 
