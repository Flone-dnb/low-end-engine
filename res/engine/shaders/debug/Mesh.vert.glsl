#include "../Base.glsl"

layout (location = 0) in vec3 posWorldSpace;

uniform mat4 worldMatrix;
uniform mat4 viewProjectionMatrix;

/// Entry point.
void main() {
    gl_Position = viewProjectionMatrix * worldMatrix * vec4(posWorldSpace, 1.0F);
} 
