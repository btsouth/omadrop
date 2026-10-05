#pragma once
#include "../world.h"
#include "../rig.h"
#include "osaka-legacy.h"

namespace Journey::Kit {
struct OsakaFestoonV1 {
    static constexpr const char* name = "osaka-festoon-v1";
    static void draw(Ctx& c, const OsakaState& s, const OsakaLegacyLife& L, V2 b);
};

}
