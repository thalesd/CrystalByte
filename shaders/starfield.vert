#version 450

layout(push_constant) uniform PC {
    mat4 invProj;
    mat4 invViewRot;
} pc;

layout(location = 0) out vec3 worldDir;

void main() {
    // Full-screen triangle — three oversized NDC vertices covering the entire viewport
    vec2 ndc;
    ndc.x = float((gl_VertexIndex & 1) << 2) - 1.0;
    ndc.y = float((gl_VertexIndex & 2) << 1) - 1.0;
    gl_Position = vec4(ndc, 1.0, 1.0);

    // Reconstruct a view-space direction at the far plane, then rotate to world space.
    // invProj already encodes the Vulkan Y-flip baked into the projection matrix.
    vec4 vp = pc.invProj * vec4(ndc, 1.0, 1.0);
    vp /= vp.w;
    worldDir = mat3(pc.invViewRot) * normalize(vp.xyz);
}
