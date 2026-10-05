#pragma once
#include "../world.h"
#include "../rig.h"
#include "osaka-legacy.h"

namespace Journey::Kit {
struct OsakaDownhillRowsV1 {
    static constexpr const char* name = "osaka-downhill-rows-v1";
    static void draw(Ctx& c, const OsakaState& s, const OsakaLegacyLife& L);
};

inline void downhillRoofs(Ctx& c, const OsakaState& s, const OsakaLegacyLife& L) { OsakaDownhillRowsV1::draw(c, s, L); }

}
