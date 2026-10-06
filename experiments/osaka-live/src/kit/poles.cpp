#include "poles.h"
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
void OsakaPolesV1::draw(Ctx& c, const OsakaState& s, const Life& L, Canvas& cv, Canvas& l, Canvas& cone, Canvas* staticPoles, double t, double cam, double land, bool far) {
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
}

}
