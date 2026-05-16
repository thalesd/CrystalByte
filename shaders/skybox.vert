#version 450

layout(location = 0) out vec2 fragNDC;

void main() {
    vec2 positions[3] = vec2[3](
        vec2(-1.0, -1.0),
        vec2( 3.0, -1.0),
        vec2(-1.0,  3.0)
    );
    vec2 pos = positions[gl_VertexIndex];
    fragNDC = pos;
    // z = w = 1.0 places this at the far plane (depth = 1.0)
    gl_Position = vec4(pos, 1.0, 1.0);
}
