#pragma once

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
}
