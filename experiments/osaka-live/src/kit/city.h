#pragma once
#include "../world.h"
#include "../rig.h"
#include "events.h"

namespace Journey::Kit {
struct OsakaValleyCityV1 {
    static constexpr const char* name = "osaka-valley-city-v1";
    static void draw(Ctx& c, const OsakaState& s);
};

inline void valleyCity(Ctx& c, const OsakaState& s) { OsakaValleyCityV1::draw(c, s); }

}
