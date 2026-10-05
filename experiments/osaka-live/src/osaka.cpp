// Osaka Jade: "Evening on the hill". A held stage that wakes up and comes
// alive: panes, a noodle cart, a lantern-bearer, shadow play behind paper,
// birds landing on the six wires, and a surge that bursts the flock and
// blooms one firework. Geometry and palette follow the approved mock.
#include "osaka_shaders.h"
#include "parts.h"
#include "rig.h"
#include "kit/neon.h"
#include "kit/city.h"
#include "kit/chime.h"
#include "kit/animals.h"
#include "kit/sky-lanterns.h"
#include "kit/festoon.h"
#include "kit/cloth.h"
#include "kit/onset.h"
#include "kit/lanterns.h"
#include "kit/rooms.h"
#include "kit/pane.h"
#include "kit/town.h"
#include "kit/layout.h"
#include "kit/ridges.h"
#include "kit/sky.h"
#include "kit/haze.h"
#include "kit/mountain.h"
#include "kit/disc.h"
#include "kit/primitives.h"

#include <cmath>
#include <vector>
#include <map>
#include <cstdio>
#include <cstdlib>

namespace Journey {
namespace {
using namespace Kit;
constexpr double QUAY = 2330.0;
const V2 FIREWORK(842, 418);


// ---------- timeline ----------
// One firework shell. The main pair blooms on the surge; the finale shells
// burst on the bass hits that follow it, so the sky answers the drop.
using Kit::Shell;

using Life = Kit::OsakaLegacyLife;


std::vector<Shell> shellPlan(const Ctx& c, const Life& L) {
    std::vector<Shell> out;
    if (L.surge.t < 0) return out;
    const Col GOLD(1.0f, 0.80f, 0.42f), EMBER(0.95f, 0.36f, 0.16f), PINK(0.98f, 0.45f, 0.72f);
    const Col JADEF(0.40f, 1.0f, 0.74f), ICE(0.70f, 0.95f, 1.0f);
    out.push_back({L.bloom, FIREWORK, 1.0 * L.scale, 0, GOLD, EMBER, 1.0, 0});
    if (!c.schedule->fullFireworkShow) return out;
    out.push_back({L.bloom + 0.26, FIREWORK + V2(-6, 10), 0.55 * L.scale, 1, JADEF, ICE, 0.8, 0});
    // Finale: up to four shells on the next bass hits before the glide.
    struct Slot { V2 at; double size; int kind; Col a, b; double tilt; };
    const Slot spots[4] = {{{640, 330}, 0.62, 1, PINK, MAG, 0},
                           {{1010, 372}, 0.66, 1, ICE, CYAN, 0.6},
                           {{730, 250}, 0.80, 3, GOLD, EMBER, 0},
                           {{960, 268}, 0.58, 1, CYAN, CREAM, 0}};
    for (std::size_t i=0; i<c.schedule->finale.size(); ++i) {
        const auto& e=c.schedule->finale[i];
        const Slot& sl=spots[i];
        out.push_back({e.t+0.62,sl.at,sl.size*L.scale,sl.kind,sl.a,sl.b,e.strength,sl.tilt+0.4*c.jit(600+i)});
    }
    return out;
}

Life lifeAt(const Ctx& c) {
    Life L;
    L.t = c.t;
    const double t = c.t;
    const double gust=window((c.schedule->action(Moment::Gust,t,0)),0,0.7,7.3,2.6);
    L.wind=0.12+0.06*fbm1(t*0.6,3)+gust*(0.85+0.15*fbm1(t*2.3,9));
    if(c.schedule->fireworks>=0 && t-c.schedule->fireworks<12) {
        L.surge={c.schedule->fireworks,c.schedule->fireworkStrength,false};
        L.scale=c.schedule->fullFireworkShow?1.0:0.35;
        L.bloom=L.surge.t+0.55;
        L.look=window(t,L.surge.t+0.3,0.45,L.surge.t+5.0,1.3);
    }
    // No predicted breakdown or timed surge. The calm breeze is always alive.
    L.hush=0;
    L.shells = shellPlan(c, L);
    return L;
}



// Panes switch on at authored times, then breathe with their band and answer
// the firework with a flash that travels outward from it.





// ---------- backdrop ----------

}  // namespace



namespace {
void band(Ctx& c, double y0, double sigma, double lo, double hi, double shift, double seed, Col col, double gain,
          V2 noise = {480, 108}) {
    hazeBand(c, y0, sigma, lo, hi, shift, seed, col, gain, noise);
}


}  // namespace

// The one mountain: a broad cone in Osaka Jade that recedes to Fuji at sea.

namespace {
// The city below the hill: hazy blocks with lit windows that come and go,
// streets of lamps, a few cars, and warning lights on the towers. Its
// windows sparkle with the treble.


void nearRidge(Ctx& c, const OsakaState& s) {
    GpuProfile::Group profileGroup(c.gpu.profile,"nearRidge");
    Canvas& cv = c.canvas();
    const auto& pts = c.points("near-ridge", [&] {
        std::vector<V2> points;
        for (double x = -20; x <= 1941; x += 3)
            points.push_back({x, ridgeY(27, x + s.cam * 0.2, 742, 46, 260)});
        return points;
    }, {s.cam});
    c.retain(cv, "near-ridge-base", [&](Canvas& cv) {
        cv.linear(0, 690, 0, 840, {{0, Col(0.022f, 0.13f, 0.096f), 1}, {1, Col(0.05f, 0.25f, 0.18f), 1}});
        cv.moveTo(-20, 1080);
        for (const auto& p : pts) cv.lineTo(p.x, p.y);
        cv.lineTo(1941, 1080);
        cv.closePath();
        cv.fill();
    }, {s.cam});
    // A mixed wood: cedars in stands, rounder broadleaf crowns, and gaps.
    const Col pine(0.022f, 0.13f, 0.096f), leaf(0.028f, 0.15f, 0.11f);
    for (std::size_t i = 0; i < pts.size(); i += 2) {
        const double wx = pts[i].x + s.cam * 0.2;
        const double stand = 0.5 + 0.5 * noise1(wx / 70, 3);
        if (stand < 0.22) continue;
        const double hk = hash2(std::floor(wx / 6), 17);
        if (hk > 0.45 + 0.5 * stand) continue;
        const double hgt = (10 + 26 * stand * (0.5 + 0.5 * hash2(wx, 4))) * (hk < 0.1 ? 1.4 : 1.0);
        const double sway = std::sin(c.t * 0.9 + wx * 0.02) * 0.6 * (hgt / 25);
        const V2 base = pts[i] + V2(0, 3);
        if (hash2(std::floor(wx / 23), 5) < 0.62) {
            const double w = hgt * (0.26 + 0.1 * hash2(wx, 6));
            cv.tri({base.x - w, base.y}, {base.x + sway, base.y - hgt}, {base.x + w, base.y}, pine);
            cv.tri({base.x - w * 0.8, base.y - hgt * 0.32}, {base.x + sway * 1.1, base.y - hgt * 1.12}, {base.x + w * 0.8, base.y - hgt * 0.32}, pine);
        } else {
            const double r = hgt * 0.32;
            cv.disc(base.x + sway * 0.5, base.y - r * 1.1, r, leaf);
            cv.disc(base.x - r * 0.7 + sway * 0.4, base.y - r * 0.6, r * 0.8, leaf);
            cv.disc(base.x + r * 0.75 + sway * 0.4, base.y - r * 0.7, r * 0.85, leaf);
        }
    }
    c.gpu.over(cv, 1, 0, float(s.land));
}

// A lit train crosses the valley below the ridge; sparks on bass hits.
void train(Ctx& c, const OsakaState& s) {
    GpuProfile::Group profileGroup(c.gpu.profile,"train");
    const double t = c.schedule->action(Moment::Train,c.t,9.8);
    if (t < 9.8 || t > 20.0) return;
    const double ox = -s.cam * 0.35;
    const double head = -360 + (t - 9.8) * 300 + ox;
    Canvas& cv = c.canvas();
    Canvas& l = c.canvas();
    const double y = 744;
    const Col body(0.016f, 0.085f, 0.064f);
    for (int car = 0; car < 5; ++car) {
        const double x1 = head - car * 106, x0 = x1 - 100;
        if (x1 < -20 || x0 > 1940) continue;
        cv.color(body);
        cv.moveTo(x0, y + 18); cv.lineTo(x0, y + 3); cv.quadTo(x0, y, x0 + 4, y);
        cv.lineTo(x1 - (car == 0 ? 14 : 4), y); cv.quadTo(x1, y + 2, x1, y + 12 + (car == 0 ? 0 : 6)); cv.lineTo(x1, y + 18);
        cv.closePath();
        cv.fill();
        cv.line(x0, y - 0.5, x1 - 6, y - 0.5, 1.0, MINT, 0.3);
        for (double wx = x0 + 7; wx < x1 - 12; wx += 12) {
            const double flick = 0.85 + 0.15 * std::sin(t * 6 + wx * 0.3);
            l.fillRect(wx, y + 5, 8, 6, WARM_B * float(flick), 0.95);
        }
        if (car == 2 && c.score) {
            const Event* e = Score::last(c.score->bassHits, c.t);
            if (e && c.t - e->t < 0.25) l.glow(x0 + 50, y - 6, 14, BBLUE, 1.0 - (c.t - e->t) / 0.25);
        }
        cv.line(x0 + 44, y, x0 + 56, y - 7, 1.0, body);
    }
    l.glow(head + 2, y + 10, 18, CREAM, 1.0);
    l.linear(head, y + 10, head + 260, y + 18, {{0, CREAM, 0.35f}, {1, CREAM, 0}});
    l.moveTo(head, y + 8); l.lineTo(head + 260, y - 6); l.lineTo(head + 260, y + 34); l.closePath();
    l.fill();
    l.glowEllipse(head - 240, y + 16, 300, 18, WARM_T, 0.22);
    c.gpu.over(cv);
    c.gpu.over(l, 1.4f);
    c.gpu.add(l, 0.5f, 14);
}

// After the firework, sky lanterns rise from the valley.


// Star position for a shell: drag-limited outward travel plus gravity, so
// stars race out, slow, then sag (the willow sags most).
V2 starPos(const Shell& sh, V2 dir, double speed, double age, double fx, double fy) {
    const double k = sh.kind == 3 ? 1.7 : 2.4;
    const double reach = (1 - std::exp(-k * age)) / (1 - std::exp(-k * 1.6));
    const double g = sh.kind == 3 ? 30 : 13;
    return {fx + dir.x * speed * reach, fy + dir.y * speed * reach + g * age * age * (0.55 + 0.45 * sh.size)};
}

void firework(Ctx& c, const OsakaState& s, const Life& L) {
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

// ---------- town ----------
void downhillRoofs(Ctx& c, const OsakaState& s, const Life& L) {
    GpuProfile::Group profileGroup(c.gpu.profile,"downhillRoofs");
    Rng rng(55);
    struct Row { double yb, sc, par; int n; Col wall, rf2; };
    const Row rows[3] = {{850, 0.40, 0.42, 10, Col(0.050f, 0.290f, 0.210f), Col(0.085f, 0.420f, 0.300f)},
                         {905, 0.56, 0.55, 8, Col(0.038f, 0.215f, 0.158f), Col(0.070f, 0.340f, 0.245f)},
                         {975, 0.76, 0.70, 6, Col(0.026f, 0.150f, 0.110f), Col(0.055f, 0.265f, 0.192f)}};
    for (int row = 0; row < 3; ++row) {
        const Row& R = rows[row];
        Canvas& cv = c.canvas();
        Canvas& cw = c.canvas();
        Canvas* staticRoofs = c.retainedBuilder(cv, "downhill-roofs-" + std::to_string(row), {s.cam});
        const Col rf = mix(R.wall, R.rf2, 0.3);
        double x = 540 - s.cam * R.par + rng.uni() * 40;
        for (int i = 0; i < R.n; ++i) {
            const double w = (150 + rng.uni() * 130) * R.sc, hgt = (120 + rng.uni() * 70) * R.sc;
            const double ye = R.yb - hgt + rng.normal() * 8, yr = ye - (36 + rng.uni() * 26) * R.sc;
            const bool antenna = rng.uni() < 0.5;
            if (staticRoofs) {
                staticRoofs->fillRect(x, ye, w, 400, R.wall);
                roof(*staticRoofs, x, x + w, ye, yr, 16 * R.sc, rf, R.rf2, MINT, 5 * R.sc, true, 0.30);
                if (antenna) {
                    staticRoofs->line(x + w * 0.3, yr, x + w * 0.3, yr - 30 * R.sc, 1.2, R.wall);
                    staticRoofs->line(x + w * 0.3 - 9 * R.sc, yr - 24 * R.sc, x + w * 0.3 + 9 * R.sc, yr - 24 * R.sc, 1.2, R.wall);
                }
            }
            const int nw = int(1 + rng.uni() * 3);
            for (int j = 0; j < nw; ++j) {
                const double r1 = rng.uni(), r2 = rng.uni(), r3 = rng.uni(), r4 = rng.uni();
                const bool tinted = rng.uni() >= 0.8;
                if (r1 >= 0.82) continue;
                const double wx = x + w * (0.12 + 0.6 * r2), wy = ye + (14 + r3 * 26) * R.sc;
                const double ww = (26 + r4 * 22) * R.sc, wh = 26 * R.sc;
                const int key = row * 100 + i * 7 + j;
                // Windows wake through the first twelve seconds; a few were already lit.
                const double h = hash2(key, 5.5 + c.seed);
                const double on = h < 0.14 ? -100 : 2.5 + 10.5 * hash2(key, 9.1 + c.seed);
                const double level = s.chapter || on < 0 ? paneLevel(c, L, on, key % 6, {wx, wy}) : 1.0;
                warmPane(cw, wx, wy, ww, wh, level, tinted ? &BCYAN : nullptr);
                if (level > 0.01) c.retain(cw, "downhill-lattice-" + std::to_string(key), [&](Canvas& cw) {
                    lattice(cw, wx, wy, ww, wh, 2, 2, R.wall, 1.0);
                }, {s.cam});
            }
            x += w + (8 + rng.uni() * 46) * R.sc;
        }
        c.gpu.over(cv, 1, 0, float(s.land));
        c.gpu.over(cw, 1.25f, 0, float(s.land));
        band(c, R.yb + 2, 26, 0.16, 0, 0, 0, Col(0.3f, 0.9f, 0.62f), s.land);
    }
}

void rightHouses(Ctx& c, const OsakaState& s, const Life& L) {
    GpuProfile::Group profileGroup(c.gpu.profile,"rightHouses");
    const double t = c.t, ox = -s.cam * 0.9;
    Canvas& cv = c.canvas();
    Canvas& w = c.canvas();
    const Col wall(0.014f, 0.046f, 0.037f), rf(0.018f, 0.070f, 0.054f), rf2(0.050f, 0.215f, 0.160f);
    double x0 = 1262 + ox;
    Kit::OsakaRightHouse2V1::draw(c, s, cv, x0, wall, rf, rf2);
    Kit::OsakaRightRoom2V1::draw(c, s, L, w, x0);
    x0 = 1512 + ox;
    Kit::OsakaRightHouse3V1::draw(c, s, cv, x0, wall, rf, rf2);
    const auto& ups = Kit::upperPanes;
    double shamisenPane = 0;
    Kit::OsakaRightRoom3V1::draw(c, s, L, w, t, x0, ups, shamisenPane);
    c.gpu.over(cv);
    c.gpu.over(w, 1.22f);
    // Shamisen player behind the paper from 17 s, strumming on mid-band peaks.
    if (shamisenPane > 0.05 && t > 16.6) {
        Canvas& sh = c.canvas();
        const double appear = sstep(16.8, 17.7, t);
        const double fx = x0 + 112, baseY = 792 + (1 - easeOut(appear)) * 60;
        double strum = 0;
        if (c.score) {
            const Event* e = Score::last(c.score->midPeaks, t);
            double et = e ? e->t : -10;
            // Fallback rhythm keeps her playing in silence or sparse passages.
            const double grid = 17.9 + std::floor((t - 17.9) / 0.62) * 0.62;
            if (t - et > 0.9 && t > 17.9) et = grid;
            const double age = t - et;
            if (age >= 0 && age < 0.45 && et > 17.6) strum = age < 0.07 ? age / 0.07 : 1 - easeOut((age - 0.07) / 0.38);
        } else if (t > 17.9) {
            const double age = std::fmod(t - 17.9, 0.62);
            strum = age < 0.07 ? age / 0.07 : 1 - easeOut((age - 0.07) / 0.38);
        }
        RigIn r;
        r.h = 160; r.facing = -1; r.obi = true; r.flutter = L.wind * std::sin(t * 2.1); r.robe = true; r.bun = true; r.lean = 0.08 + 0.03 * std::sin(t * 2.1);
        r.hip = {fx, baseY - hipHeight(160) + 18};
        r.footF = {fx - 30, baseY}; r.footB = {fx - 20, baseY};
        r.handF = {fx - 60, baseY - 120 + 4 * std::sin(t * 1.3)};
        r.handB = {fx + 18 - 10 * strum, baseY - 92 + 16 * strum};
        r.headTilt = -0.15 + 0.08 * std::sin(t * 2.2) - L.look * 0.0;
        drawBody(sh, solve(r), SHADOW);
        sh.line(fx + 24, baseY - 70, fx - 86, baseY - 124, 4.5, SHADOW);
        sh.fillRect(fx + 8, baseY - 88, 34, 30, SHADOW);
        Canvas& mask = c.canvas();
    Kit::OsakaShamisenMaskV1::draw(c, s, mask, x0);
        const int lt = c.gpu.layer(sh);
        const int bl = c.gpu.blurred(lt, 2.0);
        const int mk = c.gpu.layer(mask);
        Program& m = c.gpu.effect("masked", R"(
uniform sampler2D u_tex, u_mask;
uniform float u_opacity;
void main() { o = texture(u_tex, v_uv) * texture(u_mask, v_uv).a * u_opacity; }
)");
        c.gpu.pass(m, Blend::Over, [&](Program& q) {
            c.gpu.bindTexture(0, bl, q, "u_tex");
            c.gpu.bindTexture(1, mk, q, "u_mask");
            q.set("u_opacity", float(0.85 * appear * std::min(1.0, shamisenPane)));
        });
    }
    Canvas& f = c.canvas();
    c.retain(f, "izakaya-lattice", [&](Canvas& f) {
        for (int i = 0; i < 4; ++i) lattice(f, x0 + ups[i].wx, 640, ups[i].ww, 96, ups[i].ww > 70 ? 3 : 2, 3, INK, 1.3);
    }, {s.cam});
    const double wind = L.wind;
    OsakaIzakayaClothV1::draw(L, f, t, x0);
    c.retain(f, "izakaya-counter", [&](Canvas& f) {
        f.line(x0 + 58, 904, x0 + 252, 904, 5, INK);
    }, {s.cam});
    // Patrons: lean, gesture and drink, each on their own clock.
    for (int i = 0; i < 3; ++i) {
        const double px = x0 + (i == 0 ? 98 : i == 1 ? 152 : 214);
        const double face = i == 1 ? -1 : 1;
        const double ph = t * (0.7 + 0.13 * i) + i * 2.1;
        // A shared toast starts on the first strong onset in this phrase.
        const double toastAt=c.schedule->moments[int(Moment::Toast)].start;
        const double toastAge=c.schedule->action(Moment::Toast,t,0);
        const double order=c.schedule->parameter(Moment::Toast,1,0,2,0);
        const double delay=std::fmod(i+std::floor(order),3.0)*0.08;
        const double toast=c.schedule->moments[int(Moment::Toast)].cycle<=1
            ? window(t,toastAt+i*0.08,0.24,toastAt+0.7,0.35)
            : window(toastAge,delay,0.24,0.7,0.35);
        const double drink = std::max(toast, std::pow(std::max(0.0, std::sin(ph)), 6) * (1 - L.hush));
        const double laugh = std::max(0.0, std::sin(t * 0.43 + i * 1.9)) * 0.08;
        RigIn r;
        r.h = 104; r.facing = face;
        r.hair = i == 1 ? Hair::Ponytail : Hair::Short;
        r.garment = i == 2 ? Garment::Happi : Garment::Jacket;
        r.flutter = 0.25 * std::sin(t * 2 + i) + L.wind * 0.5;
        r.sleeve = i == 1; r.obi = i == 1;
        r.hip = {px, 902 - 6};
        r.lean = 0.05 + laugh + 0.05 * std::sin(ph * 0.5) + 0.10 * window(t, toastAt + 1, 0.3, toastAt + 1.6, 0.5);
        r.footF = {px + face * 22, 902 + 22}; r.footB = {px + face * 16, 902 + 24};
        r.handF = {px + face * (18 + 6 * drink), 902 - 30 - 28 * drink};
        r.handB = {px + face * 14, 902 - 22};
        r.headTilt = 0.12 * drink + L.look * 0.5;
        drawBody(f, solve(r), INK);
        if (drink > 0.02) f.fillRect(px + face * (20 + 6 * drink) - 3, 902 - 40 - 28 * drink, 6, 9, INK);
    }
    // Laundry on the valley side of H2: lifts and snaps in the gust.
    const double bx = 1262 + ox;
    c.retain(f, "laundry-line", [&](Canvas& f) {
        f.line(bx - 6, 788, bx + 150, 788, 1.2, INK);
    }, {s.cam});
    for (int i = 0; i < 5; ++i) {
        const double sw = std::sin(t * 1.6 + i) * 3;
        const double lift = wind * (24 + 8 * std::sin(t * 9 + i * 2.2));
        const double hang = 30 + (i % 2) * 10;
        const double a0x = bx + 8 + i * 28, a1x = bx + 28 + i * 28;
        f.color(mix(INK2, RIM, 0.3));
        f.moveTo(a0x, 788); f.lineTo(a1x, 788);
        f.lineTo(a1x + 2 + sw + lift, 788 + hang - lift * 0.8);
        f.lineTo(a0x - 2 + sw + lift * 0.9 + 3 * std::sin(t * 11 + i), 788 + hang - lift * 0.7);
        f.closePath();
        f.fill();
    }
    Canvas& n = c.canvas();
    double stutter;
    Kit::OsakaNeonV1::draw(c, s, f, n, t, x0, stutter);
    for (int i = 0; i < 7; ++i) {
        const double bulb = 0.65 + 0.55 * c.lift(3 + i % 3) * (0.6 + 0.4 * hash2(i, 3));
        n.glow(x0 - 26 + i * 44, 798 + std::sin(t * 1.3 + i) * 1.5 + wind * 2 * std::sin(t * 5 + i), 9, i % 2 ? RED : WARM_T, std::min(1.0, bulb));
    }
    c.gpu.over(f);
    const double neon = Kit::OsakaNeonV1::level(c, t, stutter);
    Kit::OsakaNeonV1::submit(c, n, neon);
}

void streetSurface(Ctx& c, const OsakaState& s) {
    GpuProfile::Group profileGroup(c.gpu.profile,"streetSurface");
    const double ox = -s.cam * 0.85;
    Canvas& cv = c.canvas();
    const double qx = QUAY - s.cam * 0.85;
    Kit::OsakaStreetV1::draw(c, s, cv, qx);
    if (qx < 1930 && s.harbour > 0.01) {
        // Harbour water beyond the quay.
        if (s.harbourFeather > 0) {
            cv.linear(0, 934, 0, 1080, {{0, Col(0.030f, 0.120f, 0.100f), float(s.harbour * (1 - s.harbourFeather))},
                {0.15f, Col(0.026f, 0.103f, 0.085f), float(s.harbour)}, {1, Col(0.006f, 0.022f, 0.022f), float(s.harbour)}});
        } else {
            cv.linear(0, 934, 0, 1080, {{0, Col(0.030f, 0.120f, 0.100f), float(s.harbour)}, {1, Col(0.006f, 0.022f, 0.022f), float(s.harbour)}});
        }
        cv.rect(qx, 934, 1930 - qx, 146);
        cv.fill();
        for (int k = 0; k < 26; ++k) {
            const double y = 944 + k * 5.2 + 3 * std::sin(c.t * 0.9 + k);
            const double x = qx + std::fmod(k * 173.3 + c.t * (14 + k % 5 * 3), std::max(60.0, 1930 - qx));
            cv.line(x, y, x + 30 + k * 2, y, 1.2, MINT, (0.10 + 0.012 * k) * s.harbour);
        }
    }
    Kit::OsakaRailingV1::draw(c, s, cv, qx, ox);
    c.gpu.over(cv);
}

// ---------- people of the street ----------
// Walk schedules: distance travelled as a function of time, with eased
// starts and stops so feet plant and bodies settle.
double easedDistance(double t, double t0, double t1, double d0, double d1) {
    if (t <= t0) return d0;
    if (t >= t1) return d1;
    const double u = (t - t0) / (t1 - t0);
    // Accelerate over the first 12%, cruise, decelerate over the last 15%.
    const double a = 0.12, b = 0.15;
    const double v = 1.0 / (1 - a / 2 - b / 2);
    double s;
    if (u < a) s = v * u * u / (2 * a);
    else if (u < 1 - b) s = v * (a / 2 + (u - a));
    else { const double r = 1 - u; s = 1 - v * r * r / (2 * b); }
    return lerp(d0, d1, s);
}
double speedOf(double t, double t0, double t1, double d0, double d1) {
    return (easedDistance(t + 0.02, t0, t1, d0, d1) - easedDistance(t - 0.02, t0, t1, d0, d1)) / 0.04;
}

struct Walker { double x; double dist; double motion; };

// Lantern-bearer remains in the street. No boat departure in live Osaka.
Walker bearerAt(double) { return {800,330,0}; }

// A paper chochin on a short pole: ribbed, capped, lit warm from within.




// A little reflected lantern light on the street-facing cloth. Keep the
// silhouette, but let a coat front / obi separate at small viewing sizes.


void streetFront(Ctx& c, const OsakaState& s, const Life& L) {
    GpuProfile::Group profileGroup(c.gpu.profile,"streetFront");
    const double t = c.t, ox = -s.cam * 0.95;
    Canvas& p = c.canvas();
    Canvas& l = c.canvas();
    // Lantern-bearer, with a straw hat so they read in every world.
    const Walker w = bearerAt(t);
    if (w.x + ox > -150 && w.x + ox < 2100) {
        // Walk in the foreground lane, then merge back toward the quay.
        const double h = 146, gy = 1008;
        Gait g;
        g.stride = 0.72 * h; g.lift = 0.07 * h; g.bob = 0.018 * h;
        const Steps st = gaitAt(470 + ox, w.dist, gy - 0.04 * h, 1, g, w.motion);
        RigIn r;
        r.h = h; r.facing = 1; r.hat = true; r.garment = Garment::Jacket; r.flutter = L.wind * std::sin(t * 2.6) + 0.3 * std::sin(Tau * st.phase) * w.motion;
        r.hip = {w.x + ox, gy - hipHeight(h) + st.hipBob - 0.012 * h * w.motion};
        r.lean = 0.06 * w.motion + 0.02;
        r.footF = st.footF; r.footB = st.footB;
        const double arm = std::sin(Tau * st.phase) * w.motion;
        r.handF = r.hip + V2(0.24 * h, -0.17 * h + 0.01 * h * arm);
        r.handB = r.hip + V2(-0.07 * h - 0.09 * h * arm, 0.06 * h);
        // Watches the birds land, nods to the cook, watches the cyclist pass,
        // holds his hat in the gust, looks up at the firework.
        const double birds = c.gesture(9.5, 0.6, 11.9, 0.5);
        const double nod=0.22*(c.gesture(16.1,0.18,16.46,0.18)+c.gesture(16.7,0.18,17.06,0.18));
        const double watch = c.gesture(17.4, 0.4, 18.7, 0.4);
        const double hold = c.gesture(20.2, 0.35, 22.7, 0.5);
        r.headTilt = 0.42 * birds - nod + 0.6 * L.look - 0.06 * watch;
        r.headTurn = watch * 0.8;
        r.hatLift = 0.25 * c.gesture(20.0, 0.3, 20.6, 0.4) + 0.06 * hold * std::sin(t * 9);
        r.handB = lerp(r.handB, r.hip + V2(0.06 * h, -0.52 * h), hold);
        const double reply = c.gesture(10.1, 0.28, 10.65, 0.4);
        r.handB = lerp(r.handB, r.hip + V2(-0.14 * h, -0.40 * h + 4 * std::sin(t * 9)), reply);
        r.headTurn = std::max(r.headTurn, reply * 0.6);
        streetFigure(p, r, WARM_T);
        const double swing = 0.10 * std::sin(Tau * st.phase * 2 - 0.8) * w.motion + 0.18 * L.wind * std::sin(t * 3) + 0.05 * std::sin(t * 1.3);
        const double bright = 1.0 + 0.45 * onsetFlash(c, 9) - 0.15 * (hash1(std::floor(t * 14)) < 0.06);
        lantern(p, l, r.handF, swing, 1.0, bright);
        const V2 lp = r.handF + V2(14 + std::sin(swing) * 26, -6 + std::cos(swing) * 26);
        l.save();
        l.translate(lp.x, gy + 10);
        l.scale(1, 0.16);
        l.glow(0, 0, 110, Col(1.0f, 0.62f, 0.30f), 0.20 * bright);
        l.restore();
    }
    // Cyclist, right to left, headlamp sweeping the wet street.
    const double ct=c.schedule->action(Moment::Cyclist,t,15.0);
    const double cs = 15.0, ce = 21.4;
    if (ct > cs && ct < ce) {
        const double dist = (ct - cs) * 370;
        const double cx = 2060 - dist + ox, cy = 1012, r = 32;
        const double wheel = dist / r, crank = dist / (r * 2.2);
        for (int wdx = -1; wdx <= 1; wdx += 2) {
            const double wx = cx + wdx * 44, wy = cy - r;
            p.color(INK); p.ellipse(wx, wy, r, r); p.stroke(4.0);
            p.color(RIM, 0.45); p.arc(wx, wy - 1.2, r, 3.6, 5.6); p.stroke(1.0);
            for (int k = 0; k < 4; ++k) {
                const double a = -wheel + k * Pi / 4;
                p.line(wx - std::cos(a) * r, wy - std::sin(a) * r, wx + std::cos(a) * r, wy + std::sin(a) * r, 0.8, INK, 0.8);
            }
            p.disc(wx, wy, 3, INK);
        }
        const V2 bb(cx + 6, cy - r - 4), seat(cx + 14, cy - r - 52), bar(cx - 30, cy - r - 64);
        p.poly({{cx + 44, cy - r}, bb, {cx - 22, cy - r - 46}, {cx - 44, cy - r}}, 3.4, INK);
        p.poly({bb, {seat.x, seat.y + 4}}, 3.4, INK);
        p.poly({{cx - 22, cy - r - 46}, bar, {cx - 44, cy - r - 62}}, 3.4, INK);
        p.line(seat.x - 6, seat.y, seat.x + 8, seat.y, 4, INK);
        const V2 pedA = bb + V2(std::cos(crank), std::sin(crank)) * 15, pedB = bb - V2(std::cos(crank), std::sin(crank)) * 15;
        p.line(pedA.x, pedA.y, pedB.x, pedB.y, 3, INK);
        RigIn rr;
        rr.h = 150; rr.facing = -1; rr.hair = Hair::Ponytail; rr.garment = Garment::Jacket; rr.flutter = 0.65 * std::sin(t * 5); rr.lean = 0.42;
        rr.hip = seat + V2(2, -6);
        rr.footF = pedA + V2(0, -4); rr.footB = pedB + V2(0, -4);
        rr.handF = bar + V2(-4, -2); rr.handB = bar + V2(2, 0);
        rr.headTilt = 0.1 + L.look * 0.4;
        drawBody(p, solve(rr), INK);
        const double hxp = cx - 52, hyp = cy - r - 54;
        const double sweep = std::sin(t * 2.2) * 6;
        l.linear(hxp, hyp, hxp - 330, hyp + 60, {{0, WARM_B, 0.42f}, {1, WARM_B, 0}});
        l.moveTo(hxp, hyp); l.lineTo(hxp - 340, hyp + 24 + sweep); l.lineTo(hxp - 320, hyp + 98 + sweep); l.closePath();
        l.fill();
        l.glow(hxp, hyp, 20, WARM_B, 1.0);
        // Bright pool where the beam meets the wet street.
        l.glowEllipse(hxp - 250, 1052 + sweep * 0.5, 120, 14, WARM_B, 0.3);
    }
    c.gpu.over(p);
    c.gpu.over(l, 1.6f, 1.5f);
    c.gpu.add(l, 0.45f, 22);
}

// ---------- the noodle cart, the couple, the cat on the rail ----------
void yatai(Ctx& c, const OsakaState& s, const Life& L) {
    GpuProfile::Group profileGroup(c.gpu.profile,"yatai");
    const double t = c.t, ox = -s.cam * 0.85, yx = 770 + ox;
    const double yo = Kit::OsakaCartRoomV1::level(s, t);
    Canvas& b = c.canvas();
    Kit::OsakaCartRoomV1::draw(b, yx, yo);
    c.gpu.over(b, 1.25f);
    Canvas& p = c.canvas();
    Canvas& l = c.canvas();
    Kit::OsakaCartFrameV1::draw(c, s, p, yx);
    // Cook: ladles in a loop once the cart opens; passes a bowl at 12 s.
    {
        const double cookT=c.schedule->action(Moment::Cook,t,11.4);
        const double step=window(cookT,11.4,0.75,13.3,0.7);
        const double cx = yx + 72 + 68 * step;
        const double cadence=c.schedule->parameter(Moment::Cook,1,2.3,2.9,2.6);
        const double ph = std::fmod(std::max(0.0, t - 5.6), cadence) / cadence;
        V2 hand;
        // Hand targets sit where the ladle tip (hand + 18, 20) meets the pot
        // or the bowl, so the elbow stays bent instead of pointing.
        const V2 pot(yx + 108, 814), bowl(yx + 92, 826), rest(cx + 20, 846);
        if (t < 5.6) hand = rest;
        else if (ph < 0.3) hand = lerp(rest, pot, easeInOut(ph / 0.3));
        else if (ph < 0.45) hand = pot + V2(2 * std::sin(Tau * (ph - 0.3) / 0.15), 7 * std::sin(Pi * (ph - 0.3) / 0.15));
        else if (ph < 0.7) hand = lerp(pot, bowl, easeInOut((ph - 0.45) / 0.25));
        else if (ph < 0.85) hand = bowl + V2(0, 3 * std::sin(Pi * (ph - 0.7) / 0.15));
        else hand = lerp(bowl, rest, easeInOut((ph - 0.85) / 0.15));
        const double serve = window(cookT,11.9,0.4,12.9,0.4);
        const double nod = window(cookT,13.05,0.18,13.25,0.32);
        hand = lerp(hand, rest, L.hush);
        hand = lerp(hand, V2(yx + 170, 838), serve);
        RigIn r;
        r.h = 108; r.facing = 1; r.hair = Hair::Short; r.garment = Garment::Happi; r.shoulderTowel = true; r.flutter = L.wind * std::sin(t * 3); r.lean = 0.05 + 0.08 * serve + 0.05 * std::max(0.0, (hand.x - rest.x) / 20);
        r.hip = {cx, 902 - hipHeight(108)};
        Gait g; g.stride = 62; g.lift = 7; g.bob = 2;
        const double mv = clamp01(std::abs(68 * (window(cookT + 0.02, 11.4, 0.75, 13.3, 0.7) - window(cookT - 0.02, 11.4, 0.75, 13.3, 0.7)) / 0.04) / 35);
        const Steps feet = gaitAt(yx + 72, 68 * step, 898, 1, g, mv);
        r.footF = feet.footF; r.footB = feet.footB; r.hip.y += feet.hipBob;
        r.handF = hand; r.handB = {cx + 16 + 2 * std::sin(t * 1.7), 852};
        r.headTilt = -0.25 + 0.2 * serve - 0.38 * nod + L.look * 0.6;
        r.lean = std::min(r.lean, 0.18);
        // Keep the ladle inside the rig's reach through the serving step.
        const V2 shoulder = solve(r).shoulder;
        const V2 reach = hand - shoulder;
        if (reach.len() > 0.29 * r.h) hand = shoulder + reach * (0.29 * r.h / reach.len());
        r.handF = hand;
        drawBody(p, solve(r), INK);
        // Headband knot, ladle and steaming pot.
        const Body bd = solve(r);
        p.line(bd.head.x - 9, bd.head.y - 6, bd.head.x - 19, bd.head.y - 1, 3.5, INK);
        if (serve < 0.5) {
            p.line(hand.x, hand.y, hand.x + 18, hand.y + 20, 3, INK);
            p.disc(hand.x + 20, hand.y + 23, 6, INK);
        } else {
            p.color(INK);
            p.arc(hand.x + 6, hand.y - 2, 10, 0, Pi);
            p.closePath();
            p.fill();
        }
        p.fillRect(yx + 104, 836, 46, 24, INK);
    }
    const double wind = L.wind;
    OsakaNorenV1::draw(c, s, L, l, t, yx);
    OsakaCartLanternV1::draw(c, s, p, l, t, yx, wind);
    // Customer: arrives from the right, sits, takes the bowl and eats.
    if (s.chapter || t > 9) {
        const double sx = yx + 262;
        p.poly({{sx - 14, 936}, {sx - 10, 900}, {sx + 12, 900}, {sx + 16, 936}}, 3.5, INK);
        const double sit=1;
        {
            const double h=118;
            {
                // Sit with a small dip of anticipation, then eat in a loop.
                const double dip = 0;
                const double eat=std::pow(std::max(0.0,std::sin((t-12.6)*2.1)),3);
                const double hold = 1;
                RigIn r;
                r.h = h; r.facing = -1; r.hair = Hair::Short; r.garment = Garment::Jacket; r.flutter = 0.15 * std::sin(t * 2.2) + L.wind * 0.4;
                const V2 stand(sx + 10, 936 - hipHeight(h)), seat(sx, 898 - 0.06 * h);
                r.hip = lerp(stand, seat, sit) + V2(0, dip);
                r.lean = 0.12 * sit + 0.06 * eat;
                r.footF = {sx - 0.22 * h * sit - 6, 932}; r.footB = {sx - 0.19 * h * sit, 934};
                const V2 counter(sx - 40, 870), mouth(sx - 30, 820);
                r.handF = lerp(counter, mouth, eat * hold);
                r.handB = lerp(counter + V2(10, 6), mouth + V2(8, 6), eat * hold);
                r.headTilt = 0.15 * eat * hold - 0.1 * (1 - eat) - 0.38 * c.gesture(13.4, 0.18, 13.65, 0.32) + L.look * 0.6;
                streetFigure(p, r, BCYAN);
                if (hold > 0.5) {
                    const V2 bw = r.handF + V2(-4, -4);
                    p.color(INK); p.arc(bw.x, bw.y, 9, 0, Pi); p.closePath(); p.fill();
                    p.line(bw.x + 2, bw.y - 2, bw.x + 14, bw.y - 18, 1.6, INK);
                }
            }
        }
    }
    // Couple stroll to the railing; one points at the moon, the other leans in.
    {
        const double bx = 1180 + ox;
        for (int k = 0; k < 2; ++k) {
            const double h = k == 0 ? 126 : 116;
            const double endX = k == 0 ? bx : bx - 48;
            const double startX = endX - 540;
            const double d = 540;
            const double mv = 0;
            Gait g; g.stride = 0.7 * h; g.lift = 0.06 * h; g.bob = 0.016 * h;
            const Steps st = gaitAt(startX, d, 936 - 0.04 * h, 1, g, mv);
            RigIn r;
            r.h = h; r.facing = 1; r.hair = k == 0 ? Hair::Short : Hair::Default; r.garment = k == 0 ? Garment::Jacket : Garment::Default; r.obi = k == 1; r.flutter = L.wind * std::sin(t * 2.4 + k) + 0.2 * mv;
            r.hip = {startX + d, 936 - hipHeight(h) + st.hipBob};
            r.footF = st.footF; r.footB = st.footB;
            const double arm = std::sin(Tau * st.phase) * mv;
            const double hold = (L.surge.t >= 0) ? window(t, L.surge.t + 1.2, 0.5, L.surge.t + 2.4, 0.25) : 0;
            const V2 lanternAt(bx - 24, 828);
            if (k == 0) {
                const double point = s.chapter ? c.gesture(12.4, 0.5, 15.6, 0.7) : 0;
                const double antic = 0;
                r.lean = 0.10 * (1 - point) + 0.02 * point + 0.05 * mv - 0.03 * antic;
                r.handF = lerp(r.hip + V2(0.05 * h + 0.07 * h * arm, 0.08 * h), r.hip + V2(0.24 * h, -0.52 * h), backOut(point, 1.2));
                // Points the firework out to her (it is up and to the left),
                // other hand on her shoulder.
                const double cheer = L.look;
                r.handF = lerp(r.handF, r.hip + V2(-0.12 * h, -0.54 * h), cheer);
                r.handB = lerp(r.hip + V2(-0.05 * h - 0.07 * h * arm, 0.08 * h), r.hip + V2(-0.16 * h, -0.30 * h), cheer);
                r.headTilt = 0.35 * point + 0.75 * L.look - 0.2 * c.gesture(17.3, 0.3, 17.7, 0.4);
                r.handF = lerp(r.handF, lanternAt + V2(10, 12), hold);
                r.handB = lerp(r.handB, lanternAt + V2(4, 14), hold);
            } else {
                const double lean = s.chapter ? c.gesture(14.4, 0.8, 19.5, 1.0) : 0;
                r.robe = true; r.bun = true; r.sleeve = true;
                r.lean = 0.12 + 0.12 * lean;
                r.handF = lerp(r.hip + V2(0.18 * h, -0.08 * h + 0.03 * h * arm), r.hip + V2(0.14 * h, -0.36 * h), L.look);
                r.handB = lerp(r.hip + V2(0.1 * h, -0.05 * h), r.hip + V2(0.10 * h, -0.33 * h), L.look);
                r.headTilt = 0.15 * lean + 0.7 * L.look + 0.1 - 0.22 * c.gesture(17.8, 0.25, 18.15, 0.4);
                r.handF = lerp(r.handF, lanternAt + V2(-8, 14), hold);
            }
            streetFigure(p, r, k == 0 ? WARM_T : RED);
        }
        OsakaCoupleLanternV1::draw(L, p, l, t, bx);
    }
    OsakaRailCatV1::draw(c, L, p, t, ox);
    // Child runs from the izakaya toward the flock, brakes and points.
    if (L.surge.t >= 0 && t > L.surge.t + 0.18) {
        const double start = L.surge.t + 0.18, h = 80;
        const double out=easedDistance(t,start,start+1.9,0,104);
        const double back=easedDistance(t,start+5.5,start+7.6,0,104);
        const double d=out-back;
        const double mv=clamp01((speedOf(t,start,start+1.9,0,104)+speedOf(t,start+5.5,start+7.6,0,104))/40);
        Gait g; g.stride = 48; g.lift = 9; g.bob = 3;
        const Steps st = gaitAt(1440+ox-(back>0?104:0),back>0?back:out,932,back>0?1:-1,g,mv);
        RigIn r; r.h = h; r.facing = back>0?1:-1; r.hair = Hair::Short; r.garment = Garment::Happi;
        r.hip = {1440 + ox - d, 936 - hipHeight(h) + st.hipBob};
        r.footF = st.footF; r.footB = st.footB;
        r.lean = 0.10 + 0.14 * mv; r.headTilt = 0.65 + 0.06 * std::sin(t * 3);
        const double point=window(t,start+1.65,0.5,start+5.5,0.8);
        r.handF = r.hip + V2(-14 - 6 * std::sin(Tau * st.phase) * mv, -8 - 36 * point);
        r.handB = r.hip + V2(6 + 9 * std::sin(Tau * st.phase) * mv, 4);
        r.flutter = 0.4 * mv + L.wind * std::sin(t * 4);
        if(t<start+7.8) streetFigure(p,r,RED);
    }
    c.gpu.over(p);
    c.gpu.over(l, 1.6f);
    c.gpu.add(l, 0.5f, 24);
    // Steam from the pot, lit warm at its base, bent by the wind.
    const double steam = 1.0;
    if (steam > 0.01) {
        Program& st = c.gpu.effect("steam", Shaders::steam);
        c.gpu.pass(st, Blend::Add, [&](Program& q) {
            q.set("u_base", float(yx + 126), 838.f); q.set("u_t", float(t)); q.set("u_amt", float(steam));
            q.set("u_wind", float(L.wind)); q.set("u_puff", float(std::min(1.0, c.kick(3.0))));
        }, -1, c.staticGeometry ? QRectF(0,500,1920,342) : QRectF());
    }
}

// ---------- the six wires ----------
struct Pole { double x, top, base, par, sc; };
const Pole POLES[3] = {{1474, 96, 946, 0.9, 1.0}, {1318, 560, 800, 0.5, 0.36}, {1158, 640, 760, 0.3, 0.2}};
const Pole LAST_POLE = {2240, 150, 946, 0.9, 1.0};
// A shorter pole on the pier carries the run down toward the water.
const Pole PIER_POLE = {2650, 470, 1000, 0.9, 0.62};

struct WireSpan { V2 p0, p1; double sag; };
using Spans = std::vector<std::array<WireSpan, 6>>;

Spans wireRuns(double cam) {
    std::vector<std::array<V2, 6>> pts;
    std::array<V2, 6> a, b;
    // The run comes over the near house roof and is fixed to its wall
    // brackets, so it moves with the house, not the screen.
    for (int i = 0; i < 6; ++i) a[i] = {-60 - cam * 1.05, 300 + i * 17.0};
    pts.push_back(a);
    for (int i = 0; i < 6; ++i) b[i] = {534 - cam * 1.05, 318 + i * 15.5};
    pts.push_back(b);
    for (const Pole& p : POLES) {
        std::array<V2, 6> q;
        for (int i = 0; i < 6; ++i) q[i] = {p.x - cam * p.par, p.top + 26 * p.sc + i * 15.5 * p.sc};
        pts.push_back(q);
    }
    std::array<V2, 6> e;
    for (int i = 0; i < 6; ++i) e[i] = {1020 - cam * 0.2, 664 + i * 1.6};
    pts.push_back(e);
    Spans spans;
    for (std::size_t k = 0; k + 1 < pts.size(); ++k) {
        std::array<WireSpan, 6> s;
        const double L = std::abs(pts[k + 1][0].x - pts[k][0].x);
        for (int i = 0; i < 6; ++i) s[i] = {pts[k][i], pts[k + 1][i], L * (0.075 + 0.006 * i)};
        spans.push_back(s);
    }
    return spans;
}

}  // namespace

std::array<std::array<V2, 4>, 6> osakaOutRuns(double cam);
std::array<std::array<V2, 4>, 6> osakaOutRuns(double cam) {
    // Per strand: main pole, quay pole, pier pole, touchdown on the sea.
    std::array<std::array<V2, 4>, 6> r;
    for (int i = 0; i < 6; ++i) {
        r[i][0] = {POLES[0].x - cam * 0.9, POLES[0].top + 26 + i * 15.5};
        r[i][1] = {LAST_POLE.x - cam * 0.9, LAST_POLE.top + 26 + i * 15.5};
        r[i][2] = {PIER_POLE.x - cam * 0.9, PIER_POLE.top + 26 * PIER_POLE.sc + i * 15.5 * PIER_POLE.sc};
        r[i][3] = {3060 + i * 34 - cam * 0.9, 742 + i * 31};
    }
    return r;
}

double outSag(int seg, int i) { return seg == 0 ? 70 + 5.0 * i : seg == 1 ? 46 + 3.0 * i : 40 - 3.0 * i; }

namespace {
V2 wireAt(const WireSpan& s, double u) {
    return {s.p0.x + (s.p1.x - s.p0.x) * u, s.p0.y + (s.p1.y - s.p0.y) * u + s.sag * 4 * u * (1 - u)};
}

// Birds: each lands on its slot on an onset (timed fallback), the wire dips,
// it settles, hops on later onsets, and bursts away on the surge.
struct BirdPlan { double land; int wire; double u; double face; double fromA, fromD; };
const double SLOTS[15][2] = {{1, 0.33}, {1, 0.37}, {3, 0.42}, {0, 0.47}, {2, 0.50}, {2, 0.535}, {4, 0.57}, {1, 0.61},
                             {1, 0.64}, {3, 0.67}, {5, 0.71}, {0, 0.74}, {2, 0.79}, {4, 0.83}, {3, 0.26}};

std::vector<BirdPlan> birdPlan(const Ctx& c) {
    std::vector<BirdPlan> plan;
    for (int k=0;k<15;++k) {
        const double land=c.schedule->birdLand[k];
        const int order = int(std::fmod(k * 7 + c.seed * 3, 15.0));
        BirdPlan b;
        b.land = land; b.wire = int(SLOTS[order][0]); b.u = SLOTS[order][1];
        b.face = c.jit(90 + k) > -0.4 ? 1 : -1;
        b.fromA = -Pi / 2 + 0.9 * c.jit(110 + k) + (c.jit(130 + k) > 0 ? 0.7 : -0.7);
        b.fromD = 520 + 200 * hash2(k, c.seed);
        plan.push_back(b);
    }
    return plan;
}

V2 flockCentre(double t, double t0) {
    // After the burst the flock wheels up, regroups and holds near the moon's
    // side of the sky in screen space, drifting as the camera glides.
    const double u = t - t0;
    const V2 a(1260, 230);
    return a + V2(120*std::sin(u*0.35)+40*u*0.2+easedDistance(u,7,12,0,1500),
                  -30*std::sin(u*0.5)+25*std::sin(u*0.21));
}
const double V_FORM[15][3] = {{0, 0, 1.0}, {-46, 22, 0.95}, {-52, -30, 0.9}, {-100, 44, 0.9}, {-112, -58, 0.85},
                              {-160, 70, 0.8}, {-176, -84, 0.8}, {-70, -4, 0.7}, {-230, 96, 0.75}, {-250, -110, 0.7},
                              {-140, 10, 0.7}, {-300, 30, 0.65}, {-210, -40, 0.7}, {-280, -70, 0.66}, {-330, 60, 0.6}};

// Two passes: `far` draws the valley poles and the spans running down to them,
// behind the street and under the fog; the near pass draws everything else.
void polesWires(Ctx& c, const OsakaState& s, const Life& L, const std::vector<BirdPlan>& birds, bool far) {
    GpuProfile::Group profileGroup(c.gpu.profile,"polesWires");
    const double t = c.t, cam = s.cam;
    Canvas& cv = c.canvas();
    Canvas& l = c.canvas();
    Canvas& cone = c.canvas();
    l.preserveRaster=cone.preserveRaster=true;
    Spans spans = wireRuns(cam);
    const auto outs = osakaOutRuns(cam);
    const double land = clamp01(s.land);
    Canvas* staticPoles = c.retainedBuilder(cv, far ? "far-poles" : "near-poles", {cam, land});
    if(staticPoles)staticPoles->preserveRaster=true;
    const Col INSUL(0.55f, 0.66f, 0.58f);
    // Poles: valley poles fade with the land under the fog instead of popping.
    struct Drawn { Pole p; double alpha; };
    const Drawn poles[] = {{POLES[2], land}, {POLES[1], land}, {{1080, 588, 894, 0.85, 0.40}, land}, {POLES[0], 1}, {LAST_POLE, 1}, {PIER_POLE, 1}};
    for (int pi = 0; pi < 6; ++pi) {
        const Drawn& d = poles[pi];
        if ((pi < 3) != far) continue;
        const Pole& pl = d.p;
        const double px = pl.x - cam * pl.par, sc = pl.sc, al = d.alpha;
        if (px < -120 || px > 2050 || al < 0.01) continue;
        if (staticPoles) {
            Canvas& cv = *staticPoles;
            cv.color(INK, al);
            cv.moveTo(px - 6 * sc, pl.top); cv.lineTo(px + 6 * sc, pl.top); cv.lineTo(px + 8.5 * sc, pl.base); cv.lineTo(px - 8.5 * sc, pl.base); cv.closePath();
            cv.fill();
            cv.line(px - 6 * sc, pl.top, px - 8.5 * sc, pl.base, 1.2, RIM, (0.5 * sc + 0.15) * al);
            // Climbing steps and a cable bundle down the pole.
            for (int k = 0; k < 9; ++k) cv.line(px + 6 * sc, pl.top + (240 + k * 58) * sc, px + 15 * sc, pl.top + (236 + k * 58) * sc, 2.2 * sc, INK, al);
            cv.line(px - 9 * sc, pl.top + 160 * sc, px - 10 * sc, pl.base, 3.2 * sc, INK, al);
            for (int i = 0; i < 6; ++i) {
                const double yy = pl.top + 26 * sc + i * 15.5 * sc;
                cv.line(px - 15 * sc, yy, px + 15 * sc, yy, 3.4 * sc, INK, al);
                // Insulators catch a little light.
                for (double dx : {-11.0, 11.0}) {
                    cv.disc(px + dx * sc, yy - 2.6 * sc, 2.3 * sc, INK, al);
                    cv.disc(px + dx * sc - 0.6 * sc, yy - 3.4 * sc, 1.0 * sc, INSUL, 0.7 * al);
                }
            }
            for (auto [yy, hw] : {std::pair<double, double>{pl.top + 12 * sc, 62}, {pl.top + 124 * sc, 46}}) {
                cv.line(px - hw * sc, yy, px + hw * sc, yy, 6.5 * sc, INK, al);
                cv.poly({{px - hw * 0.7 * sc, yy}, {px, yy + 30 * sc}, {px + hw * 0.7 * sc, yy}}, 2.4 * sc, INK, al);
                for (double dx : {-0.85, -0.45, 0.45, 0.85}) cv.disc(px + dx * hw * sc, yy - 4 * sc, 2.8 * sc, INK, al);
            }
            // Transformer drum with a rim of moonlight.
            const double tx = px + 24 * sc, ty = pl.top + 150 * sc, tw = 15 * sc, th = 50 * sc;
            cv.fillRect(tx - tw, ty, 2 * tw, th, INK, al);
            cv.color(INK, al); cv.ellipse(tx, ty, tw, 4 * sc); cv.fill();
            cv.color(INK, al); cv.ellipse(tx, ty + th, tw, 4 * sc); cv.fill();
            cv.line(tx - tw + 1.5 * sc, ty + 3 * sc, tx - tw + 1.5 * sc, ty + th - 2 * sc, 1.2, RIM, 0.45 * al);
            cv.line(px + 8 * sc, ty + 8 * sc, tx - tw, ty + 10 * sc, 2 * sc, INK, al);
        }
        if (sc == 1.0 && pl.x < 2000) {
            const double ly = 596;
            const double flick = 0.92 + 0.08 * std::sin(t * 13) * std::sin(t * 3.7);
            if (staticPoles) staticPoles->poly({{px, ly + 22}, {px - 60, ly}, {px - 82, ly + 2}}, 4, INK);
            l.fillRect(px - 96, ly + 3, 26, 6, WARM_B * float(flick));
            l.glow(px - 83, ly + 10, 40, WARM_B, 0.7 * flick);
            cone.linear(0, ly, 0, 1000, {{0, mix(WARM_B, GLOW, 0.35), 0.30f * float(flick)}, {1, mix(WARM_B, GLOW, 0.35), 0}});
            cone.moveTo(px - 92, ly + 8); cone.lineTo(px - 74, ly + 8); cone.lineTo(px + 60, 1000); cone.lineTo(px - 240, 1000); cone.closePath();
            cone.fill();
        }
    }
    // Guy anchors share each support's parallax.
    if (far && staticPoles) {
        Canvas& cv = *staticPoles;
        for (const Pole& pl : {POLES[0], LAST_POLE, PIER_POLE}) {
            const double px = pl.x - cam * pl.par;
            cv.line(px - 2 * pl.sc, pl.top + 210 * pl.sc, px - 66 * pl.sc, pl.base, 1.1 * pl.sc, INK, 0.85);
            cv.line(px - 38 * pl.sc, pl.base - 126 * pl.sc, px - 49 * pl.sc, pl.base - 70 * pl.sc, 3 * pl.sc, INSUL, 0.5);
            for (int k = 0; k < 5; ++k) cv.line(px - 12 * pl.sc, pl.top + (220 + k * 110) * pl.sc, px + 6 * pl.sc, pl.top + (220 + k * 110) * pl.sc, 2 * pl.sc, INK2);
        }
    }
    // Service drops from the main pole into the two right-hand houses.
    if (!far) {
        const double px = POLES[0].x - cam * 0.9;
        const WireSpan drops[3] = {{{px - 12, 222}, {1452 - cam * 0.9, 668}, 26}, {{px + 14, 226}, {1628 - cam * 0.9, 604}, 22},
                                   {{px + 14, 236}, {1700 - cam * 0.9, 606}, 30}};
        for (int drop = 0; drop < 3; ++drop) {
            const WireSpan& d = drops[drop];
            if (std::min(d.p0.x, d.p1.x) > 1960) continue;
            std::vector<V2> pts;
            for (int k = 0; k <= 30; ++k) pts.push_back(wireAt(d, k / 30.0) + V2(0, L.wind * 1.5 * std::sin(t * 2.4) * 4 * (k / 30.0) * (1 - k / 30.0)));
            cv.polyline(pts, 1.0, INK, 0.9);
            cv.disc(d.p1.x,d.p1.y,2.4,INK);
        }
    }
    if (far && staticPoles) {
        Canvas& cv = *staticPoles;
        for (const WireSpan& w : {WireSpan{{1080 - cam * 0.85, 604}, {1318 - cam * 0.5, 587}, 38}}) {
            std::vector<V2> pts;
            for (int k = 0; k <= 40; ++k) pts.push_back(wireAt(w, k / 40.0));
            cv.polyline(pts, 1.4, INK, land * 0.8);
        }
    }
    // The weights are constant for this frame; sample only each spatial profile.
    struct Dip {int wire;double u,weight,surge;};
    std::vector<Dip> dips;
    for(const auto& b:birds) {
        const double age=t-b.land;
        if(age<0)continue;
        const double gone=c.schedule->fireworks>=b.land?clamp01((t-c.schedule->fireworks)/0.3):0;
        const double weight=1.4*(1-gone)+3.0*ring(age,2.2,3.5)*(1-gone);
        const double surge=L.surge.t>=0 && t>L.surge.t?2.5*ring(t-L.surge.t,2.6,2.5):0;
        dips.push_back({b.wire,b.u,weight,surge});
    }
    auto dipAt=[&](int wire,double u) {
        double dy=0;
        for(const auto& b:dips)if(b.wire==wire) {
            dy+=b.weight*std::exp(-std::pow((u-b.u)/0.05,2));
            if(b.surge!=0)dy-=b.surge*std::exp(-std::pow((u-b.u)/0.07,2));
        }
        return dy;
    };
    static const auto tailFade=[] {std::array<double,16> values{};for(int j=0;j<16;++j)values[j]=std::pow(1-j/16.0,1.6);return values;}();
    // Each strand has its own gauge (the bass strand heaviest) and a moonlit
    // upper edge; it hums with a soft glow that follows its band.
    const double gauge[6] = {2.1, 1.9, 1.6, 1.35, 1.15, 0.95};
    const double spanScale[6] = {1.0, 1.0, 0.88, 0.53, 0.35, 0.26};
    auto strand = [&](const std::vector<V2>& pts, int i, double sc, double alpha, bool out = false) {
        cv.polyline(pts, gauge[i] * sc, INK, 0.95 * alpha);
        std::vector<V2> rim(pts);
        for (V2& q : rim) q.y -= 0.55 * gauge[i] * sc;
        cv.polyline(rim, 0.6 * sc, RIM, 0.32 * alpha);
        // Bound the continuous core, not the travelling band accents. A drop
        // must not turn a whole conductor into an opaque white stroke.
        const double lift = c.lift(i);
        const double thump = (i < 2 ? 0.14 : 0.06) * c.kick(7);
        double hum = (0.035 + 0.11 * c.band(i) + 0.26 * lift + thump) / (1 + 0.7 * lift) * alpha * (far ? 0.20 : 1.0);
        double width = 1.25 + 0.35 * clamp01(lift);
        // Retain the established quay-to-water instrument as it enters view.
        // The house/moon span and diagonal valley bundle remain restrained.
        const double quay=out?0.25:0;
        hum = lerp(hum, std::min(0.6, (0.05 + 0.12 * c.band(i) + 0.46 * lift) * alpha), quay);
        width = lerp(width, 2.2 + 1.5 * lift, quay);
        if (hum > 0.01) l.polyline(pts, width * sc, i < 2 ? WARM_T : PULSE[i], hum);
    };
    for (std::size_t si = 0; si < spans.size(); ++si) {
        if ((si >= 2) != far) continue;
        const double al = si < 2 ? 1.0 : land;
        if (al < 0.01) continue;
        for (int i = 0; i < 6; ++i) {
            const WireSpan& w = spans[si][std::size_t(i)];
            if (std::min(w.p0.x, w.p1.x) > 1960 || std::max(w.p0.x, w.p1.x) < -40) continue;
            std::vector<V2> pts;
            for (int k = 0; k <= 60; ++k) {
                V2 q = wireAt(w, k / 60.0);
                if (si == 1) q.y += dipAt(i, k / 60.0);
                q.y += L.wind * 2.0 * std::sin(t * 2.1 + i) * 4 * (k / 60.0) * (1 - k / 60.0);
                pts.push_back(q);
            }
            strand(pts, i, spanScale[si], al);
        }
    }
    for (int i = 0; i < 6 && !far; ++i) {
        for (int seg = 0; seg < 3; ++seg) {
            const V2 p0 = outs[std::size_t(i)][std::size_t(seg)], p1 = outs[std::size_t(i)][std::size_t(seg + 1)];
            if (std::min(p0.x, p1.x) > 1960 || std::max(p0.x, p1.x) < -40) continue;
            std::vector<V2> pts;
            for (int k = 0; k <= 60; ++k) {
                V2 q = wireAt({p0, p1, outSag(seg, i)}, k / 60.0);
                q.y += L.wind * 2.0 * std::sin(t * 2.1 + i) * 4 * (k / 60.0) * (1 - k / 60.0);
                pts.push_back(q);
            }
            strand(pts, i, 1.0, seg == 0 ? 1.0 : s.outAlpha, true);
        }
    }
    // Light travelling the strands: one stream per frequency role, faster
    // and brighter as its band rises.
    Rng rng(101);
    for (int i = 0; i < 6; ++i) {
        // Put the stronger band response into moving accents, rather than
        // lighting an entire span. Silence retains the same ambient speed.
        const double travel = (c.score ? c.score->strandPhase[i] : 0);
        const double level = 0.55 + 0.8 * c.band(i) + 0.7 * c.lift(i);
        for (int k = 0; k < 6 + i / 2; ++k) {
            const double u = std::fmod(rng.uni() + travel, 1.0) * 4.0;
            const int si = int(u);
            const double tt = u - si;
            const double al = si < 2 ? 1.0 : 0.22 * land;
            const double lvl = std::min(1.4, (0.55 + 0.45 * rng.uni()) * level) * al;
            if (lvl < 0.02 || (si >= 2) != far) continue;
            const WireSpan& w = spans[std::size_t(si)][std::size_t(i)];
            // Consume the same RNG values, but skip geometry that cannot
            // touch the framebuffer, including its largest 33 px halo.
            if (std::min(w.p0.x, w.p1.x) > 1968 || std::max(w.p0.x, w.p1.x) < -48) continue;
            const double sc = std::array<double, 4>{1.0, 1.0, 0.35, 0.25}[std::size_t(si)];
            const double tail = (0.085 - 0.008 * i) / std::max(0.45, std::abs(w.p1.x - w.p0.x) / 900);
            for (int j = 0; j < 16; ++j) {
                const double tj = tt - tail * j / 16;
                if (tj < 0) break;
                V2 q = wireAt(w, tj);
                if (si == 1) q.y += dipAt(i, tj);
                l.disc(q.x, q.y, (2.4 - 1.6 * j / 16) * sc * (0.8 + 0.5 * lvl), PULSE[i], tailFade[j] * std::min(1.0, lvl));
            }
            V2 q = wireAt(w, tt);
            if (si == 1) q.y += dipAt(i, tt);
            l.glow(q.x, q.y, (13 + 14 * lvl) * sc, PULSE[i], 0.9 * std::min(1.0, lvl));
        }
        Rng quayRng(1012 + i);
        const double quayTravel=travel*1.4;
        for (int k = 0; k < 6; ++k) {
            Rng& pulseRng = k < 4 ? rng : quayRng;
            const double u = std::fmod(pulseRng.uni() + quayTravel, 1.0) * 3.0;
            const int seg = int(u);
            const double tt = u - seg;
            const double lvl = std::min(1.4, (0.55 + 0.45 * pulseRng.uni()) * level) * (seg == 0 ? 1.0 : s.outAlpha);
            if (lvl < 0.02 || far) continue;
            const V2 p0 = outs[std::size_t(i)][std::size_t(seg)], p1 = outs[std::size_t(i)][std::size_t(seg + 1)];
            if (std::min(p0.x, p1.x) > 1968 || std::max(p0.x, p1.x) < -48) continue;
            const WireSpan w{p0, p1, outSag(seg, i)};
            for (int j = 0; j < 16; ++j) {
                const double tj = tt - 0.07 * j / 16;
                if (tj < 0) break;
                const V2 q = wireAt(w, tj);
                l.disc(q.x, q.y, (2.4 - 1.6 * j / 16) * (0.8 + 0.5 * lvl), PULSE[i], tailFade[j] * std::min(1.0, lvl));
            }
            const V2 q = wireAt(w, tt);
            l.glow(q.x, q.y, 13 + 14 * lvl, PULSE[i], 0.9 * std::min(1.0, lvl));
        }
    }
    c.gpu.add(cone, 1.0f, 7);
    c.gpu.over(cv);
    c.gpu.over(l, 1.9f);
    c.gpu.add(l, 0.5f, 12);
}

// Festival lanterns strung from the near eave to the street pole. They come
// on one by one at 3 s; each onset then sends a wave of light along the
// string (alternating direction), bass hits lift them all, and every
// firework shell makes them flare as its light arrives.


// Moths circling the street lamp; they get busier with the treble.


void drawBirds(Ctx& c, const OsakaState& s, const Life& L, const std::vector<BirdPlan>& plan) {
    GpuProfile::Group profileGroup(c.gpu.profile,"drawBirds");
    const double t = c.t;
    Canvas& cv = c.canvas();
    const Spans spans = wireRuns(s.cam);
    for (std::size_t k = 0; k < plan.size(); ++k) {
        const BirdPlan& b = plan[k];
        const WireSpan& w = spans[1][std::size_t(b.wire)];
        const V2 slot = wireAt(w, b.u) + V2(0, 1.4 + 3.0 * ring(t - b.land, 2.2, 3.5));
        const double fly = 1.15;
        const double burst=c.schedule->fireworks>=b.land?c.schedule->fireworks+0.04*k+0.12*hash2(k,3):1e9;
        if (t < b.land - fly) continue;
        if (t < b.land) {
            // Approach on a curve, flare and land.
            const double u = (t - (b.land - fly)) / fly;
            const V2 direction(std::cos(b.fromA),std::sin(b.fromA));
            const double tx=direction.x>0?(2000-slot.x)/direction.x:(-80-slot.x)/direction.x;
            const double ty=direction.y>0?(1160-slot.y)/direction.y:(-80-slot.y)/direction.y;
            const double distance=std::max(b.fromD,std::min(tx,ty)+80);
            const V2 from=slot+direction*distance;
            const V2 ctrl = lerp(from, slot, 0.5) + V2(0, -120);
            const double e = easeOut(u);
            const V2 q = from * ((1 - e) * (1 - e)) + ctrl * (2 * (1 - e) * e) + slot * (e * e);
            const double flare = sstep(0.75, 1.0, u);
            const double flap = lerp(0.5 + 0.5 * std::sin(t * 24 + k), 1.0, flare);
            drawBirdFly(cv, q.x, q.y - 8 * flare, 13 - 2 * flare, INK, flap, 0.2 * (slot.x > from.x ? 1 : -1) * (1 - flare), 1 - 0.2 * flare);
        } else if (t < burst) {
            // Perched: settle, hop on onsets, turn now and then, preen.
            const double settle = 1 - clamp01((t - b.land) / 0.25);
            double hop = 0, dip = 0;
            double face = b.face;
            if (c.score) {
                const Event* e = Score::last(c.score->onsets, t);
                if (e && e->t > b.land + 0.3 && hash2(k, std::floor(e->t * 50)) < 0.35) {
                    const double age = t - e->t;
                    if (age < 0.28) hop = 5 * std::sin(Pi * age / 0.28);
                }
            }
            const double turn = std::floor((t + hash2(k, 7) * 4) / 3.1);
            if (hash2(k, turn) < 0.3) face = -face;
            dip = std::max(0.0, std::sin(t * 0.8 + k * 1.9)) > 0.97 ? 0.7 : 0.0;
            dip += 0.4 * L.look;
            if (settle > 0) drawBirdFly(cv, slot.x, slot.y - 10 * settle, 11, INK, 1.0, 0, settle * 0.8, settle);
            drawBirdPerched(cv, slot.x, slot.y - hop, 10.5, INK, face, dip);
        } else {
            // Burst: scatter upward, then regroup into the flock heading right.
            const double age = t - burst;
            if(age>12) continue;
            const double ang = -1.9 + 0.55 * c.jit(150 + k);
            const double dist = (150 + 320 * hash2(k, 11)) * easeOut(std::min(1.0, age / 1.4));
            const V2 scatter = slot + V2(std::cos(ang) * dist * 1.4 + 120 * std::min(1.0, age), std::sin(ang) * dist);
            const double join = sstep(1.6, 4.0, age);
            const V2 formation = flockCentre(t, burst) + V2(V_FORM[k][0], V_FORM[k][1]);
            const V2 q = lerp(scatter, formation, join);
            const double flapRate = lerp(22, 9, join);
            const double size = lerp(14, 13 * V_FORM[k][2], join);
            drawBirdFly(cv, q.x, q.y, size, INK, 0.5 + 0.5 * std::sin(t * flapRate + k * 2.1), -0.15 + 0.1 * std::sin(k * 1.0));
        }
    }
    c.gpu.over(cv);
}

}  // namespace

void drawBearerLantern(Canvas& body, Canvas& light, V2 hand, double swing, double size, double bright) {
    lantern(body, light, hand, swing, size, bright);
}


namespace {
// ---------- the near house ----------
void nearHouse(Ctx& c, const OsakaState& s, const Life& L) {
    GpuProfile::Group profileGroup(c.gpu.profile,"nearHouse");
    const double t = c.t, ox = -s.cam * 1.05;
    Canvas& cv = c.canvas();
    Canvas& w = c.canvas();
    const Col wall(0.009f, 0.028f, 0.023f), rf(0.013f, 0.050f, 0.039f), rf2(0.040f, 0.170f, 0.127f);
    const double x0 = -80 + ox, x1 = 540 + ox;
    Kit::OsakaNearHouseV1::draw(c, s, cv, x0, x1, wall, rf, rf2);
    // Upstairs panes, laid out like a tiling window manager: master + stack.
    using P = Kit::NearPane;
    const auto& U = Kit::nearPanes;
    double lv[4];
    double room;
    Kit::OsakaNearRoomV1::draw(c, s, L, w, t, ox, U, lv, room);
    c.gpu.over(cv);
    c.gpu.over(w, 1.18f);
    // Shadow play behind the paper.
    Canvas& sh = c.canvas();
    bool any = false;
    if (lv[0] > 0.05  && s.chapter) {
        const double t=c.schedule->action(Moment::Tea,c.t,9.5);
        // Tea-pourer: walks in, lifts the kettle, pours, sets it down, sips.
        const double enter = 1;
        const double next = 0;
        const double hx = lerp(40, 158, enter) + next + ox;
        const double walking=0;
        Gait g; g.stride = 160; g.lift = 14; g.bob = 5;
        const Steps st = gaitAt(40 + ox, 118 * enter + next, 640, 1, g, walking);
        const double bob = st.hipBob;
        const double lift = sstep(9.5, 10.2, t) * (1 - sstep(12.6, 13.3, t));
        const double tilt = window(t,10.4, 0.5, 12.4, 0.4);
        const double sip = window(t,14.6, 0.6, 16.2, 0.6);
        RigIn r;
        r.h = 300; r.facing = 1; r.obi = true; r.flutter = 0.16 * std::sin(c.t * 1.8) + 0.4 * walking; r.robe = true; r.bun = true; r.sleeve = true;
        r.lean = 0.10 + 0.04 * tilt + 0.05 * window(t,16.6, 0.25, 17.1, 0.2) - 0.05 * L.look;
        r.hip = {hx, 640 - hipHeight(300) + bob};
        r.footF = st.footF; r.footB = st.footB;
        const V2 low(hx + 40, 470), kettleUp(hx + 88, 446 - 10 * tilt);
        r.handF = lerp(low, kettleUp, lift);
        r.handF = lerp(r.handF, V2(hx + 30, 400), sip);
        r.handB = lerp(V2(hx + 20, 480), V2(hx + 60, 470), lift);
        const double slide = window(t,19.0, 0.4, 19.8, 0.35);
        r.handF = lerp(r.handF, V2(hx + 42 - 25 * sstep(19.4, 20.0, t), 435), slide);
        r.handF = lerp(r.handF, V2(hx + 18, 474), L.look);
        r.headTilt = -0.2 * tilt + 0.15 * sip + 0.5 * L.look;
        drawBody(sh, solve(r), SHADOW);
        if (t < 13.4 && sip < 0.5) {
            const V2 k = r.handF + V2(6, 0);
            const double ang = 0.65 * tilt;
            sh.save();
            sh.translate(k.x, k.y);
            sh.rotate(ang);
            sh.color(SHADOW); sh.ellipse(0, 0, 20, 15); sh.fill();
            sh.line(16, -4, 34, 8, 5, SHADOW);
            sh.color(SHADOW); sh.arc(0, -12, 13, 3.4, 6.0); sh.stroke(3);
            sh.restore();
            if (tilt > 0.6) {
                const V2 spout = k + V2(std::cos(ang) * 34 - std::sin(ang) * 8, std::sin(ang) * 34 + std::cos(ang) * 8);
                sh.line(spout.x, spout.y, spout.x + 4, 498, 2.2 * (tilt - 0.6) / 0.4, SHADOW);
            }
        } else if (t > 14.3 && t < 17.2) {
            sh.fillRect(r.handF.x - 6, r.handF.y - 10, 12, 12, SHADOW);
        }
        if (t > 17.2) sh.fillRect(200 + ox, 487, 12, 12, SHADOW);
        // The tea is left on a low shelf; the paper door slides a little.
        sh.fillRect(187 + ox, 501, 64, 5, SHADOW);
        const double door = 20 * window(t,19.1, 0.5, 21.0, 0.8);
        sh.fillRect(260 + ox - door, 312, 3, 196, SHADOW, 0.55);
        any = true;
    }
    if (lv[3] > 0.05) {
        OsakaSillCatV1::draw(L, sh, t, ox);
        any = true;
    }
    if (any) {
        Canvas& mask = c.canvas();
    Kit::OsakaNearMaskV1::draw(c, s, mask, ox, U);
        const int lt = c.gpu.layer(sh);
        const int bl = c.gpu.blurred(lt, 3.2f);
        const int mk = c.gpu.layer(mask);
        Program& m = c.gpu.effect("masked", R"(
uniform sampler2D u_tex, u_mask;
uniform float u_opacity;
void main() { o = texture(u_tex, v_uv) * texture(u_mask, v_uv).a * u_opacity; }
)");
        c.gpu.pass(m, Blend::Over, [&](Program& q) {
            c.gpu.bindTexture(0, bl, q, "u_tex");
            c.gpu.bindTexture(1, mk, q, "u_mask");
            q.set("u_opacity", 0.82f);
        });
    }
    Canvas& f = c.canvas();
    c.retain(f, "near-house-lattice", [&](Canvas& f) {
        for (const P& q : U) lattice(f, q.x + ox, q.y, q.w, q.h, q.cols, q.rows, INK, 1.6);
    }, {s.cam});
    const Col roomCol = mix(WARM_T, INK, 0.62);
    c.retain(f, "near-house-lamp-hanger", [&](Canvas& f) {
        f.fillRect(214 + ox, 648, 20, 4, roomCol);
        f.line(270 + ox, 640, 270 + ox, 694, 1.5, roomCol);
    }, {s.cam});
    f.color(Col(1.0f, 0.96f, 0.80f) * float(room));
    f.ellipse(270 + ox, 716, 26, 24);
    f.fill();
    Kit::OsakaDeckV1::draw(c, s, f, ox, roomCol, wall);
    // Woman on the deck edge: opens her fan at 2 s and fans slowly.
    {
        const double h = 290, x = 260 + ox, y = 968;
        const double open = s.chapter ? backOut(clamp01((t - 2.15) / 0.35), 2.0) : 1.0;
        const double antic = s.chapter ? std::sin(Pi * clamp01((t - 1.85) / 0.3)) * (t < 2.15) : 0;
        const double fanPh = std::max(0.0, t - 2.6);
        const double fanning = (0.5 - 0.5 * std::cos(fanPh * Tau / 1.9)) * sstep(2.5, 3.2, t) * (1 - 0.6 * L.look);
        RigIn r;
        r.h = h; r.facing = 1; r.bun = true; r.robe = false; r.sleeve = true; r.obi = true; r.flutter = L.wind * std::sin(t * 2);
        r.hip = {x, y - 0.06 * h};
        r.lean = 0.04 + 0.03 * std::sin(t * 0.7) - 0.03 * L.look;
        r.footF = {x + 0.235 * h, y + 0.2 * h}; r.footB = {x + 0.22 * h, y + 0.21 * h};
        const V2 lap(x + 0.16 * h, y - 0.12 * h), raise(x + 0.2 * h, y - 0.27 * h);
        r.handF = lerp(lap, raise, clamp01(open)) + V2(0, 10 * antic) + V2(-4, -12) * fanning;
        const double wave = c.gesture(9.2, 0.35, 10.25, 0.4);
        r.handB = lerp(V2(x + 0.13 * h, y - 0.1 * h), V2(x + 0.15 * h + 8 * std::sin(t * 8), y - 0.48 * h), wave);
        const double glance = c.gesture(9.0, 0.5, 10.6, 0.6);
        r.headTilt = 0.05 * std::sin(t * 0.5) - 0.25 * glance + 0.8 * L.look;
        drawBody(f, solve(r), INK);
        const double spread = 0.62 * clamp01(open) + 0.04;
        const double ang = -0.85 + 0.5 * fanning + 0.5 * (1 - clamp01(open));
        f.save();
        f.translate(r.handF.x + 2, r.handF.y - 2);
        f.rotate(ang);
        f.color(INK);
        f.moveTo(0, 0);
        f.arc(0, 0, 48, -spread, spread);
        f.closePath();
        f.fill();
        f.restore();
    }
    {
        OsakaVerandaCatV1::draw(c, s, L, f, t, ox);
    }
    {
        OsakaChimeV1::draw(c, L, f, t, ox);
    }
    Rng rng(66);
    for (int i = 0; i < 70; ++i) {
        const double bx = 480 + rng.uni() * 190 + ox, by = 968;
        const double sway = (std::sin(t * 1.4 + i * 0.3) * 2 + L.wind * 9) * (0.6 + 0.4 * rng.uni());
        f.color(INK);
        f.moveTo(bx, by + 6);
        f.curveTo(bx + rng.normal() * 8, by - 14, bx + rng.normal() * 16 + sway * 0.5, by - 30, bx + rng.normal() * 24 + sway, by - 20 - rng.uni() * 30);
        f.stroke(2.4);
    }
    c.gpu.over(f);
    Canvas& az = c.canvas();
    for (int i = 0; i < 54; ++i)
        az.glow(484 + rng.uni() * 180 + ox, 936 + rng.uni() * 30, 2.6 + rng.uni() * 3.2, MAG, 0.9);
    c.gpu.over(az, 1.3f);
}

void wisteria(Ctx& c, const OsakaState& s, const Life& L) {
    GpuProfile::Group profileGroup(c.gpu.profile,"wisteria");
    if (s.wisteria <= 0) return;
    const double t = c.t, ox = -s.cam * 1.25;
    // Past the quay the whole canopy has left the screen. Bound crowns,
    // 420 px racemes, their wind displacement and the petal halos before
    // generating the many small live paths / blur layers.
    const double swayBound = 18.7 * (1 + 2.5 * std::abs(L.wind)) + 26 * std::abs(L.wind) + 2 * std::abs(c.a.bass);
    if (std::max(2034.0, 1970.0 + swayBound) + ox < -24) return;
    Canvas& cv = c.canvas();
    Canvas& e = c.canvas();
    cv.batchSpatially=true;cv.preserveRaster=e.preserveRaster=true;
    const Col dark(0.010f, 0.050f, 0.037f);
    struct Petal {double f,dx,y,r;Col color,light;bool glow;double rotation;Canvas::PreparedEllipse shape;};
    struct Raceme {double x,length;std::vector<Petal> petals;};
    struct Canopy {std::vector<std::array<double,3>> crowns;std::vector<Raceme> racemes;};
    auto makeCanopy=[&] {
        Canopy canopy;
        Rng rng(88);
        auto crown=[&](double x,double y,double r){canopy.crowns.push_back({x,y,r});};
        for(int i=0;i<15;++i)crown(1560+rng.uni()*400,rng.uni()*30-14,34+rng.uni()*40);
        std::vector<double> xs(30);
        for(double& x:xs)x=1575+std::pow(rng.uni(),0.85)*370;
        std::sort(xs.begin(),xs.end());
        for(int i=0;i<30;++i) {
            const double xw=xs[i];
            const double ln=(90+rng.uni()*120)+210*std::pow((xw-1575)/370,1.4)*(0.6+0.4*rng.uni());
            const double wmax=9+rng.uni()*5;
            const int n=int(ln/5);
            Raceme raceme{xw,ln,{}};
            for(int j=0;j<n;++j) {
                const double f=double(j)/n;
                const double wd=wmax*std::sin(std::min(1.0,f*2.6)*Pi/2)*std::pow(1-f,0.6)+0.8;
                for(int side=-1;side<=1;side+=2) {
                    const double dx=side*wd*(0.35+0.65*rng.uni()),y=6+ln*f+rng.normal()*1.6;
                    const double r=2.3+2.0*(1-f)*rng.uni();
                    const Col tone=mix(Col(0.03f,0.26f,0.18f),MINT,std::pow(f,1.3)*(0.6+0.4*rng.uni()));
                    const Col color=mix(dark,tone,0.35+0.65*f);
                    const bool glow=f>0.45 && rng.uni()<0.5;
                    const Col light=glow?(rng.uni()<0.88?MINT:MAG):MINT;
                    raceme.petals.push_back({f,dx,y,r,color,light,glow,side*0.35,Canvas::prepareEllipse(r*0.8,r*1.6,side*0.35,c.gpu.pixelScale())});
                }
            }
            canopy.racemes.push_back(std::move(raceme));
        }
        return canopy;
    };
    Canopy temporary;
    Canopy* canopy=nullptr;
    if(c.staticGeometry) {
        auto& layout=c.staticGeometry->layouts["wisteria"];
        if(!layout.has_value())layout=makeCanopy();
        canopy=&std::any_cast<Canopy&>(layout);
    } else {temporary=makeCanopy();canopy=&temporary;}
    for(const auto& crown:canopy->crowns)cv.disc(crown[0]+ox,crown[1],crown[2],dark);
    const double glint=1+0.9*onsetFlash(c,6);
    for(std::size_t i=0;i<canopy->racemes.size();++i) {
        const auto& raceme=canopy->racemes[i];
        const double x=raceme.x+ox,ln=raceme.length;
        const double sway=std::sin(t*0.9+i*0.7)*(4+ln*0.035)*(1+2.5*L.wind)
                          +L.wind*26*(0.7+0.3*std::sin(t*2.7+i))+2.0*c.a.bass;
        cv.line(x,-10,x+sway*0.1,ln*0.3,1.6,dark);
        for(const auto& p:raceme.petals) {
            const double px=(x+sway*p.f*p.f)+p.dx;
            cv.color(p.color);
            if(c.staticGeometry)cv.fillEllipsePrepared(px,p.y,p.shape);else {cv.ellipse(px,p.y,p.r*0.8,p.r*1.6,p.rotation);cv.fill();}
            if(p.glow)e.glow(px,p.y,p.r*2.4,p.light,std::min(1.0,0.55*p.f*glint));
        }
    }
    c.gpu.over(cv, 1, 1.3f);
    if (!e.empty()) {
        // Both original passes used the same resolved live petal image.
        const int petals = c.gpu.layer(e);
        const int soft = c.gpu.blurred(petals, 1.0f);
        c.gpu.composite(soft, Blend::Over, 1.15f);
        const int bloom = c.gpu.blurred(petals, 12);
        c.gpu.composite(bloom, Blend::Add, 0.35f);
    }
}

void reflections(Ctx& c, const OsakaState& s) {
    GpuProfile::Group profileGroup(c.gpu.profile,"reflections");
    const int snap = c.gpu.snapshot();
    Program& p = c.gpu.effect("reflect", Shaders::reflect);
    c.gpu.pass(p, Blend::Add, [&](Program& q) {
        c.gpu.bindTexture(0, snap, q, "u_img");
        q.set("u_y0", 936.f); q.set("u_qx", 4000.f); q.set("u_t", float(c.t)); q.set("u_gain", float(s.reflection));
        q.set("u_kick", float(std::min(1.0, c.kick(4.5))));
    }, -1, c.staticGeometry ? QRectF(0,936,1920,144) : QRectF());
}
}  // namespace

double bearerX(double t) { return bearerAt(t).x; }

// A shooting star crosses the upper sky on the first strong onset after
// 15.5 s (timed fallback at 16.8 s): one small thing to catch on a rewatch.
void shootingStar(Ctx& c, const OsakaState& s) {
    GpuProfile::Group profileGroup(c.gpu.profile,"shootingStar");
    const double age = c.schedule->action(Moment::Star,c.t,0);
    if (age < 0 || age > 0.9) return;
    const double u = easeOut(age / 0.75);
    const double dx=c.schedule->parameter(Moment::Star,1,-80,480,0);
    const double dy=c.schedule->parameter(Moment::Star,2,-20,35,0);
    const V2 from(470+dx - s.cam * 0.02, 70+dy), to(150+dx - s.cam * 0.02, 205+dy);
    const V2 head = lerp(from, to, u);
    const double fade = 1 - sstep(0.55, 0.9, age);
    Canvas& cv = c.canvas();
    for (int j = 0; j < 24; ++j) {
        const double f = j / 24.0;
        const V2 q = lerp(from, to, std::max(0.0, u - f * 0.35));
        cv.disc(q.x, q.y, 1.8 * (1 - f) + 0.3, mix(CREAM, BCYAN, f), std::pow(1 - f, 1.5) * fade);
    }
    cv.glow(head.x, head.y, 12, CREAM, 0.9 * fade);
    c.gpu.over(cv, 1.6f);
    c.gpu.add(cv, 0.5f, 6);
}

void drawOsakaBackdrop(Ctx& c, const OsakaState& s, bool disc, bool mountain, const BackdropHooks* hooks) {
    GpuProfile::Group profileGroup(c.gpu.profile,"drawOsakaBackdrop");
    const Life L = lifeAt(c);
    Kit::OsakaSkyV1::draw(c, s);
    if (hooks && hooks->afterSky) hooks->afterSky();
    if (s.chapter) shootingStar(c, s);
    if (hooks && hooks->disc) hooks->disc();
    if (disc) {
        DiscLook d;
        d.pos = {1190 - s.cam * 0.015 + s.moonDx, 286 + s.moonDy};
        d.r = 108;
        // Opening: the moon brightens out of the haze over the first seconds.
        const double rise = 1;
        d.col = d.col2 = mix(CREAM, hex(0xe2703a), s.moonWarm) * float(1.22 * rise);
        d.halo = Col(0.55f, 0.95f, 0.74f) * float(rise);
        d.ring = CREAM;
        // The halo swells on each bass hit as well as with the bass body.
        d.energy = (0.35 + 0.6 * c.a.bass + 0.2 * c.a.surge + 0.5 * c.kick(6)) * rise;
        drawDisc(c, d, s.cam);
    }
    if (hooks && hooks->mountain) hooks->mountain();
    if (mountain) {
        MountainLook m;
        m.px = 1040 - s.cam * 0.04; m.peak = 396; m.base = 632; m.width = 330;
        m.top = Col(0.050f, 0.300f, 0.220f); m.bot = Col(0.30f, 0.84f, 0.60f);
        drawMountain(c, m);
    }
    if (hooks && hooks->beforeCoast) hooks->beforeCoast();
    if (hooks && hooks->coast) hooks->coast();
    else if (s.land > 0.01) {
        Kit::OsakaRidgesV1::draw(c, s);
        valleyCity(c, s);
        if (s.chapter) firework(c, s, L);
        if (hooks && hooks->afterValley) hooks->afterValley();
        nearRidge(c, s);
        train(c, s);
    }
    skyLanterns(c, s, L);
}

void drawOsakaCoast(Ctx& c, const OsakaState& s, const BackdropHooks* hooks) {
    GpuProfile::Group profileGroup(c.gpu.profile,"drawOsakaCoast");
    if (s.land > 0.01) {
        const Life L = lifeAt(c);
        Kit::OsakaRidgesV1::draw(c, s);
        valleyCity(c, s);
        if (s.chapter) firework(c, s, L);
        if (hooks && hooks->afterValley) hooks->afterValley();
        nearRidge(c, s);
        train(c, s);
    }
}

void drawOsakaDistantTown(Ctx& c, const OsakaState& s) {
    GpuProfile::Group profileGroup(c.gpu.profile,"drawOsakaDistantTown");
    const Life L = lifeAt(c);
    const auto plan = birdPlan(c);
    if (s.land > 0.01) downhillRoofs(c, s, L);
    if (s.land > 0.01) polesWires(c, s, L, plan, true);
}

void drawOsakaForeground(Ctx& c, const OsakaState& s, const OsakaHooks& hooks) {
    GpuProfile::Group profileGroup(c.gpu.profile,"drawOsakaForeground");
    const Life L = lifeAt(c);
    const auto plan = birdPlan(c);
    if (hooks.town) hooks.town();
    else {
        if (s.land > 0.01) downhillRoofs(c, s, L);
        if (s.land > 0.01) polesWires(c, s, L, plan, true);
    }
    if (hooks.afterTown) hooks.afterTown();
    rightHouses(c, s, L);
    streetSurface(c, s);
    yatai(c, s, L);
    if (hooks.afterYatai) hooks.afterYatai();
    OsakaFestoonV1::draw(c, s, L, V2(POLES[0].x - 16 - s.cam * POLES[0].par, 640));
    polesWires(c, s, L, plan, false);
    OsakaMothsV1::draw(c, s, POLES[0].x - s.cam * POLES[0].par - 83);
    if (hooks.afterWires) hooks.afterWires();
    reflections(c, s);
    if (hooks.afterReflections) hooks.afterReflections();
    streetFront(c, s, L);
    drawBirds(c, s, L, plan);
    nearHouse(c, s, L);
    wisteria(c, s, L);
}

void osakaFog(Ctx& c, double amount, double top, double bottom, Col col) {
    GpuProfile::Group profileGroup(c.gpu.profile,"osakaFog");
    if (amount <= 0.001) return;
    Program& p = c.gpu.effect("fog", Shaders::fog);
    c.gpu.pass(p, Blend::Over, [&](Program& q) {
        q.set("u_amount", float(amount)); q.set("u_top", float(top)); q.set("u_bottom", float(bottom));
        q.set("u_t", float(c.t)); q.set("u_col", col);
    });
}

void osakaGlowThrough(Ctx& c, const OsakaState& s, double amount) {
    GpuProfile::Group profileGroup(c.gpu.profile,"osakaGlowThrough");
    if (amount <= 0.01) return;
    const double t = c.t;
    Canvas& g = c.canvas();
    const double treble = 0.75 + 0.6 * c.lift(5) + 0.3 * c.lift(4);
    Rng rng(77);
    // City: a broad glow with brighter knots where the towers stand.
    for (int k = 0; k < 26; ++k) {
        const double x = 930 + rng.normal() * 230 - s.cam * 0.14, y = 650 + rng.uni() * 50;
        const double tw = 0.75 + 0.25 * std::sin(t * (0.7 + rng.uni()) + k);
        const Col col = rng.uni() < 0.7 ? WARM_T : (rng.uni() < 0.7 ? BCYAN : MAG);
        g.glow(x, y, 26 + rng.uni() * 40, col, 0.30 * tw * treble * clamp01(s.land + 0.35));
    }
    // Downhill town windows and the festival street, closer and warmer.
    for (int k = 0; k < 22; ++k) {
        const double x = 540 + rng.uni() * 720 - s.cam * 0.55, y = 790 + rng.uni() * 150;
        const double band = c.lift(k % 6);
        g.glow(x, y, 34 + rng.uni() * 36, k % 3 ? WARM_T : RED, (0.22 + 0.25 * band) * clamp01(s.land + 0.25));
    }
    c.gpu.add(g, float(1.1 * amount), 10);
}

void drawOsaka(Ctx& c, const OsakaState& s) {
    GpuProfile::Group profileGroup(c.gpu.profile,"drawOsaka");
    drawOsakaBackdrop(c, s, true, true);
    drawOsakaForeground(c, s, {});
    FinishParams f;
    f.time = float(c.t);
    c.gpu.finish(f,nullptr);

}
}
