#pragma once
#include "../world.h"
#include "../rig.h"
#include "events.h"

namespace Journey::Kit {
struct OsakaDownhillRowsV1 {
    static constexpr const char* name = "osaka-downhill-rows-v1";
    static void draw(Ctx& c, const OsakaState& s, const OsakaEventState& L);
};

inline void downhillRoofs(Ctx& c, const OsakaState& s, const OsakaEventState& L) { OsakaDownhillRowsV1::draw(c, s, L); }

}
