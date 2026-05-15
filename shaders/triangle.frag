#version 450

layout(binding = 1) uniform sampler2D texSampler;

layout(location = 0) in  vec3 fragNormal;
layout(location = 1) in  vec2 fragTexCoord;
layout(location = 2) in  vec4 fragBaseColor;
layout(location = 0) out vec4 outColor;

void main() {
    vec3 n = normalize(fragNormal);

    // Hemisphere ambient GI: sky above, warm ground below.
    float t           = dot(n, vec3(0.0, 1.0, 0.0)) * 0.5 + 0.5;
    vec3  skyColor    = vec3(0.55, 0.70, 1.00);
    vec3  groundColor = vec3(0.22, 0.18, 0.14);
    vec3  ambient     = mix(groundColor, skyColor, t);

    // Texture tints the base color (white fallback = solid base color).
    vec3 albedo = texture(texSampler, fragTexCoord).rgb * fragBaseColor.rgb;
    outColor    = vec4(albedo * ambient, 1.0);
}
