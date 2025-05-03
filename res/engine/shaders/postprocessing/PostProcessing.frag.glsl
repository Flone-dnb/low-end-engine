#include "../Base.glsl"

in vec2 fragmentUv;

layout(binding = 0) uniform sampler2D renderedColorTexture;
layout(binding = 1) uniform sampler2D depthTexture;

uniform bool bIsDistanceFogEnabled;
uniform vec3 distanceFogColor;
uniform vec2 distanceFogRange; // start and end pos as distance from camera in world units
uniform mat4 invProjMatrix;

out vec4 color;

/**
 * Converts depth to distance from the origin in view space.
 *
 * @param fragmentUv UV coordinates of a fullscreen quad in range [0; 1].
 * @param depth      Depth value in this coordinate.
 * 
 * @return Length of the view space vector.
 */
float convertDepthToViewSpaceDistance(vec2 fragmentUv, float depth) {
    vec4 toNdc = vec4(
        fragmentUv.x * 2.0 - 1.0,
        fragmentUv.y * 2.0 - 1.0,
        depth * 2.0 - 1.0,
        1.0);

    vec4 viewPos = invProjMatrix * toNdc;
    viewPos.xyz /= viewPos.w;

    return length(viewPos.xyz);
}

/// Rendering full screen quad.
void main() {
    color = vec4(texture(renderedColorTexture, fragmentUv).rgb, 1.0F);

    if (bIsDistanceFogEnabled) {
        float dist = convertDepthToViewSpaceDistance(fragmentUv, texture(depthTexture, fragmentUv).r);
        color.rgb = mix(color.rgb, distanceFogColor, smoothstep(distanceFogRange.x, distanceFogRange.y, dist));
    }
}