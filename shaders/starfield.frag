#version 450

layout(location = 0) in  vec3 worldDir;
layout(location = 0) out vec4 outColor;

const float PI = 3.14159265359;

// ---- Pseudo-random helpers -----------------------------------------------

float hash(vec2 p) {
    p = fract(p * vec2(234.12, 567.45));
    p += dot(p, p + 34.67);
    return fract(p.x * p.y);
}

// ---- Map a unit direction to spherical UV --------------------------------

vec2 sphereUV(vec3 d) {
    float phi   = atan(d.z, d.x) / (2.0 * PI);   // [-0.5, 0.5]
    float theta = asin(clamp(d.y, -1.0, 1.0)) / PI; // [-0.5, 0.5]
    return vec2(phi, theta);
}

// ---- One density layer of stars ------------------------------------------
// scale     : cells per hemisphere (more = denser stars)
// threshold : fraction of cells that are empty (higher = fewer stars)
// brightness: peak glow brightness

vec3 starLayer(vec3 dir, float scale, float threshold, float brightness) {
    vec2 uv   = sphereUV(dir) * scale;
    vec2 cell = floor(uv);
    vec2 fr   = fract(uv);
    vec3 col  = vec3(0.0);

    for (int dy = -1; dy <= 1; dy++) {
        for (int dx = -1; dx <= 1; dx++) {
            vec2 nb = cell + vec2(float(dx), float(dy));

            float r1 = hash(nb);
            if (r1 < threshold) continue; // empty cell

            float r2 = hash(nb + vec2(17.3,  3.1));  // sub-cell x offset
            float r3 = hash(nb + vec2(42.7, 91.1));  // sub-cell y offset
            float r4 = hash(nb + vec2( 5.1, 71.9));  // colour tint selector

            vec2  starPos = vec2(float(dx), float(dy)) + vec2(r2, r3) - fr;
            float dist    = length(starPos);
            float glow    = smoothstep(0.05, 0.0, dist) * brightness;

            // Mix between cool blue-white and warm yellow-white
            vec3 tint = mix(vec3(0.75, 0.87, 1.0), vec3(1.0, 0.95, 0.72), r4);
            col += tint * glow;
        }
    }
    return col;
}

// --------------------------------------------------------------------------

void main() {
    vec3 dir = normalize(worldDir);
    vec3 col = vec3(0.0);

    col += starLayer(dir, 250.0, 0.984, 0.75);  // dense, tiny, dim
    col += starLayer(dir, 120.0, 0.970, 1.00);  // medium density and brightness
    col += starLayer(dir,  50.0, 0.940, 1.50);  // sparse, large, bright

    outColor = vec4(clamp(col, 0.0, 1.0), 1.0);
}
