#pragma once
#include "../world.h"
#include "../rig.h"
#include "events.h"

namespace Journey::Kit {
struct OsakaSkyLanternsV1 {
    static constexpr const char* name = "osaka-sky-lanterns-v1";
    static void draw(Ctx& c, const OsakaState& s, const OsakaEventState& L);
};

inline void skyLanterns(Ctx& c, const OsakaState& s, const OsakaEventState& L) { OsakaSkyLanternsV1::draw(c, s, L); }

struct OsakaCoupleLanternV1 {
    static constexpr const char* name = "osaka-couple-lantern-v1";
    static void draw(const OsakaEventState& L, Canvas& p, Canvas& l, double t, double bx);
};

}
