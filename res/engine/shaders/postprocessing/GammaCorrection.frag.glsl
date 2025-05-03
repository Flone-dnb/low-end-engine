#include "../Base.glsl"

in vec2 fragmentUv;

layout(binding = 0) uniform sampler2D renderedColorTexture;

out vec4 color;

/// Rendering full screen quad.
void main() {
    color = vec4(texture(renderedColorTexture, fragmentUv).rgb, 1.0F);

    color.rgb = pow(color.rgb, vec3(1.0F/2.2F));
}