#version 450

layout(push_constant) uniform PC { float aspect; } pc;

void main() {
    // 12 vertices for 2 quads (horizontal bar + vertical bar).
    // Values are in multiples of 0.003 NDC-y units so the scale step below
    // keeps the math readable.  arm = 10 * 0.003 = 0.030 NDC-y ≈ 16 px @ 1080p.
    const vec2 p[12] = vec2[12](
        // Horizontal bar
        vec2(-10.0, -1.0), vec2( 10.0, -1.0), vec2( 10.0,  1.0),
        vec2(-10.0, -1.0), vec2( 10.0,  1.0), vec2(-10.0,  1.0),
        // Vertical bar
        vec2( -1.0,-10.0), vec2(  1.0,-10.0), vec2(  1.0, 10.0),
        vec2( -1.0,-10.0), vec2(  1.0, 10.0), vec2( -1.0, 10.0)
    );

    vec2 v = p[gl_VertexIndex] * 0.003;
    v.x /= pc.aspect;  // equal pixel size on both axes
    gl_Position = vec4(v, 0.0, 1.0);
}
