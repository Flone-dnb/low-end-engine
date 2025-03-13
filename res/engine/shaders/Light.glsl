// Macro values same as in C++ code, IF CHANGING also change in C++ code.
#define MAX_POINT_LIGHT_COUNT 50
#define MAX_SPOT_LIGHT_COUNT 30
#define MAX_DIRECTIONAL_LIGHT_COUNT 3
// ----------------------------------------------

struct DirectionalLight {
    /** Forward unit vector in the direction of the light source. 4th component is not used. */
    vec4 direction;

    /** Light color and 4th component stores intensity in range [0.0; 1.0]. */
    vec4 colorAndIntensity;
};

/** Actual number of visible directional lights. */
uniform uint iDirectionalLightCount;

/** Uniform buffer object. */
layout (std140) uniform DirectionalLights {
    /** Visible directional lights. */
    DirectionalLight directionalLights[MAX_DIRECTIONAL_LIGHT_COUNT];
};

/**
 * Calculates light color that a fragment receives from all light sources.
 *
 * @param fragmentNormalUnit   Fragment's normal vector (normalized).
 * @param fragmentDiffuseColor Diffuse color of the fragment (vertex color or diffuse texture value).
 *
 * @return Light color.
 */
vec3 calculateColorFromLights(vec3 fragmentNormalUnit, vec3 fragmentDiffuseColor) {
    vec3 lightColor = vec3(0.0F, 0.0F, 0.0F);

    // Apply directional lights.
    for (uint i = 0u; i < iDirectionalLightCount; i++) {
        vec3 attenuatedLightColor = directionalLights[i].colorAndIntensity.rgb * directionalLights[i].colorAndIntensity.w;

        float cosFragmentToLight = max(dot(fragmentNormalUnit, -directionalLights[i].direction.xyz), 0.0F);
        attenuatedLightColor *= cosFragmentToLight;

        lightColor += fragmentDiffuseColor * attenuatedLightColor;
    }

    return lightColor;
}
