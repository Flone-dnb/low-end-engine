layout(location = 0) in vec2 quadUv; // UVs in [0.0; 1.0]
layout(location = 1) in vec4 particleColorIn;
layout(location = 2) in vec4 particleWorldPosAndSize; // size in W

out vec4 particleColor;
out vec2 uv;

uniform mat4 viewMatrix;
uniform mat4 projectionMatrix;

struct Particle {
    vec4 color;
    vec4 worldPosAndSize;
};
layout (std140) uniform ParticleInstanceData {
    Particle particles[512]; // <- size same as in C++ 
};

void main() {
    particleColor = particles[gl_InstanceID].color;
    uv = vec2(quadUv.x, 1.0f - quadUv.y);

    vec4 particleViewPos = viewMatrix * vec4(particles[gl_InstanceID].worldPosAndSize.xyz, 1.0f);
    vec3 upDirection = vec3(viewMatrix * vec4(0.0f, 1.0f, 0.0f, 0.0f));
    vec3 rightDirection = normalize(cross(upDirection, -particleViewPos.xyz));

    float halfSize = particles[gl_InstanceID].worldPosAndSize.w / 2.0f;
    vec2 vertexOffset = quadUv - vec2(0.5f, 0.5f);
    vertexOffset *= halfSize;

    vec3 vertexViewPos = particleViewPos.xyz;
    vertexViewPos += vertexOffset.x * rightDirection;
    vertexViewPos += vertexOffset.y * upDirection;

    gl_Position = projectionMatrix * vec4(vertexViewPos, particleViewPos.w);
}
