#pragma once
#include "../world.h"
#include "../rig.h"
#include "events.h"

namespace Journey::Kit {
struct OsakaWisteriaV1 {
    static constexpr const char* name = "osaka-wisteria-v1";
    static void draw(Ctx& c, const OsakaState& s, const OsakaEventState& L);
};

inline void wisteria(Ctx& c, const OsakaState& s, const OsakaEventState& L) { OsakaWisteriaV1::draw(c, s, L); }

}
