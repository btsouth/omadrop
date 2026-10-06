#include "city.h"
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
void OsakaValleyCityV1::draw(Ctx& c, const OsakaState& s) {
    GpuProfile::Group profileGroup(c.gpu.profile,"valleyCity");
    const double t = c.t, ox = -s.cam * 0.14;
    Rng rng(41);
    Canvas& bld = c.canvas();
    Canvas& lit = c.canvas();
    Canvas* staticBld = c.retainedBuilder(bld, "valley-buildings", {s.cam});
    const double sparkle = 0.80 + 0.30 * c.band(5) + 0.55 * c.lift(5) + 0.30 * c.lift(4);
    struct Tower { double x, top; };
    std::vector<Tower> towers;
    for (int layer = 0; layer < 2; ++layer) {
        const Col wall = layer == 0 ? Col(0.07f, 0.34f, 0.25f) : Col(0.045f, 0.24f, 0.175f);
        const double base = layer == 0 ? 690 : 712;
        double x = 560 + rng.uni() * 20;
        while (x < 1400) {
            const double centre = std::exp(-std::pow((x - 930) / 260, 2));
            const double w = 9 + rng.uni() * (layer ? 26 : 18);
            double h = (layer ? 10 : 14) + std::pow(rng.uni(), 2.2) * (layer ? 30 : 46) * (0.4 + centre);
            if (layer == 0 && centre > 0.4 && rng.uni() < 0.12) h += 30 + rng.uni() * 26;
            const double top = base - h;
            if (staticBld) {
                staticBld->fillRect(x + ox, top, w, base - top + 40, wall);
                if (rng.uni() < 0.35 && h < 40) {
                    staticBld->color(wall);
                    staticBld->moveTo(x + ox - 1.5, top); staticBld->lineTo(x + ox + w * 0.5, top - w * 0.28); staticBld->lineTo(x + ox + w + 1.5, top);
                    staticBld->closePath(); staticBld->fill();
                }
            } else rng.uni(); // the authored roof-choice draw still advances RNG
            if (layer == 0 && h > 55) towers.push_back({x + w * 0.5 + ox, top});
            // Windows: each keeps its own hours, switching every few seconds.
            const double wx0 = x + 2, wy0 = top + 3;
            const int cols = int((w - 3) / 4), rows = int((base - top - 4) / 5);
            for (int i = 0; i < cols; ++i) {
                for (int j = 0; j < rows; ++j) {
                    const double key = x * 7.1 + i * 13.7 + j * 3.3 + layer * 101;
                    const double period = 3 + 6 * hash2(key, 1);
                    const double cell = std::floor((t + 20) / period + hash2(key, 2));
                    if (hash2(key, cell) > 0.34) continue;
                    const double pick = hash2(key, 4);
                    const Col col = pick < 0.68 ? CREAM : (pick < 0.92 ? BCYAN : MAG);
                    const double b = (0.55 + 0.45 * hash2(key, 5)) * sparkle * (layer ? 0.75 : 1.0);
                    lit.fillRect(wx0 + i * 4 + ox, wy0 + j * 5, 2.0, 2.4, col * float(std::min(1.6, b)), std::min(1.0, b));
                }
            }
            x += w + rng.uni() * 5 - 1;
        }
    }
    // Street lamps along three curving streets, and cars moving on them.
    for (int st = 0; st < 3; ++st) {
        const double y0 = 676 + st * 13, slope = (st - 1) * 0.018;
        for (double x = 600 + st * 23; x < 1360; x += 17 + 6 * hash2(x, st)) {
            const double y = y0 + (x - 930) * slope + 3 * std::sin(x * 0.01 + st);
            lit.glow(x + ox, y, 2.4, WARM_T, 0.65 + 0.2 * std::sin(t * 1.3 + x));
        }
        for (int k = 0; k < 4; ++k) {
            const double dir = (k + st) % 2 ? 1 : -1, speed = 34 + 22 * hash2(k, st + 9);
            const double x = 600 + wrap(hash2(k, st) * 760 + dir * speed * (t + 20) + 7600,760.0);
            const double y = y0 + (x - 930) * slope + 3 * std::sin(x * 0.01 + st) - 0.5;
            if (dir > 0) { lit.glow(x + ox, y, 3.2, CREAM, 0.95); lit.glow(x + ox - 3, y, 2.4, CREAM, 0.8); }
            else { lit.glow(x + ox, y, 2.6, RED, 0.9); lit.glow(x + ox + 3, y, 2.2, RED, 0.8); }
        }
    }
    for (const Tower& tw : towers) {
        const double blink = std::pow(std::max(0.0, std::sin((t + tw.x * 0.01) * Pi)), 6);
        lit.glow(tw.x, tw.top - 2, 5, RED, 0.25 + 0.75 * blink);
    }
    c.gpu.over(bld, 1, 1.2f, float(s.land));
    c.gpu.over(lit, 1.6f, 0, float(s.land));
    c.gpu.add(lit, float(0.45 * s.land), 6);
    hazeBand(c, 690, 30, 0.10, 0.08, ox * 2 + t * 4, 61, mix(WARM_T, GLOW, 0.6), 0.5 * s.land);
}

}
