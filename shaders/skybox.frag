#version 450

layout(binding = 0) uniform UniformBufferObject {
    mat4 view;
    mat4 proj;
    mat4 invVP;
    vec4 camPos;
} ubo;

layout(location = 0) in  vec2 fragNDC;
layout(location = 0) out vec4 outColor;

uint hashU(uvec3 v) {
    v = v * 1664525u + uvec3(1013904223u);
    v.x += v.y * v.z;
    v.y += v.z * v.x;
    v.z += v.x * v.y;
    v ^= v >> 16u;
    v.x += v.y * v.z;
    v.y += v.z * v.x;
    v.z += v.x * v.y;
    return v.x ^ v.y ^ v.z;
}

float hashF(uvec3 v) {
    return float(hashU(v)) / 4294967295.0;
}

// Returns brightness contribution from stars at this grid scale.
// density  = fraction of grid cells that contain a star (0–1)
// cosThresh = cos(angular radius of each star dot)
float starLayer(vec3 dir, float scale, float density, float cosThresh) {
    vec3  p    = dir * scale;
    ivec3 cell = ivec3(floor(p));
    float brightness = 0.0;

    for (int dx = -1; dx <= 1; dx++)
    for (int dy = -1; dy <= 1; dy++)
    for (int dz = -1; dz <= 1; dz++) {
        ivec3 c  = cell + ivec3(dx, dy, dz);
        uvec3 uc = uvec3(c + ivec3(1000));

        if (hashF(uc) > density) continue;

        // Random offset within the cell to place the star
        float ox = hashF(uvec3(uc.x ^ 17u, uc.y,       uc.z      ));
        float oy = hashF(uvec3(uc.x,       uc.y ^ 31u, uc.z      ));
        float oz = hashF(uvec3(uc.x,       uc.y,       uc.z ^ 47u));
        vec3 starDir = normalize(vec3(c) + vec3(ox, oy, oz) - 0.5);

        float cosA = dot(dir, starDir);
        if (cosA > cosThresh) {
            float t   = (cosA - cosThresh) / (1.0 - cosThresh);
            float lum = hashF(uvec3(uc.x ^ 3u, uc.y ^ 7u, uc.z ^ 11u));
            brightness += t * t * mix(0.4, 1.0, lum);
        }
    }
    return brightness;
}

void main() {
    // Unproject NDC → world-space direction
    vec4 worldFar = ubo.invVP * vec4(fragNDC, 1.0, 1.0);
    vec3 dir = normalize(worldFar.xyz / worldFar.w - ubo.camPos.xyz);

    // Three layers: sparse bright stars, medium layer, dense faint layer
    float s  = starLayer(dir, 120.0, 0.030, 0.99985);
          s += starLayer(dir, 300.0, 0.050, 0.99990) * 0.55;
          s += starLayer(dir, 700.0, 0.080, 0.99995) * 0.25;

    s = clamp(s, 0.0, 1.0);

    // Slight blue-white tint on brighter stars
    vec3 starColor = mix(vec3(0.75, 0.88, 1.0), vec3(1.0), s);
    outColor = vec4(starColor * s, 1.0);
}
