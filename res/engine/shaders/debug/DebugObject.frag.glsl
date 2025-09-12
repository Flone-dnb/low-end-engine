#include "../Base.glsl"

uniform vec3 meshColor;

out vec4 outColor;

/// Entry point.
void main() {
    outColor = vec4(meshColor, 1.0F);
} 
