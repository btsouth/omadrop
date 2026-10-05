#pragma once
#include "../world.h"
#include "../rig.h"
#include "osaka-legacy.h"

namespace Journey::Kit {
struct OsakaChimeV1 {
    static constexpr const char* name = "osaka-chime-v1";
    static void draw(Ctx& c, const OsakaLegacyLife& L, Canvas& f, double t, double ox);
};

}
