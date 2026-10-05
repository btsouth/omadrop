#include "groups.h"
#include "effects_shaders.h"
#include "../parts.h"
#include "actors.h"
#include "neon.h"
#include "firework.h"
#include "light-wave.h"
#include "flock.h"
#include "pulses.h"
#include "strands.h"
#include "poles.h"
#include "network.h"
#include "wisteria.h"
#include "grass.h"
#include "downhill.h"
#include "city.h"
#include "chime.h"
#include "animals.h"
#include "sky-lanterns.h"
#include "festoon.h"
#include "cloth.h"
#include "onset.h"
#include "lanterns.h"
#include "rooms.h"
#include "pane.h"
#include "town.h"
#include "layout.h"
#include "ridges.h"
#include "sky.h"
#include "haze.h"
#include "mountain.h"
#include "disc.h"
#include "primitives.h"
#include <cmath>
namespace Journey::Kit {
using Life = OsakaEventState;
constexpr double QUAY = 2330.0;
void OsakaNearRidgeV1::draw(Ctx& c, const OsakaState& s) {
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
void OsakaTrainV1::draw(Ctx& c, const OsakaState& s) {
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
void OsakaReflectionV1::draw(Ctx& c, const OsakaState& s) {
    GpuProfile::Group profileGroup(c.gpu.profile,"reflections");
    const int snap = c.gpu.snapshot();
    Program& p = c.gpu.effect("reflect", Shaders::reflect);
    c.gpu.pass(p, Blend::Add, [&](Program& q) {
        c.gpu.bindTexture(0, snap, q, "u_img");
        q.set("u_y0", 936.f); q.set("u_qx", 4000.f); q.set("u_t", float(c.t)); q.set("u_gain", float(s.reflection));
        q.set("u_kick", float(std::min(1.0, c.kick(4.5))));
    }, -1, c.staticGeometry ? QRectF(0,936,1920,144) : QRectF());
}
void OsakaShootingStarV1::draw(Ctx& c, const OsakaState& s) {
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
void OsakaFogV1::draw(Ctx& c, double amount, double top, double bottom, Col col) {
    GpuProfile::Group profileGroup(c.gpu.profile,"osakaFog");
    if (amount <= 0.001) return;
    Program& p = c.gpu.effect("fog", Shaders::fog);
    c.gpu.pass(p, Blend::Over, [&](Program& q) {
        q.set("u_amount", float(amount)); q.set("u_top", float(top)); q.set("u_bottom", float(bottom));
        q.set("u_t", float(c.t)); q.set("u_col", col);
    });
}
void OsakaGlowThroughV1::draw(Ctx& c, const OsakaState& s, double amount) {
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
void OsakaSteamV1::draw(Ctx& c, const OsakaEventState& L, double t, double yx) {
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
}
