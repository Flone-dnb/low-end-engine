vec3 gizmoFrag(vec3 lightColor, vec3 fragmentDiffuseColor) {
    // TODO: because gizmo shader does not use lighting the CPU code that sets
    // normalMatrix and lighting info will fail because it's also not used and is optimized out. To avoid this:
    if (lightColor.r > 100.0F) {
        discard;
    }

    // Editor's gizmo mesh should not have lighting.
    lightColor = fragmentDiffuseColor;

    // Draw gizmo in front (it's simpler to do it this way rather than add a new feature to our MeshRenderer).
    gl_FragDepth = 0.0F;

    return lightColor;
}

#define DISABLE_EARLY_Z // <- because we change depth
#define CUSTOM_CODE lightColor = gizmoFrag(lightColor, fragmentDiffuseColor.rgb);
#include "../../engine/shaders/node/MeshNode.frag.glsl"
