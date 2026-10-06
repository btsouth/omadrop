#include "grass.h"
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
void OsakaGrassV1::draw(const Life& L, Canvas& f, Rng& rng, double t, double ox) {
    for (int i = 0; i < 70; ++i) {
        const double bx = 480 + rng.uni() * 190 + ox, by = 968;
        const double sway = (std::sin(t * 1.4 + i * 0.3) * 2 + L.wind * 9) * (0.6 + 0.4 * rng.uni());
        f.color(INK);
        f.moveTo(bx, by + 6);
        f.curveTo(bx + rng.normal() * 8, by - 14, bx + rng.normal() * 16 + sway * 0.5, by - 30, bx + rng.normal() * 24 + sway, by - 20 - rng.uni() * 30);
        f.stroke(2.4);
    }
}

void OsakaGrassFlowersV1::draw(Canvas& az, Rng& rng, double ox) {
    for (int i = 0; i < 54; ++i)
        az.glow(484 + rng.uni() * 180 + ox, 936 + rng.uni() * 30, 2.6 + rng.uni() * 3.2, MAG, 0.9);
}

}
