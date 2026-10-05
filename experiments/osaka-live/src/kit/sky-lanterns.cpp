#include "lanterns.h"
#include "sky-lanterns.h"
#include "palette.h"
#include "primitives.h"
#include "pane.h"
#include "haze.h"
#include "../rig.h"
#include "../osaka_shaders.h"
#include <cmath>
#include <vector>

namespace Journey::Kit {
using Life = OsakaLegacyLife;
void OsakaSkyLanternsV1::draw(Ctx& c, const OsakaState& s, const Life& L) {
    GpuProfile::Group profileGroup(c.gpu.profile,"skyLanterns");
    if (L.surge.t < 0) return;
    const double t = c.t;
    // Retire the complete warm lantern before its glow leaves a dark shell.
    const double fade = 1 - sstep(10.0,12.0,t-L.surge.t);
    if (fade <= 0) return;
    Canvas& l = c.canvas();
    for (int k = 0; k < 11; ++k) {
        const double born = L.surge.t + 1.4 + 0.42 * k + 0.3 * c.jit(500 + k);
        const double age = t - born;
        if (age < 0) continue;
        const double par = 0.25 + 0.1 * hash2(k, 4);
        const double x = 620 + 70 * k + 40 * c.jit(520 + k) + 12 * std::sin(age * 0.7 + k) + 4 * age - s.cam * par;
        const double y = 705 - (20 + 9 * hash2(k, 5)) * age * (1 - 0.012 * age);
        const double sz = 5.0 + 2.5 * hash2(k, 6);
        const double flick = 0.85 + 0.15 * std::sin(t * 7 + k * 2.3);
        const double in = sstep(0, 0.8, age);
        l.color(WARM_B * float(flick));
        l.ellipse(x, y, sz, sz * 1.3);
        l.fill();
        l.glow(x, y, sz * 10, WARM_T, 0.7 * flick * in);
    }
    c.gpu.over(l, 1.3f, 0, float(fade));
    c.gpu.add(l, float(0.5 * fade), 10);
}

void OsakaCoupleLanternV1::draw(const Life& L, Canvas& p, Canvas& l, double t, double bx) {
        // Their sky lantern: lit in their hands, released, rising past the moon.
        if (L.surge.t >= 0 && t > L.surge.t + 1.2) {
            const double rel = L.surge.t + 2.5;
            const double age = std::max(0.0, t - rel);
            const V2 base(bx - 24, 828);
            const V2 pos = base + V2(5 * age + 10 * std::sin(age * 0.8), -(24 * age + 3 * age * age) * sstep(0, 0.6, age));
            const double glow = sstep(L.surge.t + 1.2, L.surge.t + 2.2, t) * (1 - sstep(9.5,11.5,t-L.surge.t));
            const double flick = 0.88 + 0.12 * std::sin(t * 8);
            paperLantern(l, pos, 9, 12, 0.05 * std::sin(t * 2), WARM_T, flick);
            l.glow(pos.x, pos.y, 70, WARM_T, 0.6 * glow * flick);
            p.line(pos.x - 9, pos.y + 11, pos.x + 9, pos.y + 11, 1.4, INK, std::min(1.0, glow + 0.2));
        }
}

}
