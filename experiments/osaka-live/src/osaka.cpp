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
    OsakaShamisenV1::draw(c, s, L, t, x0, shamisenPane);
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
// A paper chochin on a short pole: ribbed, capped, lit warm from within.




// A little reflected lantern light on the street-facing cloth. Keep the
// silhouette, but let a coat front / obi separate at small viewing sizes.


void streetFront(Ctx& c, const OsakaState& s, const Life& L) {
    GpuProfile::Group profileGroup(c.gpu.profile,"streetFront");
    const double t = c.t, ox = -s.cam * 0.95;
    Canvas& p = c.canvas();
    Canvas& l = c.canvas();
    OsakaBearerV1::draw(c, L, p, l, t, ox);
    OsakaCyclistV1::draw(c, L, p, l, t, ox);
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
    OsakaCookV1::draw(c, L, p, t, yx);
    const double wind = L.wind;
    OsakaNorenV1::draw(c, s, L, l, t, yx);
    OsakaCartLanternV1::draw(c, s, p, l, t, yx, wind);
    OsakaCustomerV1::draw(c, s, L, p, t, yx);
    OsakaCoupleV1::draw(c, s, L, p, l, t, ox);
    OsakaRailCatV1::draw(c, L, p, t, ox);
    OsakaChildV1::draw(c, L, p, t, ox);
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

double bearerX(double t) { return OsakaBearerV1::x(t); }

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
