#version 450
layout(push_constant) uniform PC { mat4 model; vec4 color; } pc;
layout(location = 0) in vec3 fragNormal;
layout(location = 0) out vec4 outColor;
void main() {
    vec3 n = normalize(fragNormal);
    float diff = clamp(dot(n, normalize(vec3(1.0, 2.0, 1.5))), 0.0, 1.0) * 0.55 + 0.45;
    outColor = vec4(pc.color.rgb * diff, 1.0);
}
