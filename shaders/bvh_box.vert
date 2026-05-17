#version 450

layout(set = 0, binding = 0) uniform UBO {
    mat4 view;
    mat4 proj;
} ubo;

layout(push_constant) uniform PC {
    vec4 bmin;   // xyz = AABB min
    vec4 bmax;   // xyz = AABB max
    vec4 color;
} pc;

void main() {
    vec3 mn = pc.bmin.xyz;
    vec3 mx = pc.bmax.xyz;

    // 8 corners of the AABB
    vec3 corners[8] = vec3[8](
        vec3(mn.x, mn.y, mn.z), // 0
        vec3(mx.x, mn.y, mn.z), // 1
        vec3(mn.x, mx.y, mn.z), // 2
        vec3(mx.x, mx.y, mn.z), // 3
        vec3(mn.x, mn.y, mx.z), // 4
        vec3(mx.x, mn.y, mx.z), // 5
        vec3(mn.x, mx.y, mx.z), // 6
        vec3(mx.x, mx.y, mx.z)  // 7
    );

    // 12 edges as line-list pairs (24 vertices total)
    int edges[24] = int[24](
        0,1,  2,3,  4,5,  6,7,  // 4 edges along X
        0,2,  1,3,  4,6,  5,7,  // 4 edges along Y
        0,4,  1,5,  2,6,  3,7   // 4 edges along Z
    );

    vec3 pos = corners[edges[gl_VertexIndex]];
    gl_Position = ubo.proj * ubo.view * vec4(pos, 1.0);
}
