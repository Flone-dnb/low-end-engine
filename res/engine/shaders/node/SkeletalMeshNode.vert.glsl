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
uniform mat4 vSkinningMatrices[MAX_BONE_COUNT_ALLOWED];

/// Entry point.
void main() {
    // up to 4 bones might affect a vertex
    mat4 finalSkinningMatrix = mat4(1.0F);
    for (int i = 0; i < 4; i++) {
        uint iBoneIndex = boneIndices[i];
        float boneWeight = boneWeights[i];
        mat4 boneMatrix = vSkinningMatrices[iBoneIndex];

        finalSkinningMatrix += boneMatrix * boneWeight;
    }

    vec3 posModelSpace = (finalSkinningMatrix * vec4(position, 1.0F)).xyz;
    vec3 normalModelSpace = (finalSkinningMatrix * vec4(normal, 0.0F)).xyz;

    // Calculate world position.
    vec4 posWorldSpace = worldMatrix * vec4(posModelSpace, 1.0F);
    gl_Position = viewProjectionMatrix * posWorldSpace;

    // Set output parameters.
    fragmentPosition = posWorldSpace.xyz;
    fragmentNormal = normalMatrix * normalModelSpace;
    fragmentUv = uv;
} 

