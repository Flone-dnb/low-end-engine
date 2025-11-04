// Group size same as in C++
layout(local_size_x = 16, local_size_y = 16) in;

layout(r32ui) uniform highp writeonly uimage2D nodeIdTexture;
uniform uvec2 textureSize;

/// Entry.
void main() {
    if (gl_GlobalInvocationID.x >= textureSize.x || gl_GlobalInvocationID.y >= textureSize.y) {
        return;
    }

    imageStore(nodeIdTexture, ivec2(gl_GlobalInvocationID.xy), uvec4(0u, 0u, 0u, 0u));
}
