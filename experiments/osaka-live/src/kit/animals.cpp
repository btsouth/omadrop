#include "animals.h"
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
void OsakaMothsV1::draw(Ctx& c, const OsakaState& s, double px) {
    GpuProfile::Group profileGroup(c.gpu.profile,"moths");
    const double t = c.t, py = 612;
    if (px < -40 || px > 1960) return;
    Canvas& m = c.canvas();
    const double busy = 1 + 0.8 * c.lift(5);
    for (int k = 0; k < 4; ++k) {
        const double ph = t * (1.6 + 0.5 * k) * busy + k * 1.7 + 0.8 * noise1(t * 1.3, k);
        const double r = 16 + 9 * k + 6 * noise1(t * 0.9, k + 5);
        const V2 q(px + std::cos(ph) * r, py + 6 + std::sin(ph * 1.3) * r * 0.55);
        const double flap = 0.5 + 0.5 * std::sin(t * 40 + k * 3);
        m.ellipse(q.x, q.y, 2.0 + 1.2 * flap, 1.4);
        m.color(Col(0.9f, 0.95f, 0.8f), 0.85);
        m.fill();
    }
    c.gpu.over(m, 1.4f);
    c.gpu.add(m, 0.5f, 3);
}

}
