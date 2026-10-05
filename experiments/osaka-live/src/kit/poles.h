#include "network.h"
#pragma once
#include "../world.h"
#include "../rig.h"
#include "osaka-legacy.h"

namespace Journey::Kit {
struct OsakaPolesV1 {
    static constexpr const char* name = "osaka-poles-v1";
    static void draw(Ctx& c, const OsakaState& s, const OsakaLegacyLife& L, Canvas& cv, Canvas& l, Canvas& cone, Canvas* staticPoles, double t, double cam, double land, bool far);
};

}
