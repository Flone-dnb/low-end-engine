#include "../Base.glsl"

in vec2 fragmentUv;

layout(binding = 0) uniform sampler2D renderedColorTexture;
layout(binding = 1) uniform sampler2D depthTexture;

uniform bool bIsDistanceFogEnabled;
uniform vec3 distanceFogColor;
uniform float distanceFogStartDistance; // in range [0.0; 1.0]
uniform float zNear;
uniform float zFar;

out vec4 color;

/**
 * Converts depth from depth texture (range [0; 1]) to a linear depth.
 *
 * @param depth Depth from texture.
 * @param zNear Camera's near clip plane.
 * @param zFar  Camera's far clip plane.
 *
 * @return Linear depth in range [0; 1].
 */
float convertDepthToLinear(float depth, float zNear, float zFar) {
    float zNdc = depth * 2.0F - 1.0F; // from [0; 1] to [-1; 1]
    float zView = (2.0F * zNear * zFar) / (zFar + zNear - zNdc * (zFar - zNear)); // zView in range [near; far]
    return zView / zFar;
}

/// Rendering full screen quad.
void main() {
    color = vec4(texture(renderedColorTexture, fragmentUv).rgb, 1.0F);

    if (bIsDistanceFogEnabled) {
        float depth = convertDepthToLinear(texture(depthTexture, fragmentUv).r, zNear, zFar);
        color.rgb = mix(color.rgb, distanceFogColor, max(0.0F, depth - distanceFogStartDistance) * (1.0F / (1.0F - distanceFogStartDistance)));
    }
}