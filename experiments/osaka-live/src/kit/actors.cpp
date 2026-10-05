#include "actors.h"
#include "palette.h"
#include "lanterns.h"
#include "sky-lanterns.h"
#include "town.h"
#include "onset.h"
#include <cmath>
namespace Journey::Kit {
void OsakaWomanFanV1::draw(Ctx& c, const OsakaState& s, const OsakaEventState& L, Canvas& f, double t, double ox) {
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
        const double wave = ActionWindow{9.2, 0.35, 10.25, 0.4}.gesture(c);
        r.handB = lerp(V2(x + 0.13 * h, y - 0.1 * h), V2(x + 0.15 * h + 8 * std::sin(t * 8), y - 0.48 * h), wave);
        const double glance = ActionWindow{9.0, 0.5, 10.6, 0.6}.gesture(c);
        r.headTilt = 0.05 * std::sin(t * 0.5) - 0.25 * glance + 0.8 * L.look;
        OsakaFigureV1::draw(f, r, INK);
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
}
}
