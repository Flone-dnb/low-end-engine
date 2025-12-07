in vec4 particleColor;
in vec2 uv;

uniform bool bIsUsingTexture;
uniform sampler2D particleTexture;

out vec4 color;

layout(early_fragment_tests) in;
void main() {
    color = particleColor;
    if (bIsUsingTexture) {
        color *= texture(particleTexture, uv);
    }
}
