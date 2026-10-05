#pragma once
#include "kit/haze_shaders.h"
#include "kit/disc_shaders.h"

// Full-screen passes for Osaka Jade (and the fog of the crossing).
namespace Journey::Shaders {


inline const char* osakaSky = R"(
uniform float u_energy;
void main() {
    vec2 p = design();
    float x = p.x, y = p.y;
    vec3 c0 = vec3(0.010, 0.022, 0.020), c1 = vec3(0.014, 0.070, 0.056);
    vec3 c2 = vec3(0.030, 0.230, 0.165), c3 = vec3(0.090, 0.520, 0.360);
    vec3 col = y < 260.0 ? mix(c0, c1, ss(0.0, 260.0, y)) : (y < 470.0 ? mix(c1, c2, ss(260.0, 470.0, y)) : mix(c2, c3, ss(470.0, 640.0, y)));
    float gy = y < 640.0 ? exp(-pow((640.0 - y) / 150.0, 2.0)) : 1.0;
    float gx = 0.30 + 0.70 * exp(-pow((x - (930.0 - u_cam * 0.05)) / 560.0, 2.0));
    float g = gy * gx * (0.80 + 0.25 * u_energy);
    col += vec3(0.42, 0.95, 0.68) * pow(g, 1.3) * 0.62;
    float m = cloudField(p);
    float cl = ss(0.47, 0.64, m) * ss(640.0, 380.0, y) * ss(20.0, 150.0, y);
    float lit = ss(0.47, 0.56, m) * (1.0 - ss(0.56, 0.72, m)) * ss(150.0, 560.0, y);
    col *= 1.0 - 0.62 * cl;
    col += lit * vec3(0.10, 0.50, 0.34) * 0.42;
    vec2 sp = p + vec2(u_cam * 0.01, 0.0);
    vec2 cell = floor(sp / 26.0);
    vec2 f = sp - cell * 26.0;
    float h = hash12(cell + 3.7);
    float dens = 0.34 * pow(1.0 - clamp(y / 340.0, 0.0, 1.0), 0.8);
    if (h < dens) {
        vec2 c = vec2(hash12(cell + 11.1), hash12(cell + 23.9)) * 22.0 + 2.0;
        float d = length(f - c);
        float tw = 0.65 + 0.35 * sin(u_t * (0.8 + 2.4 * hash12(cell + 5.0)) + h * 40.0);
        col += vec3(0.675, 0.831, 0.812) * (0.3 + 0.7 * hash12(cell + 41.0)) * exp(-d * d / 0.55) * (1.0 - cl) * tw * 1.3;
    }
    o = vec4(col, 1.0);
}
)";





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
