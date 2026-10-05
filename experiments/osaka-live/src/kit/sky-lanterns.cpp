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

}
