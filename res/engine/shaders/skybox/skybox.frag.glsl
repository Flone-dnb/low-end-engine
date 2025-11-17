in vec3 skyboxSampleDir;
out vec4 color;

uniform bool bIsSkyboxCubemapSet;
uniform samplerCube skybox;

// Distance fog settings.
uniform vec3 fogColor;
uniform float fogHeightOnSky; // -1 if distance fog disabled

// A very simple procedural sky.
vec3 sky(float height) {
    vec3 colorAboveHorizon = vec3(0.35F, 0.55F, 1.0F);
    vec3 colorOnHorizon = vec3(0.5F, 0.7F, 1.0F);
    vec3 colorBelowHorizon = vec3(0.6F, 0.8F, 1.0F);

    float aboveHorizon = 1.0F - min(1.0F, 1.0F - height);
    float belowHorizon = 1.0F - min(1.0F, 1.0F + height);
    float horizon = 1.0F - aboveHorizon - belowHorizon;

    return vec3(
        colorAboveHorizon * aboveHorizon +
        colorOnHorizon * horizon +
        colorBelowHorizon * belowHorizon);
}

layout(early_fragment_tests) in;
void main() {
    vec3 normalizedDir = normalize(skyboxSampleDir);

    if (bIsSkyboxCubemapSet) {
        color = vec4(texture(skybox, skyboxSampleDir).rgb, 1.0F);
    }
    else {
        color = vec4(sky(normalizedDir.y), 1.0F);
    }

    if (fogHeightOnSky >= 0.0F) {
        float skyPortion = smoothstep(0.0F, fogHeightOnSky, max(normalizedDir.y, 0.0F));
        float fogPortion = 1.0F - skyPortion;
        color.rgb = mix(color.rgb, fogColor, fogPortion);
    }
}
