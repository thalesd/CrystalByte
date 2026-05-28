#version 450
layout(binding = 0) uniform UBO { mat4 view; mat4 proj; } ubo;
layout(push_constant) uniform PC { mat4 model; vec4 color; } pc;
layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;
layout(location = 0) out vec3 fragNormal;
void main() {
    gl_Position = ubo.proj * ubo.view * pc.model * vec4(inPos, 1.0);
    fragNormal = normalize(mat3(transpose(inverse(mat3(pc.model)))) * inNormal);
}
