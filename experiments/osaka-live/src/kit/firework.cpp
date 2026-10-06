#include "firework.h"
#include "palette.h"
#include "primitives.h"
#include "pane.h"
#include "haze.h"
#include "../rig.h"
#include "../osaka_shaders.h"
#include <cmath>
#include <vector>

namespace Journey::Kit {
using Life = OsakaEventState;
namespace {
V2 starPos(const Shell& sh, V2 dir, double speed, double age, double fx, double fy) {
    const double k = sh.kind == 3 ? 1.7 : 2.4;
    const double reach = (1 - std::exp(-k * age)) / (1 - std::exp(-k * 1.6));
    const double g = sh.kind == 3 ? 30 : 13;
    return {fx + dir.x * speed * reach, fy + dir.y * speed * reach + g * age * age * (0.55 + 0.45 * sh.size)};
}
}

void OsakaFireworkV1::draw(Ctx& c, const OsakaState& s, const Life& L) {
    GpuProfile::Group profileGroup(c.gpu.profile,"firework");
    if (L.surge.t < 0 || L.shells.empty()) return;
    const double t = c.t;
    Canvas& smoke = c.canvas();
    Canvas& cv = c.canvas();
    bool anySmoke = false;
    for (std::size_t si = 0; si < L.shells.size(); ++si) {
        const Shell& sh = L.shells[si];
        const double fx = sh.at.x - s.cam * 0.1, fy = sh.at.y;
        const double climb = si == 0 ? sh.burst - L.surge.t : 0.62;
        const double launch = sh.burst - climb;
        if (t < launch) continue;
        Rng rng(404 + si * 31);
        if (t < sh.burst + 0.05 && si != 1) {
            // Rocket: a hot head on a wobbling, sparking trail.
            const double x0 = fx - 24 + 48 * hash2(si, 2), y0 = 700;
            auto at = [&](double tt) {
                const double u = clamp01((tt - launch) / climb);
                const double e = easeOut(u);
                return V2(lerp(x0, fx, e) + 3 * std::sin(u * 17 + si), lerp(y0, fy, e));
            };
            for (int j = 0; j < 26; ++j) {
                const double tt = t - j * 0.016;
                if (tt < launch) break;
                const V2 q = at(tt);
                cv.disc(q.x, q.y, 2.0 * (1 - j / 26.0) + 0.4, mix(CREAM, AMBER, j / 26.0), (1 - j / 26.0) * 0.85);
            }
            for (int j = 0; j < 10; ++j) {
                const double born = t - 0.04 * j - 0.02 * hash2(j, si);
                if (born < launch) break;
                const V2 q = at(born) + V2(6 * (hash2(j + 3, si + std::floor(t * 20)) - 0.5), 6 * (t - born) * 30);
                cv.disc(q.x, q.y, 1.0, AMBER, 0.6 * (1 - j / 10.0));
            }
            const V2 h = at(t);
            cv.glow(h.x, h.y, 14, CREAM, 0.9);
            continue;
        }
        const double age = t - sh.burst;
        if (age < 0) continue;
        const double life = sh.kind == 3 ? 3.4 : sh.kind == 0 ? 2.7 : 1.8;
        const double R = 215 * sh.size;
        // Burst flash and the light it throws on the haze and the town.
        if (age < 0.16) cv.glow(fx, fy, 150 * sh.size * (1 - age / 0.16), CREAM, 1.0);
        const double shine = std::exp(-age * 2.2) * sh.strength;
        if (shine > 0.01) {
            Program& g = c.gpu.effect("radialGlow", Shaders::radialGlow);
            c.gpu.pass(g, Blend::Add, [&](Program& q) {
                q.set("u_c", float(fx), float(fy)); q.set("u_falloff", float(300 + 220 * sh.size));
                q.set("u_gain", float(0.30 * shine * sh.size)); q.set("u_col", mix(sh.a, GLOW, 0.35));
            });
        }
        // Smoke: a soft veil that holds the colour for a moment, then drifts.
        if (age < 6.0) {
            anySmoke = true;
            Rng sr(900 + si);
            for (int k = 0; k < 9; ++k) {
                const double a = Tau * k / 9 + sr.uni();
                const double r = R * (0.25 + 0.5 * sr.uni()) * (1 - std::exp(-age * 1.4));
                const double drift = age * (10 + 18 * L.wind);
                const double sx = fx + std::cos(a) * r + drift, sy = fy + std::sin(a) * r * 0.8 + age * 6;
                const double alpha = 0.16 * sstep(0, 0.4, age) * (1 - sstep(2.5, 6.0, age)) * sh.size;
                Col col = mix(Col(0.10f, 0.20f, 0.17f), sh.a * 0.55f, std::exp(-age * 1.6));
                for (const Shell& later : L.shells) {
                    const double dt = t - later.burst;
                    if (later.burst > sh.burst && dt >= 0 && dt < 1.3)
                        col = mix(col, later.a * 0.6f, 0.45 * std::exp(-dt * 3) * std::exp(-(V2(sx, sy) - later.at).len() / 380));
                }
                smoke.glowEllipse(sx, sy, 70 * sh.size + 30 * age, 46 * sh.size + 18 * age, col, alpha);
            }
        }
        if (age > life * 1.15) continue;
        const int n = sh.kind == 0 ? 96 : sh.kind == 3 ? 72 : sh.kind == 2 ? 54 : 64;
        const int trail = sh.kind == 0 || sh.kind == 3 ? 16 : 8;
        const double span = sh.kind == 0 ? 0.42 : sh.kind == 3 ? 0.75 : 0.16;
        for (int k = 0; k < n; ++k) {
            V2 dir;
            if (sh.kind == 2) {
                const double a = Tau * k / n + rng.normal() * 0.02;
                const double ca = std::cos(sh.tilt), sa = std::sin(sh.tilt);
                const V2 d0(std::cos(a), std::sin(a) * 0.45);
                dir = {d0.x * ca - d0.y * sa, d0.x * sa + d0.y * ca};
            } else {
                // Points on a sphere, projected: dense at the rim like a real shell.
                const double z = rng.uni() * 2 - 1, ph = rng.uni() * Tau, rr = std::sqrt(1 - z * z);
                dir = {rr * std::cos(ph), rr * std::sin(ph)};
            }
            const double speed = R * (0.88 + 0.2 * rng.uni());
            const double myLife = life * (0.8 + 0.3 * rng.uni());
            const double u = age / myLife;
            const double seedK = rng.uni();
            if (u >= 1) continue;
            const double fade = std::pow(1 - u, 1.4);
            const Col head = mix(mix(Col(1, 0.97f, 0.88f), sh.a, sstep(0.0, 0.18, u)), sh.b, sstep(0.45, 0.95, u));
            for (int j = trail; j >= 0; --j) {
                const double aj = age - span * j / trail;
                if (aj < 0) continue;
                const V2 q = starPos(sh, dir, speed, aj, fx, fy);
                const double f = double(j) / trail;
                const double tw = sh.kind == 3 ? 0.55 + 0.45 * hash2(k * 13 + j, std::floor(t * 24)) : 1.0;
                cv.disc(q.x, q.y, (2.4 * (1 - f) + 0.5) * (0.7 + 0.3 * sh.size), mix(head, sh.b, f * 0.7),
                        std::pow(1 - f, 1.6) * fade * tw);
            }
            const V2 q = starPos(sh, dir, speed, age, fx, fy);
            cv.glow(q.x, q.y, 9 * (0.7 + 0.3 * sh.size), head, fade * (0.8 + 0.2 * std::sin(t * 31 + k * 2.7)));
            // Crackle: the chrysanthemum's stars break into glitter as they die.
            if (sh.kind == 0 && u > 0.62) {
                for (int g = 0; g < 3; ++g) {
                    if (hash2(k * 7 + g, std::floor(t * 26)) < 0.45) continue;
                    const V2 o = q + V2(14 * (hash2(k, g) - 0.5), 14 * (hash2(g, k) - 0.5) + 6 * (u - 0.62));
                    cv.glow(o.x, o.y, 4, Col(1, 0.95f, 0.8f), 0.9 * (1 - u) / 0.38 * (0.5 + 0.5 * seedK));
                }
            }
        }
    }
    if (anySmoke) c.gpu.over(smoke, 1.0f, 18);
    if (!cv.empty()) {
        // Rasterise the live star geometry once for both the core and bloom.
        // The compositor's frame pool releases these layers on the next frame.
        const int stars = c.gpu.layer(cv);
        c.gpu.composite(stars, Blend::Over, 2.0f);
        const int bloom = c.gpu.blurred(stars, 26);
        c.gpu.composite(bloom, Blend::Add, 1.0f);
    }
}

}
