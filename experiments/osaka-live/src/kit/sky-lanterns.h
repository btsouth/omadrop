#pragma once
#include "../world.h"
#include "../rig.h"
#include "osaka-legacy.h"

namespace Journey::Kit {
struct OsakaSkyLanternsV1 {
    static constexpr const char* name = "osaka-sky-lanterns-v1";
    static void draw(Ctx& c, const OsakaState& s, const OsakaLegacyLife& L);
};

inline void skyLanterns(Ctx& c, const OsakaState& s, const OsakaLegacyLife& L) { OsakaSkyLanternsV1::draw(c, s, L); }

}
