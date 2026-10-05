#pragma once

namespace Journey::Shaders {
inline const char* cloud = R"(
uniform float u_cam, u_t;
float cloudField(vec2 p) {
    float n = fbm(vec2((p.x + u_cam * 0.03 + u_t * 3.0) / 560.0, p.y / 140.0) + vec2(3.1, 1.7), 5);
    float n2 = fbm(vec2((p.x + u_cam * 0.04 + u_t * 5.0) / 340.0, p.y / 70.0) + vec2(11.3, 4.2), 4);
    return 0.5 + (n * 0.7 + n2 * 0.3 - 0.5) * 1.75;
}
float cloudMask(vec2 p) {
    float m = cloudField(p);
    return ss(0.47, 0.64, m) * ss(640.0, 380.0, p.y) * ss(20.0, 150.0, p.y);
}
)";

inline const char* disc = R"(
uniform vec3 u_disc;      // x, y, r
uniform float u_energy, u_veil;
uniform vec4 u_rings[6];  // radius, alpha
uniform vec3 u_halo, u_col, u_col2, u_ring;
uniform vec4 u_haloK;
uniform float u_haloFar;
uniform float u_texAmt;
void main() {
    vec2 p = design();
    float d = length(p - u_disc.xy), r = u_disc.z, e = u_energy;
    float dd = max(d - r, 0.0);
    vec3 add = u_halo * (exp(-dd / (u_haloK.x + u_haloK.y * e)) * (u_haloK.z + u_haloK.w * e) + exp(-dd / 300.0) * u_haloFar);
    for (int k = 0; k < 6; ++k) add += u_ring * exp(-pow((d - u_rings[k].x) / 1.4, 2.0)) * u_rings[k].y;
    float m = ss(r + 1.2, r - 1.2, d);
    float tex = fbm((p - u_disc.xy) / 70.0 + vec2(5.3, 2.1), 4);
    float shade = 1.0 - u_texAmt * (0.09 * ss(0.48, 0.68, tex) + 0.10 * pow(ss(0.2, 1.0, d / r), 3.0));
    float veil = 1.0 - 0.14 * cloudMask(p) * u_veil;
    vec3 dc = mix(u_col, u_col2, ss(-u_disc.z, u_disc.z, p.y - u_disc.y));
    o = vec4(add * (1.0 - m) + m * dc * shade * veil, m);
}
)";
}
