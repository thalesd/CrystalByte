#version 450

layout(push_constant) uniform PC {
    vec4  rect;   // xy = NDC top-left, zw = full bar size
    vec4  color;
    float fill;   // 0..1 fraction to draw
} pc;

void main() {
    float x0 = pc.rect.x;
    float y0 = pc.rect.y;
    float x1 = pc.rect.x + pc.rect.z * pc.fill;
    float y1 = pc.rect.y + pc.rect.w;

    // Two triangles forming the filled rectangle
    const vec2 corners[4] = vec2[4](
        vec2(x0, y0),
        vec2(x1, y0),
        vec2(x1, y1),
        vec2(x0, y1)
    );
    const int idx[6] = int[6](0, 1, 2, 0, 2, 3);
    gl_Position = vec4(corners[idx[gl_VertexIndex]], 0.0, 1.0);
}
