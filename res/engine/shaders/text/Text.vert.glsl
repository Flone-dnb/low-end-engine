#include "../Base.glsl"

/** Stores position in screen space in XY and UV in ZW. */
layout (location = 0) in vec4 vertex;

out vec2 uv;

/** Orthographic projection matrix. */
uniform mat4 projectionMatrix;

/// Entry.
void main() {
    gl_Position = projectionMatrix * vec4(vertex.xy, 0.0F, 1.0F);
    uv = vertex.zw;
}  