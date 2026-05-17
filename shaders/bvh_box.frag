#version 450

layout(push_constant) uniform PC {
    vec4 bmin;
    vec4 bmax;
    vec4 color;
} pc;

layout(location = 0) out vec4 outColor;

void main() {
    outColor = pc.color;
}
