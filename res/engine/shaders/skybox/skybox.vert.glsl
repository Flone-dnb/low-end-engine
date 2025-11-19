layout (location = 0) in vec3 position;

out vec3 skyboxSampleDir;

uniform mat4 viewProjectionMatrix;

void main() {
    gl_Position = viewProjectionMatrix * vec4(position, 0.0F);

    // passing `xyww` so that after perspective division the depth will be 1.0
    // and skybox will only be rendered for pixels where no other object was drawn
    gl_Position = gl_Position.xyww;

    skyboxSampleDir = position.xyz;
}
