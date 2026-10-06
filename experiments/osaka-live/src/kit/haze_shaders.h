#pragma once

namespace Journey::Shaders {
inline const char* band = R"(
uniform float u_y0, u_sigma, u_lo, u_hi, u_shift, u_seed, u_gain;
uniform vec3 u_col;
uniform vec2 u_noise;
void main() {
    vec2 p = design();
    float b = exp(-pow((p.y - u_y0) / u_sigma, 2.0));
    float n = u_hi > 0.0 ? fbm(vec2((p.x + u_shift) / u_noise.x, p.y / u_noise.y) + vec2(u_seed, u_seed * 0.7), 5) : 0.0;
    o = vec4(u_col * b * (u_lo + u_hi * n) * u_gain, 0.0);
}
)";
}
