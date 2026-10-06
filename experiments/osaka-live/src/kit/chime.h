#pragma once
#include "../world.h"
#include "../rig.h"
#include "events.h"

namespace Journey::Kit {
struct OsakaChimeV1 {
    static constexpr const char* name = "osaka-chime-v1";
    static void draw(Ctx& c, const OsakaEventState& L, Canvas& f, double t, double ox);
};

}
