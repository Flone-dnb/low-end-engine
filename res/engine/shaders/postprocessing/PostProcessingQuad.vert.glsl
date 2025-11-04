/** Stores position in screen space in XY and UV in ZW. */
layout (location = 0) in vec4 posAndUv;

out vec2 fragmentUv;

/// Rendering full screen quad.
void main() {
    // Vertex positions were passed in normalized device coordinates.
    gl_Position = vec4(posAndUv.xy, 0.0F, 1.0F); 

    fragmentUv = posAndUv.zw;
}  
