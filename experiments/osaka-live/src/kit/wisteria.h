#pragma once
#include "../world.h"
#include "../rig.h"
#include "osaka-legacy.h"

namespace Journey::Kit {
struct OsakaWisteriaV1 {
    static constexpr const char* name = "osaka-wisteria-v1";
    static void draw(Ctx& c, const OsakaState& s, const OsakaLegacyLife& L);
};

inline void wisteria(Ctx& c, const OsakaState& s, const OsakaLegacyLife& L) { OsakaWisteriaV1::draw(c, s, L); }

}
