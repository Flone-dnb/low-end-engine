in vec2 fragmentUv;

layout(binding = 0) uniform sampler2D renderedColorTexture;
layout(binding = 1) uniform sampler2D depthTexture;

// Distance fog settings.
uniform bool bIsDistanceFogEnabled;
uniform vec3 distanceFogColor;
uniform vec2 distanceFogRange; // start and end pos as distance from camera in world units
uniform float fogHeightOnSky;

// Procedural sky settings.
uniform bool bIsSkyEnabled;
uniform vec3 skyColorAboveHorizon;
uniform vec3 skyColorOnHorizon;
uniform vec3 skyColorBelowHorizon;

uniform mat4 invProjMatrix;
uniform mat4 invViewMatrix;
uniform vec3 cameraDirection;

out vec4 color;

/**
 * Converts coordinates from screen space to view space.
 *
 * @param fragmentUv UV coordinates of a fullscreen quad in range [0; 1].
 * @param depth      Depth value in this coordinate.
 * 
 * @return Coordinates in the view space.
 */
vec3 convertScreenSpaceToViewSpace(vec2 fragmentUv, float depth) {
    vec4 ndcPos = vec4(
        fragmentUv.x * 2.0 - 1.0,
        fragmentUv.y * 2.0 - 1.0,
        depth * 2.0 - 1.0,
        1.0);

    vec4 viewPos = invProjMatrix * ndcPos;
    viewPos.xyz /= viewPos.w;

    return viewPos.xyz;
}

/**
 * Draws procedural sky.
 * 
 * @param cameraDirectionZ Camera's look direction (Z (height) component).
 *
 * @return Color of the sky.
 */
vec3 sky(float cameraDirectionZ) {
    float aboveHorizon = 1.0F - min(1.0F, 1.0F - cameraDirectionZ);
    float belowHorizon = 1.0F - min(1.0F, 1.0F + cameraDirectionZ);
    float horizon = 1.0F - aboveHorizon - belowHorizon;

    return vec3(
        skyColorAboveHorizon * aboveHorizon +
        skyColorOnHorizon * horizon +
        skyColorBelowHorizon * belowHorizon);
}

layout(early_fragment_tests) in;

/// Rendering full screen quad.
void main() {
    color = vec4(texture(renderedColorTexture, fragmentUv).rgb, 1.0F);

    float depth = texture(depthTexture, fragmentUv).r;

    if (bIsSkyEnabled && depth == 1.0F) {
        color.rgb = sky(cameraDirection.z);
    }

    if (bIsDistanceFogEnabled) {
        vec3 viewSpacePos = convertScreenSpaceToViewSpace(fragmentUv, depth);
        float dist = length(viewSpacePos);
        float skyPortion = 0.0F;
        if (bIsSkyEnabled) {
            vec4 worldSpacePos = invViewMatrix * vec4(viewSpacePos, 1.0F);
            skyPortion = smoothstep(0.0F, fogHeightOnSky, worldSpacePos.z);
        }
        float fogPortion = smoothstep(distanceFogRange.x, distanceFogRange.y, dist) * (1.0F - skyPortion);
        color.rgb = mix(color.rgb, distanceFogColor, fogPortion);
    }
}
