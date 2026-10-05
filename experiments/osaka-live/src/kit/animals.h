#pragma once
#include "../world.h"
#include "../rig.h"
#include "osaka-legacy.h"

namespace Journey::Kit {
struct OsakaMothsV1 {
    static constexpr const char* name = "osaka-moths-v1";
    static void draw(Ctx& c, const OsakaState& s, double px);
};

}
