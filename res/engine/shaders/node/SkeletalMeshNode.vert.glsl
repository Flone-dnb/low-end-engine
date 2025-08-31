#include "../Base.glsl"

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 uv;
layout (location = 3) in uvec4 boneIndices;
layout (location = 4) in vec4 boneWeights;

out vec3 fragmentPosition;
out vec3 fragmentNormal;
out vec2 fragmentUv;

// same as in C++ code
#define MAX_BONE_COUNT_ALLOWED 64

uniform mat4 worldMatrix;
uniform mat3 normalMatrix;
uniform mat4 viewProjectionMatrix;
uniform mat4 vBoneMatrices[MAX_BONE_COUNT_ALLOWED]; // they convert from local space to model space

/// Entry point.
void main() {
    vec4 posModelSpace = vec4(0.0F);
    vec3 normalModelSpace = vec3(0.0F);
    for (int i = 0; i < 4; i++) {
        uint iBoneIndex = boneIndices[i];
        posModelSpace += vBoneMatrices[iBoneIndex] * vec4(position, 1.0F) * boneWeights[i];
        normalModelSpace += mat3(vBoneMatrices[iBoneIndex]) * normal * boneWeights[i];
    }

    // Calculate world position.
    vec4 posWorldSpace = worldMatrix * vec4(posModelSpace.xyz, 1.0F);
    gl_Position = viewProjectionMatrix * posWorldSpace;

    // Set output parameters.
    fragmentPosition = posWorldSpace.xyz;
    fragmentNormal = normalMatrix * normalModelSpace;
    fragmentUv = uv;
} 
