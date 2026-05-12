#version 450

// Positions are in clip space: X and Y go from -1 to +1.
// Vulkan's Y axis points DOWN, so -0.5 is near the top.
vec2 positions[3] = vec2[](
    vec2( 0.0, -0.5),
    vec2( 0.5,  0.5),
    vec2(-0.5,  0.5)
);

layout(location = 0) out vec3 fragColor;

void main() {
    gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);
    fragColor   = vec3(0.0, 1.0, 0.0); // green
}
