#pragma once
#include "../world.h"
namespace Journey::Kit {
inline double onsetFlash(const Ctx& c, double decay, double maxAge = 0.6) {
    if (!c.score) return 0;
    const Event* e = Score::last(c.score->onsets,c.t);
    if (!e || c.t - e->t > maxAge) return 0;
    return e->strength * std::exp(-(c.t - e->t) * decay);
}
}
