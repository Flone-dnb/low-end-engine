/** Stores position in XY in [0.0; 1.0] (full screen quad) and UV in ZW. */
layout (location = 0) in vec4 vertex;

uniform vec2 screenPos;  // in pixels
uniform vec2 screenSize; // in pixels
uniform vec4 clipRect;   // [0.0; 1.0] where XY mark clip start and ZW mark clip size.

uniform vec2 windowSize; // in pixels

out vec2 fragmentUv;

void main() {
    vec2 quadScreenPos = vec2(screenPos + screenSize * clipRect.xy);
    vec2 quadScreenSize = vec2(screenSize * clipRect.zw);

    quadScreenPos.y = windowSize.y - quadScreenPos.y; // flip Y from our UI to OpenGL

    vec2 relativePos = vec2(
        (vertex.x *  quadScreenSize.x + quadScreenPos.x) / windowSize.x,
        (vertex.y * -quadScreenSize.y + quadScreenPos.y) / windowSize.y); // flip Y from our UI to OpenGL
    vec2 ndcPos = relativePos * 2.0F - 1.0F;

    gl_Position = vec4(ndcPos, 0.0F, 1.0F);
    fragmentUv = vertex.zw * clipRect.zw + clipRect.xy;
}
