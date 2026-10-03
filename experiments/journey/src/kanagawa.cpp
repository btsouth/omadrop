// Kanagawa: a dawn woodblock print with a red sun. The swell that carries
// the six lines heaves up into the great wave, grows claws on onsets, flings
// spray that falls like snow across Fuji, and breaks on the surge.
#include "parts.h"
#include "rig.h"

#include <cmath>
#include <vector>

namespace Journey {
namespace {
const Col KINK = hex(0x14141c), DEEP = hex(0x1b2740), WAVE = hex(0x223249), MID = hex(0x39558a);
const Col BLUE = hex(0x7e9cd8), SPRING = hex(0x7fb4ca), PAPER = hex(0xdcd7ba), KRED = hex(0xc34043);
const Col HOT = hex(0xe82424), GOLD = hex(0xe6c384), OCHRE = hex(0xc0a36e), ORANGE = hex(0xc17158);
const Col VIOLET = hex(0x957fb8);
const Col OINK(0.012f, 0.030f, 0.024f), WARM_T(0.99f, 0.78f, 0.42f), WARM_B(0.98f, 0.90f, 0.66f);
constexpr double HZ = 612;

const char* kanagawaSkyFs = R"(
uniform float u_grade;
vec3 ramp(float l) {
    vec3 c0 = vec3(0.008, 0.026, 0.022), c1 = vec3(0.022, 0.150, 0.112), c2 = vec3(0.110, 0.520, 0.370);
    vec3 c3 = vec3(0.42, 0.93, 0.68), c4 = vec3(0.97, 0.91, 0.70);
    if (l < 0.22) return mix(c0, c1, l / 0.22);
    if (l < 0.5) return mix(c1, c2, (l - 0.22) / 0.28);
    if (l < 0.78) return mix(c2, c3, (l - 0.5) / 0.28);
    return mix(c3, c4, (l - 0.78) / 0.22);
}
void main() {
    float y = design().y;
    vec3 s0 = vec3(0.071, 0.071, 0.102), s1 = vec3(0.110, 0.125, 0.200), s2 = vec3(0.200, 0.216, 0.353);
    vec3 s3 = vec3(0.478, 0.353, 0.471), s4 = vec3(0.757, 0.443, 0.345), s5 = vec3(0.902, 0.765, 0.518);
    vec3 c;
    if (y < 190.0) c = mix(s0, s1, y / 190.0);
    else if (y < 380.0) c = mix(s1, s2, (y - 190.0) / 190.0);
    else if (y < 500.0) c = mix(s2, s3, (y - 380.0) / 120.0);
    else if (y < 570.0) c = mix(s3, s4, (y - 500.0) / 70.0);
    else if (y < 612.0) c = mix(s4, s5, (y - 570.0) / 42.0);
    else c = s5;
    float l = clamp(dot(c, vec3(0.30, 0.56, 0.14)) * 1.25, 0.0, 1.0);
    o = vec4(mix(ramp(l), c, u_grade), 1.0);
}
)";

const char* gradeCompositeFs = R"(
uniform sampler2D u_tex;
uniform float u_grade, u_opacity;
vec3 ramp(float l) {
    vec3 c0 = vec3(0.008, 0.026, 0.022), c1 = vec3(0.022, 0.150, 0.112), c2 = vec3(0.110, 0.520, 0.370);
    vec3 c3 = vec3(0.42, 0.93, 0.68), c4 = vec3(0.97, 0.91, 0.70);
    if (l < 0.22) return mix(c0, c1, l / 0.22);
    if (l < 0.5) return mix(c1, c2, (l - 0.22) / 0.28);
    if (l < 0.78) return mix(c2, c3, (l - 0.5) / 0.28);
    return mix(c3, c4, (l - 0.78) / 0.22);
}
void main() {
    vec4 s = texture(u_tex, v_uv);
    vec3 c = s.rgb / max(s.a, 1e-4);
    float l = clamp(dot(c, vec3(0.30, 0.56, 0.14)) * 1.25, 0.0, 1.0);
    o = vec4(mix(ramp(l), c, u_grade) * s.a, s.a) * u_opacity;
}
)";

std::vector<V2> catmull(const std::vector<V2>& pts, int n) {
    std::vector<V2> P;
    P.push_back(pts[0] * 2 - pts[1]);
    for (const V2& p : pts) P.push_back(p);
    P.push_back(pts.back() * 2 - pts[pts.size() - 2]);
    std::vector<V2> out;
    for (std::size_t i = 1; i + 2 < P.size(); ++i) {
        const V2 p0 = P[i - 1], p1 = P[i], p2 = P[i + 1], p3 = P[i + 2];
        for (int k = 0; k < n; ++k) {
            const double s = double(k) / n;
            out.push_back((p1 * 2 + (p2 - p0) * s + (p0 * 2 - p1 * 5 + p2 * 4 - p3) * (s * s) + (p1 * 3 - p0 - p2 * 3 + p3) * (s * s * s)) * 0.5);
        }
    }
    out.push_back(P[P.size() - 2]);
    return out;
}

struct WaveFrame {
    std::vector<V2> S, d, n;
    std::vector<double> u, w;
    V2 at(int i, double k) const { return S[std::size_t(i)] + n[std::size_t(i)] * (w[std::size_t(i)] * k); }
    int N() const { return int(S.size()); }
};

// The wave spine from flat swell (rise 0) to the towering crest (rise 1).
// `fall` throws the lip forward and down when it breaks.
WaveFrame waveFrame(double t, double r, double bass, double fall, double collapse, double lean = 0, double kick = 0) {
    static const V2 base[10] = {{-260, 1120}, {40, 930}, {250, 720}, {400, 520}, {520, 372}, {650, 286}, {775, 262}, {885, 288}, {958, 350}, {992, 428}};
    static const V2 flat[10] = {{-260, 1120}, {40, 1010}, {250, 940}, {420, 900}, {560, 880}, {700, 872}, {830, 870}, {940, 874}, {1040, 880}, {1130, 888}};
    std::vector<V2> pts;
    const double rr = r * (1 - collapse);
    for (int i = 0; i < 10; ++i) {
        V2 p = base[i] * rr + flat[i] * (1 - rr);
        // The swell rolls up from below the frame as Kanagawa arrives (no pop at the handover).
        p.y += 210 * (1 - sstep(39.9, 43.0, t)) * (1 - rr);
        p.y -= 12 * std::sin(t * 0.9 + i * 0.5) * (i / 9.0);
        p.y -= bass * 16 * rr * sstep(3, 7, i) + kick * 22 * rr * sstep(3, 8, i);
        if (lean > 0) {
            // Towering: the crest climbs and reaches out over the boats.
            const double k = sstep(3, 9, i);
            p = p + V2(230 * k * k * lean, -(110 * k - 70 * k * k * k) * lean);
        }
        if (fall > 0 && i >= 5) {
            // Plunging lip: rotate the crest forward around the crown and drop it.
            const V2 crown = base[5] * rr + flat[5] * (1 - rr);
            const double k = (i - 4) / 5.0;
            const double a = fall * 1.25 * k;
            const V2 v = p - crown;
            const V2 rv(v.x * std::cos(a) - v.y * std::sin(a), v.x * std::sin(a) + v.y * std::cos(a));
            p = crown + rv * (1 + 0.55 * fall * k) + V2(160 * fall * fall * k, 300 * fall * fall * k * k);
        }
        pts.push_back(p);
    }
    WaveFrame f;
    f.S = catmull(pts, 36);
    const int N = f.N();
    f.d.resize(std::size_t(N)); f.n.resize(std::size_t(N)); f.u.resize(std::size_t(N)); f.w.resize(std::size_t(N));
    for (int i = 0; i < N; ++i) {
        const V2 a = f.S[std::size_t(std::max(0, i - 1))], b = f.S[std::size_t(std::min(N - 1, i + 1))];
        V2 d = b - a;
        const double L = std::max(1e-6, d.len());
        d = d * (1 / L);
        f.d[std::size_t(i)] = d;
        f.n[std::size_t(i)] = {-d.y, d.x};
        f.u[std::size_t(i)] = double(i) / (N - 1);
        f.w[std::size_t(i)] = (400 * std::pow(1 - f.u[std::size_t(i)], 1.1) + 22) * (0.35 + 0.65 * rr);
    }
    return f;
}

void bandPath(Canvas& c, const WaveFrame& f, double k0, double k1, int i0 = 0, int i1 = -1) {
    if (i1 < 0) i1 = f.N();
    for (int i = i0; i < i1; ++i) {
        const V2 p = f.at(i, k0);
        if (i == i0) c.moveTo(p.x, p.y); else c.lineTo(p.x, p.y);
    }
    for (int i = i1 - 1; i >= i0; --i) { const V2 p = f.at(i, k1); c.lineTo(p.x, p.y); }
    c.closePath();
}

// A hooked foam finger: a chain of shrinking discs along a curling path.
V2 claw(Canvas& c, V2 p, double ang, double ln, double r0, double curl, Col col, const Col* out, int n = 13) {
    std::vector<std::array<double, 3>> pts;
    double a = ang;
    for (int j = 0; j < n; ++j) {
        const double f = double(j) / (n - 1);
        pts.push_back({p.x, p.y, r0 * std::pow(1 - f, 0.8) + 1.2});
        a += curl / n;
        p = p + V2(std::cos(a), std::sin(a)) * (ln / n);
    }
    if (out) for (auto& q : pts) c.disc(q[0], q[1], q[2] + 1.8, *out);
    for (auto& q : pts) c.disc(q[0], q[1], q[2], col);
    return {pts.back()[0], pts.back()[1]};
}

double swellY(double x, int row, double t) {
    static const double rows[7][3] = {{HZ, 5, 0.0}, {640, 9, 0.6}, {676, 13, 1.4}, {724, 20, 2.3}, {790, 30, 3.1}, {874, 42, 4.4}, {980, 56, 5.2}};
    const double f = row / 6.0;
    const double* r = rows[row];
    return r[0] + r[1] * (0.6 * std::sin(x / (90 + 60 * f) + r[2] + t * (0.5 + 0.2 * row)) + 0.4 * std::sin(x / (37 + 30 * f) + r[2] * 2.1 - t * 0.7));
}

void sea(Ctx& c, double t) {
    Canvas& cv = c.canvas();
    for (int i = 0; i < 7; ++i) {
        const double f = i / 6.0;
        const Col col = mix(mix(MID, GOLD, 0.25 * std::pow(1 - f, 3)), DEEP, f * 0.85);
        std::vector<V2> ys;
        for (double x = -20; x <= 1941; x += 6) ys.push_back({x, swellY(x, i, t)});
        cv.color(col);
        cv.moveTo(-20, 1080);
        for (const V2& p : ys) cv.lineTo(p.x, p.y);
        cv.lineTo(1941, 1080);
        cv.closePath();
        cv.fill();
        // Crest lines brighten with their depth's band: far rows high, near rows low.
        const int band = std::clamp(5 - int(std::round(f * 5)), 0, 5);
        const double crest = (0.75 - 0.2 * f) * (0.75 + 0.7 * c.band(band));
        cv.polyline(ys, 1.2 + 1.4 * f, mix(PAPER, BLUE, 0.3 + 0.3 * f), std::min(1.0, crest));
        for (int k = 1; k < 4; ++k) {
            std::vector<V2> s;
            for (const V2& p : ys) s.push_back({p.x, p.y + k * (6 + 16 * f) + 3 * f * std::sin(p.x / 50 + k + t * 0.6)});
            cv.polyline(s, 1.0 + f, BLUE, 0.22 - 0.04 * k);
        }
    }
    c.gpu.over(cv);
    Canvas& g = c.canvas();
    Rng rng(5);
    for (int i = 0; i < 90; ++i) {
        const double yy = HZ + 4 + std::pow(rng.uni(), 1.8) * 250;
        const double w = (10 + rng.uni() * 60) * (0.4 + (yy - HZ) / 200);
        const double xx = 1310 + rng.normal() * (26 + (yy - HZ) * 0.42) + std::sin(t * 2 + i) * 4;
        const bool hot = rng.uni() >= 0.65;
        const double tw = 0.6 + 0.4 * std::sin(t * (1.5 + i % 7 * 0.4) + i);
        g.line(xx - w / 2, yy, xx + w / 2, yy, 1.6 + (yy - HZ) / 90, hot ? HOT : GOLD, 0.75 * (1 - (yy - HZ) / 300) * tw);
    }
    c.gpu.over(g, 1.15f);
}

void mistBars(Ctx& c, double t) {
    Canvas& cv = c.canvas();
    const double bars[6][5] = {{1320, 430, 520, 20, 0.34}, {1500, 462, 600, 16, 0.28}, {700, 470, 380, 14, 0.2},
                               {1040, 548, 300, 12, 0.3}, {1560, 560, 460, 14, 0.36}, {860, 566, 240, 10, 0.26}};
    for (int i = 0; i < 6; ++i) {
        const double x = bars[i][0] + std::sin(t * 0.05 + i) * 30 - (t - 40) * (3 + i);
        cv.line(x, bars[i][1], x + bars[i][2], bars[i][1], bars[i][3], PAPER, bars[i][4]);
    }
    c.gpu.over(cv);
}
}  // namespace

Col duotone(Col c, double g) {
    const double l = clamp01((c.r * 0.30 + c.g * 0.56 + c.b * 0.14) * 1.25);
    const double st[5] = {0, 0.22, 0.5, 0.78, 1.0};
    const Col cs[5] = {Col(0.008f, 0.026f, 0.022f), Col(0.022f, 0.150f, 0.112f), Col(0.110f, 0.520f, 0.370f), Col(0.42f, 0.93f, 0.68f), Col(0.97f, 0.91f, 0.70f)};
    Col r = cs[4];
    for (int i = 0; i < 4; ++i) {
        if (l <= st[i + 1]) { r = mix(cs[i], cs[i + 1], (l - st[i]) / (st[i + 1] - st[i])); break; }
    }
    return mix(r, c, g);
}

void drawBoat(Ctx& c, Canvas& cv, Canvas& light, V2 at, double ln, double tilt, int crew, double style, double sc,
              bool bearer, double stroke, double bearerAt, double bearerH, double bearerWalk) {
    const double t = c.t;
    const Col hullOut = mix(OINK, WAVE, style), hull = mix(OINK, OCHRE, style);
    const Col body = mix(OINK, mix(WAVE, KINK, 0.3), style), head = mix(OINK, PAPER, style), oar = mix(OINK, OCHRE, style);
    cv.save();
    cv.translate(at.x, at.y);
    cv.rotate(tilt);
    cv.color(hullOut);
    cv.moveTo(-ln / 2 - 26 * sc, -16 * sc); cv.curveTo(-ln / 4, 14 * sc, ln / 4, 14 * sc, ln / 2 + 12 * sc, -8 * sc);
    cv.lineTo(ln / 2 - 4 * sc, 10 * sc); cv.curveTo(ln / 4, 26 * sc, -ln / 4, 26 * sc, -ln / 2, 6 * sc); cv.closePath();
    cv.fill();
    cv.color(hull);
    cv.moveTo(-ln / 2 - 20 * sc, -13 * sc); cv.curveTo(-ln / 4, 12 * sc, ln / 4, 12 * sc, ln / 2 + 6 * sc, -6 * sc);
    cv.lineTo(ln / 2 - 6 * sc, 6 * sc); cv.curveTo(ln / 4, 20 * sc, -ln / 4, 20 * sc, -ln / 2, 3 * sc); cv.closePath();
    cv.fill();
    // Rowers bow together on a rowing clock that quickens with the bass.
    for (int i = 0; i < crew; ++i) {
        const double f = (i + 0.5) / crew;
        const double bx = -ln / 2 + ln * (0.12 + (bearer ? 0.62 : 0.72) * f), by = 2 * sc + 8 * sc * std::sin(f * Pi);
        const double phase = stroke - i * 0.02;
        const double bow = 0.35 + 0.32 * std::sin(Tau * phase);
        cv.color(body);
        cv.ellipse(bx, by - 9 * sc, 7.5 * sc, 12 * sc, bow);
        cv.fill();
        cv.disc(bx + 9 * sc * bow, by - 23 * sc, 4.6 * sc, head);
        const double dip = std::max(0.0, std::sin(Tau * phase + 0.6));
        cv.line(bx + 4 * sc, by - 10 * sc, bx + (22 + 8 * bow) * sc, by + (16 + 10 * dip) * sc, 1.6 * sc, oar);
    }
    if (bearer) {
        // The lantern-bearer stands at the bow, hat on, lantern out front.
        const double stern = -ln / 2 + 40 * sc, bow = ln / 2 - 34 * sc;
        const double px = lerp(stern, bow, bearerAt);
        RigIn r;
        r.h = bearerH > 0 ? bearerH : 74 * sc; r.facing = 1; r.hat = true;
        r.lean = 0.05 + 0.03 * std::sin(t * 0.8);
        if (bearerWalk > 0 && bearerAt < 1) {
            Gait g; g.stride = 0.6 * r.h; g.lift = 0.06 * r.h; g.bob = 0.015 * r.h;
            const Steps st = gaitAt(stern, px - stern, -0.04 * r.h, 1, g, 1.0);
            r.hip = {px, 2 * sc - hipHeight(r.h) + st.hipBob};
            r.footF = st.footF; r.footB = st.footB;
        } else {
            r.hip = {px, 2 * sc - hipHeight(r.h)};
            r.footF = {px + 0.05 * r.h, -0.04 * r.h}; r.footB = {px - 0.08 * r.h, -0.04 * r.h};
        }
        r.handF = r.hip + V2(0.3 * r.h, -0.18 * r.h);
        r.handB = r.hip + V2(-0.05 * r.h, 0.05 * r.h);
        r.headTilt = 0.1;
        drawBody(cv, solve(r), mix(OINK, mix(WAVE, KINK, 0.3), style));
        const V2 hand = r.handF;
        const double ls = r.h / 74;
        const V2 top = hand + V2(10 * ls, -8 * ls);
        const double sw = 0.12 * std::sin(t * 1.7) - tilt * 0.8;
        const V2 lp = top + V2(std::sin(sw) * 16 * ls, std::cos(sw) * 16 * ls);
        cv.line(hand.x, hand.y, top.x, top.y, 2.0 * ls, mix(OINK, OCHRE, style));
        cv.line(top.x, top.y, lp.x, lp.y - 8 * ls, 1.2 * ls, mix(OINK, OCHRE, style));
        // Light goes to a separate canvas in the same frame.
        const double ca = std::cos(tilt), sa = std::sin(tilt);
        const V2 world(at.x + lp.x * ca - lp.y * sa, at.y + lp.x * sa + lp.y * ca);
        light.color(mix(WARM_B, Col(1.0f, 0.93f, 0.72f), style));
        light.ellipse(world.x, world.y, 7 * ls, 10 * ls);
        light.fill();
        light.glow(world.x, world.y, 58 * ls, mix(WARM_T, GOLD, style), 0.6);
    }
    cv.restore();
}

double kanagawaRise(const Ctx& c) {
    // The swell heaves up from the first seconds and keeps climbing to 64 s.
    const double u = clamp01((c.t - 43.0) / 21.0);
    return 1 - std::pow(1 - u, 1.6);
}

double kanagawaLean(const Ctx& c) { return easeInOut((c.t - 61.0) / 9.5); }

// Bass hits jolt the crest upward; it settles back.
double crestKick(const Ctx& c) {
    if (!c.score) return 0;
    double k = 0;
    for (auto it = c.score->bassHits.rbegin(); it != c.score->bassHits.rend(); ++it) {
        const double age = c.t - it->t;
        if (age < 0) continue;
        if (age > 1.2) break;
        k += it->strength * std::exp(-age * 4.5) * (1 - std::exp(-age * 30));
    }
    return k;
}

Surge kanagawaBreak(const Ctx& c) {
    Surge s = surgeIn(c.score, 64.0, 71.0, 69.6);
    if (s.fallback && c.score) {
        // Land the fallback on the next strong bass hit when there is one.
        for (const Event& e : c.score->bassHits) {
            if (e.t >= 69.6 && e.t < 70.8 && e.strength > 0.3) { s.t = e.t; break; }
        }
    }
    return s;
}

void kanagawaSky(Ctx& c) {
    Program& sky = c.gpu.effect("kanagawaSky", kanagawaSkyFs);
    c.gpu.pass(sky, Blend::Replace, [&](Program& q) { q.set("u_grade", 1.f); });
}
void kanagawaMist(Ctx& c) { mistBars(c, c.t); }
void kanagawaSea(Ctx& c) { sea(c, c.t); }

void gradeComposite(Ctx& c, int tex, double grade, double opacity) {
    Program& p = c.gpu.effect("gradeComposite", gradeCompositeFs);
    c.gpu.pass(p, Blend::Over, [&](Program& q) {
        c.gpu.bindTexture(0, tex, q, "u_tex");
        q.set("u_grade", float(grade)); q.set("u_opacity", float(opacity));
    });
}

std::array<std::vector<V2>, 6> waveStrands(double t, double rise) {
    const WaveFrame f = waveFrame(t, rise, 0, 0, 0);
    const double ks[6] = {-0.52, -0.18, 0.11, 0.29, 0.59, 0.87};
    std::array<std::vector<V2>, 6> out;
    for (int i = 0; i < 6; ++i)
        for (int j = 0; j < f.N(); ++j) out[std::size_t(i)].push_back(f.at(j, ks[i]));
    return out;
}

void kanagawaFuji(Ctx& c, double grade) {
    MountainLook m;
    m.px = 1086; m.peak = 486; m.base = HZ + 4; m.width = 150;
    m.top = m.bot = duotone(mix(WAVE, VIOLET, 0.25), grade);
    m.snow = 1; m.snowScale = 1; m.snowCol = duotone(PAPER, grade);
    m.foot = 0;
    drawMountain(c, m);
}

void kanagawaSun(Ctx& c) {
    DiscLook d;
    d.pos = {1310, 512};
    d.r = 112;
    d.col = KRED * 1.08f; d.col2 = HOT * 1.08f;
    d.halo = ORANGE; d.ring = GOLD;
    d.haloA = 90; d.haloB = 50; d.haloC = 0.34; d.haloD = 0; d.haloFar = 0;
    d.veil = 0; d.tex = 0;
    d.energy = 0.35 + 0.6 * c.a.bass;
    d.restRings = 1.6;
    drawDisc(c, d, 0);
}

namespace {
// The flock that left Osaka arrives as plovers over the sea.
void plovers(Ctx& c, const KState& k) { drawFlock(c, mix(OINK, PAPER, k.grade), 0.85); }

struct ClawBirth { double t; double u; double s; };

std::vector<ClawBirth> clawBirths(const Ctx& c, double from, double until) {
    std::vector<ClawBirth> out;
    double last = -10;
    int k = 0;
    auto add = [&](double t, double s) {
        out.push_back({t, 0.6 + 0.38 * hash2(k, 17 + c.seed), s});
        ++k; last = t;
    };
    if (c.score && !c.score->onsets.empty()) {
        for (const Event& e : c.score->onsets) {
            if (e.t < from || e.t > until) continue;
            if (e.t - last < (e.t > 63 ? 0.38 : 0.55)) continue;
            add(e.t, 0.6 + 0.4 * e.strength);
        }
    } else {
        for (double t = from; t < until; t += 0.8) add(t, 0.8);
    }
    return out;
}

void greatWave(Ctx& c, const KState& k, const Surge& br) {
    const double t = k.t, r = k.rise;
    const double age = br.t >= 0 ? t - br.t : -1;
    const double fall = age > 0 ? easeIn(clamp01(age / 1.1)) : 0;
    const double collapse = age > 0 ? easeInOut(clamp01((age - 0.7) / 2.4)) : 0;
    const double lean = kanagawaLean(c);
    const WaveFrame f = waveFrame(t, r, c.a.bass, fall, collapse, lean, crestKick(c));
    const int N = f.N();
    const double fade = 1 - sstep(2.6, 3.6, age);
    if (fade <= 0) return;
    Canvas& body = c.canvas();
    body.linear(0, 150, 0, 1080, {{0, mix(WAVE, MID, 0.35), 1}, {1, DEEP, 1}});
    bandPath(body, f, -1.0, 1.0);
    body.fill();
    const double stripes[5][3] = {{0.02, 0.20, 0.75}, {0.36, 0.52, 0.7}, {0.66, 0.80, 0.6}, {-0.42, -0.30, 0.45}, {-0.74, -0.64, 0.35}};
    for (auto& s : stripes) {
        body.color(MID, s[2]);
        bandPath(body, f, s[0], s[1]);
        body.fill();
    }
    c.gpu.over(body, 1, 0, float(fade));
    // The six strands run up the face; pulses travel with their bands.
    Canvas& st = c.canvas();
    const double ks[6] = {-0.52, -0.18, 0.11, 0.29, 0.59, 0.87};
    Rng rng(31);
    for (int i = 0; i < 6; ++i) {
        std::vector<V2> P;
        for (int j = 0; j < N; ++j) P.push_back(f.at(j, ks[i]));
        st.polyline(P, 2.0, mix(SPRING, PAPER, 0.35), 0.8 * sstep(42.0, 44.0, t));
        const double travel = (0.05 + 0.02 * i) * (0.6 * t + 1.5 * (c.score ? c.score->integral(5 - i, t) : 0));
        const double lvl0 = 0.55 + 1.0 * c.band(5 - i);
        for (int q = 0; q < 3; ++q) {
            const double pos = std::fmod(rng.uni() + travel, 1.0);
            const int j = int(pos * (N - 1));
            const double lvl = std::min(1.2, (0.6 + 0.4 * rng.uni()) * lvl0);
            for (int back = 0; back < 26; back += 2) {
                const int jj = j - back;
                if (jj < 0) break;
                st.disc(P[std::size_t(jj)].x, P[std::size_t(jj)].y, 3.4 * (1 - back / 26.0) + 0.6, PAPER, std::pow(1 - back / 26.0, 1.5) * std::min(1.0, lvl));
            }
            st.glow(P[std::size_t(j)].x, P[std::size_t(j)].y, 20, i < 2 ? GOLD : PAPER, 0.9 * std::min(1.0, lvl));
        }
    }
    c.gpu.over(st, 1.25f, 0, float(fade * sstep(42.0, 44.0, t)));
    c.gpu.add(st, 0.35f * float(fade * sstep(42.0, 44.0, t)), 10);
    // Foam: irregular lobes along the cap, claws that curl and open.
    if (r > 0.3) {
        const double grow = sstep(0.3, 0.75, r);
        Canvas& fo = c.canvas();
        const int i0 = int(N * (0.40 + 0.12 * (1 - grow)));
        for (int pass = 0; pass < 2; ++pass) {
            const bool outline = pass == 0;
            const Col col = outline ? WAVE : PAPER;
            const double g = outline ? 2.0 : 0.0;
            // Cap band, tapered in at its start so it has no square end.
            fo.color(col);
            for (int i = i0; i < N; ++i) {
                const double taper = sstep(i0, i0 + 26, i);
                const V2 p = f.at(i, -1.0 + 0.04 * (1 - taper));
                if (i == i0) fo.moveTo(p.x, p.y); else fo.lineTo(p.x, p.y);
            }
            for (int i = N - 1; i >= i0; --i) {
                const double taper = sstep(i0, i0 + 26, i);
                const V2 p = f.at(i, -1.0 + (0.04 + (0.38 * grow - 0.04) * taper));
                fo.lineTo(p.x, p.y);
            }
            fo.closePath();
            fo.fill();
            Rng rs(8);
            int j = i0 + 8;
            while (j < N) {
                // Lobes swell and shrink slowly so the cap seethes.
                const double breathe = 1 + 0.18 * std::sin(t * 1.7 + j * 0.37);
                const double rr = (7 + 26 * std::pow(rs.uni(), 1.6)) * (0.55 + 0.45 * (1 - f.u[std::size_t(j)])) * grow * breathe;
                const double off = rs.normal() * 4;
                const V2 B = f.at(j, -1.0 + 0.45 * grow) + f.n[std::size_t(j)] * off;
                fo.disc(B.x, B.y, rr + g, col);
                j += std::max(2, int(rr / 3.2));
            }
            j = i0;
            while (j < N) {
                const double rr = (5 + 15 * std::pow(rs.uni(), 1.5)) * (0.5 + 0.5 * (1 - f.u[std::size_t(j)])) * grow * (1 + 0.15 * std::sin(t * 2.3 + j));
                const V2 A = f.at(j, -1.0);
                fo.disc(A.x, A.y, rr + g, col);
                j += std::max(2, int(rr / 2.6));
            }
        }
        const int j0 = int(N * 0.58);
        fo.color(PAPER);
        bandPath(fo, f, 0.86, 1.0, j0);
        fo.fill();
        Rng rs(19);
        // Resting claws along the lip, each its own hook, opening and closing.
        int j = j0;
        while (j < N - 2) {
            const double a0 = std::atan2(f.d[std::size_t(j)].y, f.d[std::size_t(j)].x);
            const double sc = 0.5 + (f.u[std::size_t(j)] - 0.58) / 0.42;
            const double ln = (40 + 46 * rs.uni()) * sc * grow;
            const double curl = 1.9 + 0.45 * std::sin(t * 1.3 + j * 0.7) + 0.6 * fall;
            claw(fo, f.at(j, 1.0), a0 + 0.55 + rs.normal() * 0.12, ln, (7 + 5 * rs.uni()) * sc, curl, PAPER, &WAVE);
            if (rs.uni() < 0.6)
                claw(fo, f.at(j, 1.0) + f.d[std::size_t(j)] * 9, a0 + 0.2 + rs.normal() * 0.1, ln * 0.55, 5 * sc, 1.7 + 0.3 * std::sin(t * 1.9 + j), PAPER, &WAVE, 9);
            j += int(7 + 6 * rs.uni());
        }
        // New claws born on onsets: they unfurl from a tight curl and fling spray.
        const auto births = clawBirths(c, 52.0, br.t >= 0 ? br.t : 1e9);
        const int start = std::max(0, int(births.size()) - 22);
        for (int b = start; b < int(births.size()); ++b) {
            const ClawBirth& cb = births[std::size_t(b)];
            const double a = t - cb.t;
            if (a < 0) continue;
            const int jj = std::clamp(int(cb.u * (N - 1)), j0, N - 3);
            const double unfurl = backOut(clamp01(a / 0.45), 1.3);
            const double a0 = std::atan2(f.d[std::size_t(jj)].y, f.d[std::size_t(jj)].x);
            const double sc = (0.5 + (f.u[std::size_t(jj)] - 0.58) / 0.42) * cb.s;
            const double retire = b < int(births.size()) - 18 ? 1 - clamp01((t - births[std::size_t(b + 18 < int(births.size()) ? b + 18 : b)].t) / 0.6) : 1.0;
            const double curl = lerp(4.2, 1.6 + 0.4 * std::sin(t * 1.5 + b), clamp01(unfurl));
            claw(fo, f.at(jj, 1.0) + f.n[std::size_t(jj)] * 4, a0 + 0.8 - 0.2 * clamp01(unfurl), (30 + 50 * hash2(b, 2)) * sc * unfurl * retire * grow,
                 (6 + 5 * hash2(b, 5)) * sc * retire, curl, PAPER, &WAVE, 11);
        }
        // The grasping tip.
        const V2 tip = f.S.back();
        const double a0 = std::atan2(f.d.back().y, f.d.back().x);
        for (int q = 0; q < 5; ++q) {
            const double open = 0.36 + 0.06 * std::sin(t * 1.2 + q) + 0.15 * fall;
            claw(fo, tip + V2(std::cos(a0 - 0.8 + q * 0.4), std::sin(a0 - 0.8 + q * 0.4)) * 10, a0 - 0.75 + q * open,
                 (58 + 30 * rs.uni()) * grow, 11, 1.25 + 0.2 * std::sin(t * 1.6 + q), PAPER, &WAVE);
        }
        for (int q = i0; q < N; q += 9) {
            const V2 p = f.at(q, -0.82);
            fo.disc(p.x + 6 * std::sin(q), p.y + 5 * std::cos(q * 1.7), 3.2, MID, 0.8);
        }
        c.gpu.over(fo, 1, 0, float(fade));
        // Spray: a steady drift off the lip plus bursts from each new claw,
        // carried right and falling like snow across Fuji.
        Canvas& sp = c.canvas();
        Rng pr(77);
        const double amount = grow * (0.6 + 0.5 * c.a.surge + 0.5 * lean) * fade;
        for (int i = 0; i < 260; ++i) {
            const int j = int(N * (0.6 + 0.4 * pr.uni()));
            const V2 base = f.at(std::min(j, N - 1), 1.0);
            const double sa = std::fmod(pr.uni() + t * 0.15, 1.0);
            const double dx = sa * (160 + 520 * pr.uni()), dy = sa * sa * (120 + 380 * pr.uni()) - sa * 60;
            const double jx = pr.normal() * 12, jy = pr.normal() * 12;
            const double sz = 1.4 + 3.2 * pr.uni() * (1 - sa);
            sp.disc(base.x + dx + jx + 8 * std::sin(t * 1.3 + i), base.y + dy + jy, sz, PAPER, 0.95 * std::pow(1 - sa, 0.6) * amount);
        }
        for (int b = std::max(0, int(births.size()) - 10); b < int(births.size()); ++b) {
            const double a = t - births[std::size_t(b)].t;
            if (a < 0 || a > 3.5) continue;
            const int jj = std::clamp(int(births[std::size_t(b)].u * (N - 1)), j0, N - 1);
            const V2 o = f.at(jj, 1.0);
            for (int i = 0; i < 18; ++i) {
                const double ang = -0.9 + 1.4 * hash2(b * 31 + i, 1);
                const double v = 120 + 220 * hash2(b * 31 + i, 2);
                const V2 p = o + V2(std::cos(ang) * v * a + 60 * a * a, std::sin(ang) * v * a + 90 * a * a);
                sp.disc(p.x, p.y, (1.6 + 2.6 * hash2(b * 31 + i, 3)) * (1 - a / 3.5), PAPER, (1 - a / 3.5) * fade);
            }
        }
        c.gpu.over(sp);
    }
}

void frontSwell(Ctx& c, double t, double lift) {
    Canvas& cv = c.canvas();
    // The near swell rolls up into view as Kanagawa arrives instead of
    // appearing on the handover frame.
    const double drop = 170 * (1 - sstep(39.9, 42.8, t));
    std::vector<V2> ys;
    for (double x = -20; x <= 1941; x += 6)
        ys.push_back({x, drop + 990 - (96 + 40 * lift) * std::exp(-std::pow((x - 1500 + 30 * std::sin(t * 0.2)) / 300, 2)) - 30 * std::exp(-std::pow((x - 760) / 200, 2)) + 12 * std::sin(x / 60 + t)});
    cv.color(mix(DEEP, KINK, 0.25));
    cv.moveTo(-20, 1080);
    for (const V2& p : ys) cv.lineTo(p.x, p.y);
    cv.lineTo(1941, 1080);
    cv.closePath();
    cv.fill();
    for (int k = 0; k < 4; ++k) {
        std::vector<V2> s;
        for (const V2& p : ys) s.push_back({p.x, p.y + 14 + k * 18});
        cv.polyline(s, 1.6, BLUE, 0.4 - 0.08 * k);
    }
    Rng rs(23);
    for (std::size_t j = 1; j + 1 < ys.size();) {
        if (ys[j].x > 1340 && ys[j].x < 1640) {
            const double a = std::atan2(ys[j + 1].y - ys[j - 1].y, 12);
            claw(cv, ys[j] + V2(0, 3), a - 2.5 + rs.normal() * 0.15 + 0.2 * std::sin(t * 1.4 + j), 30 + 34 * rs.uni(), 6 + 5 * rs.uni(), -1.8, PAPER, &WAVE, 10);
            j += std::size_t(4 + 5 * rs.uni());
        } else ++j;
    }
    cv.polyline(ys, 3, PAPER, 0.9);
    c.gpu.over(cv);
}

// The wash after the break: a wall of foam lobes that sweeps across the
// frame and drains away, the doorway to the next world.
double washHeight(double x, double age) {
    const double front = 820 + 1500 * easeOut(clamp01((age - 0.6) / 1.9));
    const double peak = 500 * sstep(0.7, 1.7, age) * (1 - sstep(2.4, 4.6, age));
    const double reach = sstep(front + 80, front - 220, x);
    return peak * reach * (0.75 + 0.25 * std::sin(x / 140 + age * 2.2));
}

void wash(Ctx& c, double age, bool front) {
    if (age < 0.55 || age > 5.6) return;
    Canvas& cv = c.canvas();
    const double scale = front ? 0.5 : 1.0;
    const double drain = sstep(2.5, 5.2, age);
    const double frontX = 820 + 1500 * easeOut(clamp01((age - 0.6) / 1.9));
    // Foam surface: lobed, and breaking into drifting patches as it drains.
    std::vector<V2> top;
    for (double x = -30; x <= 1950; x += 6) {
        double h = washHeight(x, age) * scale;
        const double patch = fbm1(x * 0.0055 + (front ? 3.0 : 0.0) + age * 0.35, front ? 5 : 2);
        h *= sstep(drain * 1.6 - 0.75, drain * 1.6 - 0.45, patch * 0.5 + 0.5);
        h += (front ? 10 : 16) * std::sin(x / 37 + age * 3) * (h > 20);
        top.push_back({x, 1080 - std::max(0.0, h)});
    }
    for (int pass = 0; pass < 2; ++pass) {
        const Col col = pass == 0 ? WAVE : PAPER;
        const double grow = pass == 0 ? 2.6 : 0.0;
        cv.color(col);
        cv.moveTo(-30, 1090);
        for (const V2& p : top) cv.lineTo(p.x, p.y - grow);
        cv.lineTo(1950, 1090);
        cv.closePath();
        cv.fill();
        Rng rs(front ? 41 : 42);
        for (std::size_t i = 0; i < top.size(); i += 2) {
            const double h = 1080 - top[i].y;
            if (h < 12) continue;
            const double rr = (8 + 26 * std::pow(rs.uni(), 1.4)) * std::min(1.0, h / 160) * (front ? 0.8 : 1.0);
            cv.disc(top[i].x + rs.normal() * 3, top[i].y + rr * 0.35, rr + grow, col);
            i += std::size_t(rr / 5);
        }
    }
    // Scallop rows inside the foam, as in the print.
    for (int row = 1; row <= 3; ++row) {
        cv.color(MID, 0.55 - 0.12 * row);
        bool started = false;
        for (std::size_t i = 0; i < top.size(); i += 3) {
            const double h = 1080 - top[i].y;
            const double y = top[i].y + row * (front ? 26 : 38) + 6 * std::sin(top[i].x / 22 + row);
            if (h < row * (front ? 34 : 50)) { if (started) { cv.stroke(1.6); started = false; } continue; }
            if (!started) { cv.moveTo(top[i].x, y); started = true; }
            else cv.lineTo(top[i].x, y);
        }
        if (started) cv.stroke(1.6);
    }
    // Claws on the leading edge, curling forward and down.
    if (!front && drain < 0.6) {
        for (int q = 0; q < 7; ++q) {
            const double x = frontX - 40 - q * 34;
            const double h = washHeight(x, age);
            if (h < 40) continue;
            const double y = 1080 - h + 10;
            claw(cv, {x, y}, -0.9 + 0.12 * q + 0.2 * std::sin(age * 3 + q), (40 + 24 * hash2(q, 4)) * std::min(1.0, h / 300),
                 7 + 4 * hash2(q, 5), 2.2 + 0.3 * std::sin(age * 2 + q), PAPER, &WAVE, 11);
        }
    }
    // Impact splash where the lip lands.
    if (!front && age < 3.4) {
        for (int i = 0; i < 120; ++i) {
            const double a0 = age - 0.75 - 0.25 * hash2(i, 9);
            if (a0 < 0) continue;
            const double ang = -Pi / 2 + (hash2(i, 1) - 0.35) * 1.6;
            const double v = 380 + 520 * hash2(i, 2);
            const V2 p = V2(1180 + 90 * hash2(i, 3), 860) + V2(std::cos(ang) * v * a0 + 90 * a0, std::sin(ang) * v * a0 + 420 * a0 * a0);
            const double life = clamp01(1 - a0 / 2.2);
            if (life <= 0) continue;
            cv.disc(p.x, p.y, (2 + 5 * hash2(i, 4)) * (0.5 + 0.5 * life), PAPER, life);
        }
    }
    for (int i = 0; i < 90; ++i) {
        const double x = 2000 * hash2(i, front ? 13 : 14);
        const double h = 1080 - top[std::size_t(std::clamp(int((x + 30) / 6), 0, int(top.size()) - 1))].y;
        if (h < 30) continue;
        cv.disc(x + 10 * std::sin(age + i), 1080 - h * (0.15 + 0.7 * hash2(i, 15)), 3 + 3 * hash2(i, 16), MID, 0.7);
    }
    c.gpu.over(cv);
}
}  // namespace

void kanagawaForeground(Ctx& c, const KState& k) {
    const double t = k.t;
    const Surge br = k.breakAt >= 0 ? Surge{k.breakAt, 1, false} : Surge{};
    const double age = br.t >= 0 ? t - br.t : -1;
    if (k.plovers) plovers(c, k);
    // Boats climb and drop on the swell; the near boat tilts as the wave towers,
    // and both ride through the wash.
    const double stroke = 0.48 * (t - 35.4) + 0.55 * (c.score ? c.score->integral(0, t) - c.score->integral(0, 40) : 0);
    const double washLift = age > 0 ? washHeight(1250, age) : 0;
    Canvas& bc = c.canvas();
    Canvas& bl = c.canvas();
    if (k.boats) {
        const double y1 = 706 + 5 * std::sin(t * 0.9) - std::min(60.0, 0.12 * washHeight(1620, std::max(0.0, age))) * (age > 0);
        drawBoat(c, bc, bl, {1620 + 340 * (1 - easeOut((t - 40) / 7)) + (t - 40) * 2, y1}, 250, -0.07 + 0.03 * std::sin(t * 0.9), 6, k.grade, 0.7, false, stroke + 0.3);
        const double heave = 9 * std::sin(t * 0.8 + 1) + 14 * k.rise * std::sin(t * 0.55);
        const double lean = kanagawaLean(c) * (1 - sstep(0, 1.5, age));
        const double y2 = 822 + heave - std::min(110.0, 0.2 * washLift) - 26 * lean;
        const double tilt = 0.10 + 0.04 * std::sin(t * 0.8) + 0.12 * k.rise * (1 - sstep(0, 2, age)) + 0.08 * lean - 0.1 * std::sin(clamp01(age / 3) * Pi);
        drawBoat(c, bc, bl, {1250 + (t - 40) * 1.2, y2}, 400, tilt, 5, k.grade, 1.0, true, stroke);
    }
    if (k.wave) {
        wash(c, age, false);
        c.gpu.over(bc);
        c.gpu.over(bl, 1.5f);
        c.gpu.add(bl, 0.4f, 16);
        greatWave(c, k, br);
    } else {
        c.gpu.over(bc);
        c.gpu.over(bl, 1.5f);
        c.gpu.add(bl, 0.4f, 16);
    }
    frontSwell(c, t, k.rise);
    if (k.wave) wash(c, age, true);
}

void drawKanagawa(Ctx& c) {
    const double t = c.t;
    KState k;
    k.t = t;
    k.grade = sstep(35.0, 43.0, t);
    k.rise = kanagawaRise(c);
    k.breakAt = kanagawaBreak(c).t;
    const bool graded = k.grade < 0.999;
    if (graded) c.gpu.beginOffscreen();
    kanagawaSky(c);
    kanagawaSun(c);
    mistBars(c, t);
    kanagawaFuji(c, 1.0);
    sea(c, t);
    if (t < 44) crossingStrands(c, t);
    kanagawaForeground(c, k);
    if (graded) {
        const int tex = c.gpu.endOffscreen();
        Program& p = c.gpu.effect("gradeComposite", gradeCompositeFs);
        c.gpu.pass(p, Blend::Replace, [&](Program& q) {
            c.gpu.bindTexture(0, tex, q, "u_tex");
            q.set("u_grade", float(k.grade)); q.set("u_opacity", 1.f);
        });
    }
    FinishParams f;
    f.time = float(t);
    f.bloom = float(lerp(0.55, 0.3, k.grade)); f.grain = 0.016f; f.vignette = 0.34f; f.paper = float(k.grade);
    Canvas& title = c.canvas();
    drawTitle(title, 1, window(t, 40.6, 0.8, 45.8, 1.2), BLUE, PAPER);
    c.gpu.finish(f, &title);
}
}
