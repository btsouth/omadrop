// The crossing: no cut and no whole-frame dissolve. The camera glides right
// past the town to the quay; the lantern-bearer boards; valley fog rises over
// the lights; under it the land gives way to water; the wires touch down and
// run on as lines of the swell; the moon sinks and warms into the sun and the
// mountain recedes to Fuji while the palette turns from jade to ink and paper.
#include "parts.h"
#include "rig.h"

#include <cmath>

namespace Journey {
namespace {
const Col CREAM = hex(0xF7E8B2), MINT(0.62f, 0.95f, 0.78f), CYAN = hex(0x2DD5B7), BBLUE = hex(0xACD4CF);
const Col WARM_B(0.98f, 0.90f, 0.66f), PAPER = hex(0xdcd7ba), SPRING = hex(0x7fb4ca), GOLD = hex(0xe6c384);
const Col KRED = hex(0xc34043), HOT = hex(0xe82424), ORANGE = hex(0xc17158), WAVE = hex(0x223249), VIOLET = hex(0x957fb8);
const Col PULSE[6] = {WARM_B, WARM_B, CYAN, CYAN, BBLUE, BBLUE};

// Glide: ease in over 1.5 s, cruise, ease out over 3 s (about 2570 px).
double glide(double t) {
    const double t0 = 28.0, a = 1.5, cruiseEnd = 37.0, t1 = 40.0, v = 315.0;
    if (t <= t0) return 0;
    if (t < t0 + a) { const double u = t - t0; return v * u * u / (2 * a); }
    if (t < cruiseEnd) return v * (a / 2 + (t - t0 - a));
    const double base = v * (a / 2 + (cruiseEnd - t0 - a));
    const double d = t1 - cruiseEnd;
    const double u = std::min(t, t1) - cruiseEnd;
    return base + v * (u - u * u / (2 * d));
}

std::vector<V2> resample(const std::vector<V2>& p, int n) {
    std::vector<double> acc{0};
    for (std::size_t i = 1; i < p.size(); ++i) acc.push_back(acc.back() + (p[i] - p[i - 1]).len());
    std::vector<V2> out;
    const double L = acc.back();
    std::size_t j = 0;
    for (int k = 0; k < n; ++k) {
        const double s = L * k / (n - 1);
        while (j + 2 < acc.size() && acc[j + 1] < s) ++j;
        const double f = (s - acc[j]) / std::max(1e-9, acc[j + 1] - acc[j]);
        out.push_back(lerp(p[j], p[j + 1], clamp01(f)));
    }
    return out;
}
}  // namespace

double crossingCam(double t) { return 30 + glide(t); }

// The six lines on the water: each touches down with a flare, runs right
// across the swell, and from 40 s is handed over to the wave strands.
void crossingStrands(Ctx& c, double t) {
    if (t < 36.0) return;
    const double cam = crossingCam(t);
    const auto outs = osakaOutRuns(cam);
    const auto target = waveStrands(t, 0.0);
    const double morph = easeInOut((t - 40.0) / 3.5);
    const double g = sstep(36.0, 41.5, t);
    const double fadeOut = 1 - sstep(42.5, 44.0, t);
    Canvas& cv = c.canvas();
    Rng rng(9);
    for (int i = 0; i < 6; ++i) {
        const V2 td = outs[std::size_t(i)][2];
        const double touch = 36.5 + i * 0.22;
        if (t < touch) continue;
        const double reach = std::min(1990.0, td.x + 900 * (t - touch));
        std::vector<V2> line;
        for (double x = td.x; x <= reach; x += 8) {
            const double rel = x - td.x;
            line.push_back({x, td.y + (10 + 5 * i) * std::sin(rel / (120 + 22 * i) - (t - touch) * 1.3) * sstep(0, 160, rel) + rel * 0.012 * (i - 2)});
        }
        if (line.size() < 2) continue;
        std::vector<V2> pts = resample(line, 80);
        if (morph > 0) {
            const std::vector<V2> dst = resample(target[std::size_t(i)], 80);
            for (int k = 0; k < 80; ++k) pts[std::size_t(k)] = lerp(pts[std::size_t(k)], dst[std::size_t(k)], morph);
        }
        const Col col = mix(mix(MINT, CREAM, 0.3), mix(SPRING, PAPER, 0.35), g);
        cv.polyline(pts, 2.0, col, 0.85 * fadeOut);
        const double age = t - touch;
        if (age < 1.6) cv.glow(td.x, td.y, 18 + 26 * std::exp(-age * 3), CREAM, (0.55 + 0.35 * std::exp(-age * 4)) * (1 - age / 1.6));
        const double travel = (0.07 + 0.03 * i) * (0.6 * t + 1.5 * (c.score ? c.score->integral(i, t) : 0));
        const double lvl = std::min(1.0, 0.55 + 1.1 * c.band(i));
        for (int q = 0; q < 2; ++q) {
            const int j = int(std::fmod(rng.uni() + travel, 1.0) * 79);
            for (int back = 0; back < 12; ++back) {
                const int jj = j - back;
                if (jj < 0) break;
                cv.disc(pts[std::size_t(jj)].x, pts[std::size_t(jj)].y, 3.2 * (1 - back / 12.0) + 0.6, mix(PULSE[i], PAPER, g), std::pow(1 - back / 12.0, 1.5) * lvl * fadeOut);
            }
            cv.glow(pts[std::size_t(j)].x, pts[std::size_t(j)].y, 20, mix(PULSE[i], i < 2 ? GOLD : PAPER, g), 0.9 * lvl * fadeOut);
        }
    }
    c.gpu.over(cv, 1.5f);
    c.gpu.add(cv, 0.4f, 10);
}

void drawCrossing(Ctx& c) {
    const double t = c.t;
    const double cam = crossingCam(t);
    OsakaState s;
    s.cam = cam;
    s.land = 1 - sstep(33.0, 35.2, t);
    s.harbour = 1 - sstep(33.5, 35.5, t);
    s.outAlpha = 1 - sstep(38.6, 40.0, t);
    s.wisteria = 1;
    const double fog = sstep(29.6, 33.4, t) * (1 - 0.6 * sstep(36.0, 39.0, t)) * (1 - sstep(38.4, 40.0, t));
    const double fogTop = 600 - 90 * sstep(29.6, 33.4, t);
    const double fogBottom = lerp(1400.0, 560.0, sstep(36.0, 39.6, t));
    const double k = sstep(33.4, 35.6, t);
    const double g = sstep(36.0, 41.5, t);

    drawOsakaBackdrop(c, s, false, false);
    if (k > 0.001) {
        c.gpu.beginOffscreen();
        kanagawaSky(c);
        kanagawaMist(c);
        gradeComposite(c, c.gpu.endOffscreen(), g, k);
    }
    // The disc sinks and warms from cream to red.
    {
        const double m = easeInOut((t - 30.5) / 9.5);
        const double warm = sstep(32.0, 40.0, t);
        DiscLook d;
        d.pos = lerp(V2(1190 - cam * 0.015, 286), V2(1310, 512), m);
        d.r = lerp(108, 112, m);
        const Col cream = CREAM * 1.22f;
        const Col orange = hex(0xe2703a) * 1.15f;
        d.col = warm < 0.5 ? mix(cream, orange, warm * 2) : mix(orange, KRED * 1.08f, (warm - 0.5) * 2);
        d.col2 = warm < 0.5 ? d.col : mix(orange, HOT * 1.08f, (warm - 0.5) * 2);
        d.halo = mix(Col(0.55f, 0.95f, 0.74f), ORANGE, warm);
        d.ring = mix(CREAM, GOLD, warm);
        d.haloA = lerp(70, 90, warm); d.haloB = lerp(40, 50, warm); d.haloC = lerp(0.30, 0.34, warm);
        d.haloD = lerp(0.2, 0, warm); d.haloFar = lerp(0.13, 0, warm);
        d.veil = 1 - warm; d.tex = 1 - warm;
        d.energy = 0.35 + 0.6 * c.a.bass;
        d.restRings = lerp(1.0, 1.6, warm);
        drawDisc(c, d, cam);
    }
    // The mountain recedes into Fuji on the same geometry, then the sea laps its foot.
    {
        const double m = easeInOut((t - 33.0) / 6.5);
        MountainLook mt;
        const Col fuji = duotone(mix(WAVE, VIOLET, 0.25), g);
        mt.px = lerp(1040 - cam * 0.04, 1086, m);
        mt.peak = lerp(396, 486, m);
        mt.base = lerp(632, 616, m);
        mt.width = lerp(330, 150, m);
        mt.top = mix(Col(0.050f, 0.300f, 0.220f), fuji, m);
        mt.bot = mix(Col(0.30f, 0.84f, 0.60f), fuji, m);
        mt.snow = sstep(36.5, 39.5, t);
        mt.snowScale = 1;
        mt.snowCol = duotone(PAPER, g);
        mt.foot = lerp(30, 0, m);
        drawMountain(c, mt);
    }
    if (k > 0.001) {
        c.gpu.beginOffscreen(true);
        kanagawaSea(c);
        gradeComposite(c, c.gpu.endOffscreen(), g, k);
    }
    OsakaHooks hooks;
    hooks.afterTown = [&] {
        osakaFog(c, fog, fogTop, fogBottom, mix(Col(0.62f, 0.96f, 0.78f), PAPER, g));
    };
    hooks.afterWires = [&] {
        crossingStrands(c, t);
        // The boat: waits at the steps, takes the lantern-bearer aboard, and
        // rows out to where Kanagawa's near boat will be.
        const double par = 0.85, quay = 2330;
        // The boat starts close and large at the foot of the steps and shrinks
        // as it rows out to where Kanagawa's near boat will be.
        const double away = easeInOut((t - 35.7) / 4.3);
        const double sc = lerp(1.3, 1.0, away);
        const double len = lerp(440, 400, away);
        const V2 moored(quay + 190 + len / 2 - 40 * sc - cam * par, 1014 + 3 * std::sin(t * 1.3));
        const V2 target(1250, 822 + 9 * std::sin(t * 0.8 + 1));
        const V2 at = lerp(moored, target, away);
        const bool aboard = t >= 35.3;
        Canvas& b = c.canvas();
        Canvas& l = c.canvas();
        const double stroke = 0.48 * (t - 35.4);
        const double walk = sstep(35.35, 36.9, t);
        drawBoat(c, b, l, at, len, -0.03 + 0.13 * away, 5, g, sc, aboard, t > 35.4 ? stroke : 0,
                 walk, lerp(146, 74, sstep(35.5, 39.5, t)), walk > 0 && walk < 1 ? 1 : 0);
        // Boarding: down the quay steps and onto the bow.
        if (!aboard && t >= 34.4) {
            const double h = 146;
            const double u = (t - 34.4) / 0.9;
            const double d = 166 * easeInOut(std::min(1.0, u / 0.85));
            const V2 groundW(quay - 10 + d, 966 + 70 * clamp01(d / 160));
            const double hop = sstep(0.85, 1.0, u);
            const V2 ground = lerp(groundW, V2(quay + 190, 1016), hop) - V2(cam * par, 0);
            Gait gt; gt.stride = 0.55 * h; gt.lift = 0.06 * h; gt.bob = 0.015 * h;
            const Steps st = gaitAt(quay - 10 - cam * par, d, 966 - 0.04 * h, 1, gt, 1.0);
            const double drop = ground.y - 966;
            RigIn r;
            r.h = h; r.facing = 1; r.hat = true; r.lean = 0.12;
            r.hip = ground + V2(0, -hipHeight(h) + st.hipBob);
            r.footF = V2(st.footF.x, st.footF.y + drop); r.footB = V2(st.footB.x, st.footB.y + drop + 8);
            r.handF = r.hip + V2(0.24 * h, -0.17 * h);
            r.handB = r.hip + V2(-0.08 * h, 0.05 * h);
            drawBody(b, solve(r), Col(0.012f, 0.030f, 0.024f));
            const V2 lp = r.handF + V2(14, 20);
            b.line(r.handF.x, r.handF.y, lp.x, lp.y - 12, 1.4, Col(0.012f, 0.030f, 0.024f));
            l.glow(lp.x, lp.y, 66, Col(0.99f, 0.78f, 0.42f), 0.5);
            l.color(WARM_B); l.ellipse(lp.x, lp.y, 11, 15); l.fill();
        }
        c.gpu.over(b);
        c.gpu.over(l, 1.6f);
        c.gpu.add(l, 0.45f, 18);
    };
    drawOsakaForeground(c, s, hooks);
    FinishParams f;
    f.time = float(t);
    f.bloom = float(lerp(0.55, 0.3, g));
    f.paper = float(g);
    f.vignette = float(lerp(0.42, 0.34, g));
    f.grain = float(lerp(0.014, 0.016, g));
    c.gpu.finish(f, nullptr);
}
}
