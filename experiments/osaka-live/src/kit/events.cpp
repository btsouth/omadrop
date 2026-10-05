#include "events.h"
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
namespace {
const V2 FIREWORK(842, 418);
std::vector<Shell> shellPlan(const Ctx& c, const Life& L) {
    std::vector<Shell> out;
    if (L.surge.t < 0) return out;
    const Col GOLD(1.0f, 0.80f, 0.42f), EMBER(0.95f, 0.36f, 0.16f), PINK(0.98f, 0.45f, 0.72f);
    const Col JADEF(0.40f, 1.0f, 0.74f), ICE(0.70f, 0.95f, 1.0f);
    out.push_back({L.bloom, FIREWORK, 1.0 * L.scale, 0, GOLD, EMBER, 1.0, 0});
    if (!c.schedule->fullFireworkShow) return out;
    out.push_back({L.bloom + 0.26, FIREWORK + V2(-6, 10), 0.55 * L.scale, 1, JADEF, ICE, 0.8, 0});
    // Finale: up to four shells on the next bass hits before the glide.
    struct Slot { V2 at; double size; int kind; Col a, b; double tilt; };
    const Slot spots[4] = {{{640, 330}, 0.62, 1, PINK, MAG, 0},
                           {{1010, 372}, 0.66, 1, ICE, CYAN, 0.6},
                           {{730, 250}, 0.80, 3, GOLD, EMBER, 0},
                           {{960, 268}, 0.58, 1, CYAN, CREAM, 0}};
    for (std::size_t i=0; i<c.schedule->finale.size(); ++i) {
        const auto& e=c.schedule->finale[i];
        const Slot& sl=spots[i];
        out.push_back({e.t+0.62,sl.at,sl.size*L.scale,sl.kind,sl.a,sl.b,e.strength,sl.tilt+0.4*c.jit(600+i)});
    }
    return out;
}
}

OsakaEventState OsakaEventsV1::at(const Ctx& c) {
    Life L;
    L.t = c.t;
    const double t = c.t;
    const double gust=window((c.schedule->action(Moment::Gust,t,0)),0,0.7,7.3,2.6);
    L.wind=0.12+0.06*fbm1(t*0.6,3)+gust*(0.85+0.15*fbm1(t*2.3,9));
    if(c.schedule->fireworks>=0 && t-c.schedule->fireworks<12) {
        L.surge={c.schedule->fireworks,c.schedule->fireworkStrength,false};
        L.scale=c.schedule->fullFireworkShow?1.0:0.35;
        L.bloom=L.surge.t+0.55;
        L.look=window(t,L.surge.t+0.3,0.45,L.surge.t+5.0,1.3);
    }
    // No predicted breakdown or timed surge. The calm breeze is always alive.
    L.hush=0;
    L.shells = shellPlan(c, L);
    return L;
}

}
