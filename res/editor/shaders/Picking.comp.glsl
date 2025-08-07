#include "../../engine/shaders/Base.glsl"

// Group size same as in C++
layout(local_size_x = 16, local_size_y = 16) in;

layout(r32ui) uniform highp readonly uimage2D nodeIdTexture;

// Input.
uniform uvec2 textureSize;
uniform uvec2 cursorPosInPix;

// Output.
layout(std140, binding = 0) buffer BufferObject {
    // Node ID under the mouse cursor.
    uint iClickedNodeIdValue;
};

/// Entry.
void main() {
    if (gl_GlobalInvocationID.x >= textureSize.x || gl_GlobalInvocationID.y >= textureSize.y) {
        return;
    }

    if (cursorPosInPix.x != gl_GlobalInvocationID.x || cursorPosInPix.y != gl_GlobalInvocationID.y) {
        return;
    }

    iClickedNodeIdValue = imageLoad(nodeIdTexture, ivec2(gl_GlobalInvocationID.xy)).r;
}
