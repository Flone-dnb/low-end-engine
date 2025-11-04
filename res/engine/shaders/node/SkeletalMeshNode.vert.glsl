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
    vec4 skinnedPosition = vec4(0.0F);
    vec4 skinnedNormal = vec4(0.0F);
    for (int i = 0; i < 4; i++) {
        uint iBoneIndex = boneIndices[i];
        float boneWeight = boneWeights[i];
        mat4 boneMatrix = vSkinningMatrices[iBoneIndex];

        // passing 0 as 4th component for position to avoid applying translation twice
        skinnedPosition += (boneMatrix * vec4(position, 0.0F)) * boneWeight;
        skinnedNormal += (boneMatrix * vec4(normal, 0.0F)) * boneWeight;
    }

    vec3 posModelSpace = skinnedPosition.xyz;
    vec3 normalModelSpace = skinnedNormal.xyz;

    // Calculate world position.
    vec4 posWorldSpace = worldMatrix * vec4(posModelSpace, 1.0F);
    gl_Position = viewProjectionMatrix * posWorldSpace;

    // Set output parameters.
    fragmentPosition = posWorldSpace.xyz;
    fragmentNormal = normalMatrix * normalModelSpace;
    fragmentUv = uv;
} 

