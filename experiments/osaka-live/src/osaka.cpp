// Osaka Jade: "Evening on the hill". A held stage that wakes up and comes
// alive: panes, a noodle cart, a lantern-bearer, shadow play behind paper,
// birds landing on the six wires, and a surge that bursts the flock and
// blooms one firework. Geometry and palette follow the approved mock.
#include "osaka_shaders.h"
#include "parts.h"
#include "rig.h"
#include "kit/actors.h"
#include "kit/neon.h"
#include "kit/firework.h"
#include "kit/light-wave.h"
#include "kit/events.h"
#include "kit/flock.h"
#include "kit/pulses.h"
#include "kit/strands.h"
#include "kit/poles.h"
#include "kit/network.h"
#include "kit/wisteria.h"
#include "kit/grass.h"
#include "kit/downhill.h"
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


// ---------- timeline ----------
// One firework shell. The main pair blooms on the surge; the finale shells
// burst on the bass hits that follow it, so the sky answers the drop.
using Kit::Shell;

using Life = Kit::OsakaEventState;








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




// ---------- town ----------


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
    OsakaPatronsV1::draw(c, L, f, t, x0);
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

}  // namespace

std::array<std::array<V2, 4>, 6> osakaOutRuns(double cam);
std::array<std::array<V2, 4>, 6> osakaOutRuns(double cam) {
    return OsakaWireNetworkV1::outRuns(cam);
}

double outSag(int seg, int i) { return OsakaWireNetworkV1::sag(seg, i); }

namespace {


// Birds: each lands on its slot on an onset (timed fallback), the wire dips,
// it settles, hops on later onsets, and bursts away on the surge.

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
    OsakaPolesV1::draw(c, s, L, cv, l, cone, staticPoles, t, cam, land, far);
    const auto dips = OsakaFlockDipV1::make(c, L, birds, t);
    auto dipAt = [&](int wire, double u) { return dips.at(wire, u); };
    static const auto tailFade=[] {std::array<double,16> values{};for(int j=0;j<16;++j)values[j]=std::pow(1-j/16.0,1.6);return values;}();
    OsakaStrandsV1::draw(c, s, L, cv, l, spans, outs, dipAt, t, land, far);
    OsakaPulseStreamV1::draw(c, s, l, spans, outs, dipAt, tailFade, land, far);
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
        OsakaTeaV1::draw(c, L, sh, ox);
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
    OsakaWomanFanV1::draw(c, s, L, f, t, ox);
    {
        OsakaVerandaCatV1::draw(c, s, L, f, t, ox);
    }
    {
        OsakaChimeV1::draw(c, L, f, t, ox);
    }
    Rng rng(66);
    OsakaGrassV1::draw(L, f, rng, t, ox);
    c.gpu.over(f);
    Canvas& az = c.canvas();
    OsakaGrassFlowersV1::draw(az, rng, ox);
    c.gpu.over(az, 1.3f);
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
    const Life L = OsakaEventsV1::at(c);
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
        const Life L = OsakaEventsV1::at(c);
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
    const Life L = OsakaEventsV1::at(c);
    const auto plan = birdPlan(c);
    if (s.land > 0.01) downhillRoofs(c, s, L);
    if (s.land > 0.01) polesWires(c, s, L, plan, true);
}

void drawOsakaForeground(Ctx& c, const OsakaState& s, const OsakaHooks& hooks) {
    GpuProfile::Group profileGroup(c.gpu.profile,"drawOsakaForeground");
    const Life L = OsakaEventsV1::at(c);
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
