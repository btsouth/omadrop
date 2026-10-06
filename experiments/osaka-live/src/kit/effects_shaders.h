#pragma once
// Full-screen passes for Osaka Jade (and the fog of the crossing).
namespace Journey::Shaders {








inline const char* reflect = R"(
uniform sampler2D u_img;
uniform float u_y0, u_qx, u_t, u_gain, u_kick;
void main() {
    vec2 p = design();
    if (p.y < u_y0 || p.x > u_qx) { o = vec4(0.0); return; }
    float r = p.y - u_y0, n = 1080.0 - u_y0;
    // Broad, slow ripples from noise (no per-row zigzag); bass hits stir them.
    float rip = fbm(vec2(p.x / 150.0 + u_t * 0.04, r / 8.0 - u_t * 0.8), 3) - 0.5;
    float rip2 = fbm(vec2(p.x / 41.0 - u_t * 0.1, r / 3.0 - u_t * 1.7) + 7.0, 2) - 0.5;
    float wob = rip * (5.0 + r * 0.09) * (1.0 + 1.3 * u_kick) + rip2 * 2.0;
    float sy = u_y0 - r * 0.92 - 3.0;
    // Lights stretch into soft vertical streaks that lengthen with distance.
    float len = 5.0 + r * 0.30;
    vec3 acc = vec3(0.0);
    float wsum = 0.0;
    for (int k = -6; k <= 6; ++k) {
        float f = float(k) / 6.0;
        float w = exp(-f * f * 2.2);
        vec2 q = vec2(p.x + wob + f * 3.0, sy + f * len);
        acc += max(texture(u_img, vec2(q.x / 1920.0, 1.0 - q.y / 1080.0)).rgb - 0.08, 0.0) * w;
        wsum += w;
    }
    // Gentle breakup into ripple bands that drift toward the viewer.
    float phase = r * 0.5 + rip * 10.0 - u_t * 2.2;
    float bands = 0.80 + 0.20 * sin(phase) * exp(-0.25 * fwidth(phase) * fwidth(phase)) - 0.10 * u_kick * sin(r * 0.21 - u_t * 9.0);
    o = vec4(acc / wsum * bands * mix(0.66, 0.10, r / n) * u_gain, 0.0);
}
)";

inline const char* steam = R"(
uniform vec2 u_base;
uniform float u_t, u_amt, u_wind, u_puff;
void main() {
    vec2 p = design();
    float y = p.y;
    float n = fbm(vec2(p.x / 74.0, (y + u_t * 60.0) / 67.5), 4);
    float rise = 838.0 - y;
    float cx = u_base.x + 30.0 * sin(rise / 62.0 + u_t * 0.8) * ss(838.0, 600.0, y) + u_wind * rise * 0.45;
    float wd = (8.0 + rise * 0.17) * (1.0 + 0.35 * u_puff * ss(838.0, 700.0, y));
    float plume = exp(-pow((p.x - cx) / wd, 2.0)) * ss(842.0, 818.0, y) * ss(500.0, 790.0, y) * (0.3 + 1.0 * n);
    vec3 colr = vec3(0.99, 0.78, 0.42) * ss(700.0, 830.0, y) + vec3(0.6, 1.0, 0.82) * ss(830.0, 640.0, y);
    o = vec4(plume * colr * 0.40 * u_amt * (1.0 + 0.8 * u_puff), 0.0);
}
)";

inline const char* radialGlow = R"(
uniform vec2 u_c;
uniform float u_falloff, u_gain;
uniform vec3 u_col;
void main() {
    float d = length(design() - u_c);
    o = vec4(u_col * exp(-d / u_falloff) * u_gain, 0.0);
}
)";

inline const char* fog = R"(
uniform float u_amount, u_top, u_bottom, u_t;
uniform vec3 u_col;
void main() {
    vec2 p = design();
    float n = fbm(vec2((p.x + u_t * 20.0) / 384.0, p.y / 120.0) + vec2(4.0, 1.0), 5);
    float edge = u_top + (n - 0.5) * 150.0;
    float a = ss(-50.0, 70.0, p.y - edge) * u_amount * (0.88 + 0.12 * fbm(vec2((p.x + u_t * 35.0) / 213.0, p.y / 54.0), 4));
    float n2 = fbm(vec2((p.x - u_t * 26.0) / 300.0, p.y / 60.0) + vec2(9.0, 2.0), 4);
    float bottom = u_bottom + (n2 - 0.5) * 140.0;
    a *= 1.0 - ss(bottom - 80.0, bottom + 60.0, p.y);
    a = clamp(a, 0.0, 1.0);
    float fall = 0.55 + 0.45 * ss(1080.0, 560.0, p.y);
    o = vec4(a * u_col * 0.80 * fall, a);
}
)";
}
