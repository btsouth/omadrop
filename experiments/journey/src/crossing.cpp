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
// across the swell, and at arrival is handed over to the wave strands.
void crossingStrands(Ctx& c, double t) {
    if (t < 36.0) return;
    const double cam = crossingCam(t);
    const auto outs = osakaOutRuns(cam);
    const auto target = waveStrands(t, 0.0);
    const double morph = easeInOut((t - T::crossingEnd) / 3.5);
    const double g = sstep(37.0, 51.0, t);
    const double fadeOut = 1 - sstep(50.5, 52.0, t);
    Canvas& cv = c.canvas();
    Rng rng(9);
    for (int i = 0; i < 6; ++i) {
        const V2 td = outs[std::size_t(i)][3];
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
        cv.polyline(pts, 1.6 + 0.6 * c.lift(i), col, (0.38 + 0.42 * c.lift(i) + 0.20 * c.band(i)) * fadeOut);
        const double age = t - touch;
        if (age < 1.6) cv.glow(td.x, td.y, 18 + 26 * std::exp(-age * 3), CREAM, (0.55 + 0.35 * std::exp(-age * 4)) * (1 - age / 1.6));
        const double travel = (0.07 + 0.03 * i) * (0.6 * t + 3.4 * (c.score ? c.score->integral(i, t) : 0));
        const double lvl = std::min(1.0, 0.3 + 1.1 * c.band(i) + 0.7 * c.lift(i));
        for (int q = 0; q < 4; ++q) {
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
    s.land = 1 - sstep(35.0, 46.5, t);
    s.harbour = 1 - sstep(40.0, 47.5, t);
    s.outAlpha = 1 - sstep(40.0, 46.0, t);
    s.wisteria = 1;
    // Fog rises in depth: the far valley first, the near town later; it
    // thins slowly so the sea is revealed gradually, not in one flip.
    const double fog = 0.28 * sstep(29.4, 35.5, t) * (1 - sstep(39.0, 48.0, t));
    const double nearFog = 0.16 * sstep(33.0, 37.0, t) * (1 - sstep(39.0, 46.0, t));
    const double fogTop = 610 - 70 * sstep(29.4, 33.6, t);
    const double fogBottom = lerp(1400.0, 560.0, sstep(39.0, 48.0, t));
    const double k = sstep(33.4, 46.0, t);
    const double g = sstep(37.0, 51.0, t);

    // The disc and the mountain sit behind the valley (ridges, city, trees),
    // exactly as in Osaka, so nothing is painted over the town at the handover.
    BackdropHooks bh;
    bh.afterSky = [&] {
        if (k > 0.001) {
            c.gpu.beginOffscreen();
            kanagawaSky(c);
            kanagawaMist(c);
            gradeComposite(c, c.gpu.endOffscreen(), g, k);
        }
    };
    bh.disc = [&] {
        // The disc sinks and warms from cream to red.
        {
            const double m = easeInOut((t - 30.5) / 17.5);
            const double warm = sstep(34.0, 48.0, t);
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
            d.energy = 0.35 + 0.6 * c.a.bass + warm * (0.55 * c.lift(0) + 0.35 * c.kick(5));
            d.restRings = lerp(1.0, 1.6, warm);
            drawDisc(c, d, cam);
        }
    };
    bh.mountain = [&] {
        // The mountain recedes into Fuji on the same geometry, then the sea laps its foot.
        {
            const double m = easeInOut((t - 33.0) / 15.0);
            MountainLook mt;
            const Col fuji = duotone(mix(WAVE, VIOLET, 0.25), g);
            mt.px = lerp(1040 - cam * 0.04, 1086, m);
            mt.peak = lerp(396, 486, m);
            mt.base = lerp(632, 616, m);
            mt.width = lerp(330, 150, m);
            mt.top = mix(Col(0.050f, 0.300f, 0.220f), fuji, m);
            mt.bot = mix(Col(0.30f, 0.84f, 0.60f), fuji, m);
            mt.snow = sstep(40.0, 48.0, t);
            mt.snowScale = 1;
            mt.snowCol = duotone(PAPER, g);
            mt.foot = lerp(30, 0, m);
            drawMountain(c, mt);
        }
    };
    bh.afterValley = [&] {
        const double farFog = 0.30 * sstep(28.5, 34.5, t) * (1 - sstep(39.0, 48.0, t));
        osakaFog(c, farFog, 590, 840, mix(Col(0.38f, 0.72f, 0.58f), PAPER, g));
        osakaGlowThrough(c, s, farFog * 0.45);
    };
    drawOsakaBackdrop(c, s, false, false, &bh);
    if (k > 0.001) {
        c.gpu.beginOffscreen(true);
        kanagawaSea(c);
        gradeComposite(c, c.gpu.endOffscreen(), g, k);
    }
    OsakaHooks hooks;
    hooks.afterTown = [&] {
        osakaFog(c, fog, fogTop, fogBottom, mix(Col(0.62f, 0.96f, 0.78f), PAPER, g));
        // The town's lights keep glowing through the fog as it thickens.
        osakaGlowThrough(c, s, fog * (1 - sstep(42.0, 48.0, t)));
    };
    hooks.afterWires = [&] {
        osakaFog(c, nearFog, 700 - 40 * sstep(31.8, 35.0, t), 1400, mix(Col(0.55f, 0.90f, 0.72f), PAPER, g));
        crossingStrands(c, t);
        // The boat: waits at the steps, takes the lantern-bearer aboard, and
        // rows out to where Kanagawa's near boat will be.
        const double par = 0.85, quay = 2330;
        // The boat starts close and large at the foot of the steps and shrinks
        // as it rows out to where Kanagawa's near boat will be.
        const double away = easeInOut((t - 35.7) / 12.3);
        const double sc = lerp(1.3, 1.0, away);
        const double len = lerp(440, 400, away);
        const V2 moored(quay + 190 + len / 2 - 40 * sc - cam * par, 1014 + 3 * std::sin(t * 1.3));
        const V2 target(1250, 822 + 9 * std::sin(t * 0.8 + 1) - 7 * c.kick(5));
        const V2 at = lerp(moored, target, away);
        const bool aboard = t >= 35.3;
        Canvas& b = c.canvas();
        Canvas& l = c.canvas();
        const double stroke = rowingClock(c);
        const double walk = sstep(35.35, 38.6, t);
        drawBoat(c, b, l, at, len, lerp(-0.03, 0.10 + 0.04 * std::sin(t * 0.8), away), 5, g, sc, aboard, t > 35.4 ? stroke : 0,
                 walk, lerp(146, 74, sstep(35.5, 45.5, t)), walk > 0 && walk < 1 ? 1 : 0);
        // Boarding: down the quay steps and onto the bow.
        if (!aboard && t >= 34.4) {
            const double h = 146;
            const double elapsed = clamp01((t - 34.4) / 0.9);
            const double measured = c.score ? c.score->integral(0, t) - c.score->integral(0, 34.4) : 0;
            const double total = c.score ? c.score->integral(0, 35.3) - c.score->integral(0, 34.4) : 0;
            const double u = total > 0.01 ? 0.4 * elapsed + 0.6 * clamp01(measured / total) : elapsed;
            // Convert the street's .95 parallax into the quay's .85 once,
            // at the shared endpoint; then walk on the steps in quay space.
            const double start = bearerX(34.4) - 0.10 * crossingCam(34.4);
            const double distance = quay + 190 - start;
            const double d = distance * easeInOut(clamp01(u));
            const V2 groundW(start + d, 966 + 50 * clamp01(d / distance));
            const double hop = sstep(0.85, 1.0, u);
            const V2 ground = lerp(groundW, V2(quay + 190, 1016), hop) - V2(cam * par, 0);
            Gait gt; gt.stride = 0.72 * h; gt.lift = 0.07 * h; gt.bob = 0.018 * h;
            const Steps st = gaitAt(start - 2031 - cam * par, 2031 + d, 966 - 0.04 * h, 1, gt, std::sin(Pi * clamp01(u)));
            const double drop = ground.y - 966;
            RigIn r;
            r.h = h; r.facing = 1; r.hat = true; r.garment = Garment::Jacket; r.lean = 0.02 + 0.10 * std::sin(Pi * clamp01(u));
            r.hip = ground + V2(0, -hipHeight(h) + st.hipBob);
            r.footF = V2(st.footF.x, st.footF.y + drop); r.footB = V2(st.footB.x, st.footB.y + drop);
            r.handF = r.hip + V2(0.24 * h, -0.17 * h);
            r.handB = r.hip + V2(-0.08 * h, 0.05 * h);
            drawBody(b, solve(r), Col(0.012f, 0.030f, 0.024f));
            const double swing = 0.05 * std::sin(t * 1.3);
            drawBearerLantern(b, l, r.handF, swing, 1, 1 + 0.45 * c.hit(9));
        }
        c.gpu.over(b);
        c.gpu.over(l, 1.6f);
        c.gpu.add(l, 0.45f, 18);
    };
    drawOsakaForeground(c, s, hooks);
    if (t > 45.0) {
        kanagawaPlovers(c, g);
        // The far fishing crew rows in from outside the frame before arrival.
        Canvas& b = c.canvas(); Canvas& l = c.canvas();
        const double u = sstep(45.0, T::crossingEnd, t);
        drawBoat(c, b, l, {1960, 706 + 5 * std::sin(t * 0.9)}, 250,
                 -0.07 + 0.03 * std::sin(t * 0.9), 6, g, 0.7, false, rowingClock(c) + 0.3);
        c.gpu.over(b, 1, 0, float(u));
    }
    FinishParams f;
    f.time = float(t);
    f.bloom = float(lerp(0.55, 0.3, g));
    f.paper = float(g);
    f.vignette = float(lerp(0.42, 0.34, g));
    f.grain = float(lerp(0.014, 0.016, g));
    c.gpu.finish(f, nullptr);
}
}
