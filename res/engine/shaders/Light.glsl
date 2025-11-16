// Macro values same as in C++ code, IF CHANGING also change in C++ code.
#define MAX_POINT_LIGHT_COUNT 16
#define MAX_SPOT_LIGHT_COUNT 16
#define MAX_DIRECTIONAL_LIGHT_COUNT 2

uniform vec3 ambientLightColor;

// ------------------------------------------------------------------------------------------------

/** Directional light description. */
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

// ------------------------------------------------------------------------------------------------

/** Spotlight description. */
struct Spotlight {
    /** Light position in world space. 4th component is not used. */
    vec4 position;

    /** Forward unit vector in the direction of the light source. 4th component is not used. */
    vec4 direction;

    /** Light color and 4th component stores intensity in range [0.0; 1.0]. */
    vec4 colorAndIntensity;

    /** Lit distance. */
    float distance;

    /**
     * Cosine of the spotlight's inner cone angle (cutoff).
     *
     * @remark Represents cosine of the cutoff angle on one side from the light direction
     * (not both sides), i.e. this is a cosine of value [0-90] degrees.
     */
    float cosInnerConeAngle;

    /**
     * Cosine of the spotlight's outer cone angle (cutoff).
     *
     * @remark Represents cosine of the cutoff angle on one side from the light direction
     * (not both sides), i.e. this is a cosine of value [0-90] degrees.
     */
    float cosOuterConeAngle;
};

/** Actual number of visible spotlights. */
uniform uint iSpotlightCount;

/** Uniform buffer object. */
layout (std140) uniform Spotlights {
    /** Visible spotlights. */
    Spotlight spotlights[MAX_SPOT_LIGHT_COUNT];
};

// ------------------------------------------------------------------------------------------------

/** Point light description. */
struct PointLight {
    /** Light position in world space. 4th component is not used. */
    vec4 position;

    /** Light color and 4th component stores intensity in range [0.0; 1.0]. */
    vec4 colorAndIntensity;

    /** Lit distance. */
    float distance;
};

/** Actual number of visible point lights. */
uniform uint iPointLightCount;

/** Uniform buffer object. */
layout (std140) uniform PointLights {
    /** Visible point lights. */
    PointLight pointLights[MAX_POINT_LIGHT_COUNT];
};

// ------------------------------------------------------------------------------------------------

/**
 * Calculates attenuation factor for a point/spot light sources.
 *
 * @param distanceToLightSource Distance between pixel/fragment and the light source.
 * @param lightIntensity        Light intensity, valid values range is [0.0F; 1.0F].
 * @param lightDistance         Lit distance.
 *
 * @return Factor in range [0.0; 1.0] where 0.0 means "no light is received" and 1.0 means "full light is received".
 */
float calculateLightAttenuation(float distanceToLightSource, float lightIntensity, float lightDistance) {
    // Use linear attenuation because it allows us to do sphere/cone intersection tests in light culling
    // more "efficient" in terms of light radius/distance to lit area. For example with linear attenuation if we have lit
    // distance 30 almost all this distance will be somewhat lit while with "inverse squared distance" function (and similar)
    // almost half of this distance will have pretty much no light.
    return clamp((lightDistance - distanceToLightSource) / lightDistance, 0.0F, 1.0F) * lightIntensity;
}

/**
 * Calculates light color that a fragment receives from all light sources.
 *
 * @param fragmentPosition     Position of the fragment in world space.
 * @param fragmentNormalUnit   Fragment's normal vector (normalized).
 * @param fragmentDiffuseColor Diffuse color of the fragment (vertex color or diffuse texture value).
 *
 * @return Light color.
 */
vec3 calculateColorFromLights(vec3 fragmentPosition, vec3 fragmentNormalUnit, vec3 fragmentDiffuseColor) {
    vec3 lightColor = ambientLightColor * fragmentDiffuseColor;

    // Apply directional lights.
    for (uint i = 0u; i < iDirectionalLightCount; i++) {
        // Calculate light attenuation.
        vec3 attenuatedLightColor = directionalLights[i].colorAndIntensity.rgb * directionalLights[i].colorAndIntensity.w;

        // Scale down light depending on the angle.
        float cosFragmentToLight = max(dot(fragmentNormalUnit, -directionalLights[i].direction.xyz), 0.0F);
        attenuatedLightColor *= cosFragmentToLight;

        // Done.
        lightColor += fragmentDiffuseColor * attenuatedLightColor;
    }

    // Apply spotlights.
    for (uint i = 0u; i < iSpotlightCount; i++) {
        // Calculate light attenuation.
        float fragmentDistanceToLight = length(spotlights[i].position.xyz - fragmentPosition);
        vec3 attenuatedLightColor
            = spotlights[i].colorAndIntensity.rgb * calculateLightAttenuation(
            fragmentDistanceToLight, spotlights[i].colorAndIntensity.w, spotlights[i].distance);

        // Determine how much of this fragment is inside of the light cone.
        vec3 fragmentToLightDirectionUnit = normalize(spotlights[i].position.xyz - fragmentPosition);
        float cosDirectionAngle = dot(fragmentToLightDirectionUnit, -spotlights[i].direction.xyz);

        // Calculate intensity based on inner/outer cone.
        float lightIntensity
            = clamp((cosDirectionAngle - spotlights[i].cosOuterConeAngle) / (spotlights[i].cosInnerConeAngle - spotlights[i].cosOuterConeAngle),
            0.0F, 1.0F);
        attenuatedLightColor *= lightIntensity;

        // Scale down light depending on the angle.
        float cosFragmentToLight = max(dot(fragmentNormalUnit, fragmentToLightDirectionUnit), 0.0F);
        attenuatedLightColor *= cosFragmentToLight;

        // Done.
        lightColor += fragmentDiffuseColor * attenuatedLightColor;
    }

    // Apply point lights.
    for (uint i = 0u; i < iPointLightCount; i++) {
        // Calculate light attenuation.
        float fragmentDistanceToLight = length(pointLights[i].position.xyz - fragmentPosition);
        vec3 attenuatedLightColor
            = pointLights[i].colorAndIntensity.rgb * calculateLightAttenuation(
            fragmentDistanceToLight, pointLights[i].colorAndIntensity.w, pointLights[i].distance);

        // Scale down light depending on the angle.
        vec3 fragmentToLightDirectionUnit = normalize(pointLights[i].position.xyz - fragmentPosition);
        float cosFragmentToLight = max(dot(fragmentNormalUnit, fragmentToLightDirectionUnit), 0.0F);
        attenuatedLightColor *= cosFragmentToLight;

        // Done.
        lightColor += fragmentDiffuseColor * attenuatedLightColor;
    }

    return lightColor;
}
