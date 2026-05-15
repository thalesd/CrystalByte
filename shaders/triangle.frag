#version 450

layout(binding = 1) uniform sampler2D texSampler;

layout(location = 0) in  vec3 fragNormal;
layout(location = 1) in  vec2 fragTexCoord;
layout(location = 0) out vec4 outColor;

void main() {
    vec3 n = normalize(fragNormal);

    // Hemisphere ambient GI: smooth gradient from ground to sky based on surface facing.
    // t = 0 → facing down (ground bounce), t = 1 → facing up (sky light).
    float t           = dot(n, vec3(0.0, 1.0, 0.0)) * 0.5 + 0.5;
    vec3  skyColor    = vec3(0.55, 0.70, 1.00);   // cool blue-white sky
    vec3  groundColor = vec3(0.22, 0.18, 0.14);   // warm dark ground bounce
    vec3  ambient     = mix(groundColor, skyColor, t);

    vec3 albedo  = texture(texSampler, fragTexCoord).rgb;
    outColor     = vec4(albedo * ambient, 1.0);
}
