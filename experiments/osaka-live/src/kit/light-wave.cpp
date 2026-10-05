#include "light-wave.h"
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
void OsakaLightWaveV1::pane(const Ctx& c, const OsakaEventState& L, double on, V2 centre, double& level) {
    for (const Shell& sh : L.shells) {
        const double arrive = sh.burst + (centre - sh.at).len() / 1500.0;
        if (c.t > arrive) level += 1.1 * sh.size * sh.strength * std::exp(-(c.t - arrive) * 3.2) * on;
    }
}

void OsakaFestoonLightWaveV1::apply(const Life& L, double t, V2 at, double& lv) {
    for (const Shell& sh : L.shells) {
        const double arrive = sh.burst + (at - sh.at).len() / 1500.0;
        if (t > arrive) lv += 0.8 * sh.size * std::exp(-(t - arrive) * 2.6);
    }
}

}
