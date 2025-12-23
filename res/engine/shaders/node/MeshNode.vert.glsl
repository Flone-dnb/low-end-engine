layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 uv;

out vec3 fragmentPosition;
out vec3 fragmentNormal;
out vec2 fragmentUv;
out vec3 viewSpacePosition;

uniform mat4 worldMatrix;
uniform mat3 normalMatrix;
uniform mat4 viewMatrix;
uniform mat4 viewProjectionMatrix;
uniform float outlineWidth;

void main() {
    // Calculate position.
    vec4 posWorldSpace = worldMatrix * vec4(position + normalize(position) * outlineWidth, 1.0F);
    gl_Position = viewProjectionMatrix * posWorldSpace;

    viewSpacePosition = (viewMatrix * posWorldSpace).xyz;

    // Set output parameters.
    fragmentPosition = posWorldSpace.xyz;
    fragmentNormal = normalMatrix * normal;
    fragmentUv = uv;
} 
