#include "chime.h"
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
void OsakaChimeV1::draw(Ctx& c, const Life& L, Canvas& f, double t, double ox) {
        // Wind chime under the eave swings in the gust.
        const double swing = std::sin(t * 2) * 0.05 + 0.35 * ring((c.schedule->action(Moment::Gust,t,0)),1.3,0.7) + 0.06 * L.wind * std::sin(t * 4.7);
        const V2 top(606 + ox, 596), bell = top + V2(std::sin(swing) * 38, std::cos(swing) * 38);
        f.line(top.x, top.y, bell.x, bell.y - 4, 1.2, INK);
        f.disc(bell.x, bell.y, 7, INK);
        const V2 strip = bell + V2(std::sin(swing * 1.6) * 20, std::cos(swing * 1.6) * 20);
        f.line(bell.x, bell.y + 6, strip.x, strip.y, 6, mix(WARM_B, INK, 0.25));
}

}
