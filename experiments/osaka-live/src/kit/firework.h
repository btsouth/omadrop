#pragma once
#include "../world.h"
#include "../rig.h"
#include "osaka-legacy.h"

namespace Journey::Kit {
struct OsakaFireworkV1 {
    static constexpr const char* name = "osaka-firework-v1";
    static void draw(Ctx& c, const OsakaState& s, const OsakaLegacyLife& L);
};

inline void firework(Ctx& c, const OsakaState& s, const OsakaLegacyLife& L) { OsakaFireworkV1::draw(c, s, L); }

}
