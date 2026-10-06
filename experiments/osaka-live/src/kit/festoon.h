#pragma once
#include "../world.h"
#include "../rig.h"
#include "events.h"

namespace Journey::Kit {
struct OsakaFestoonV1 {
    static constexpr const char* name = "osaka-festoon-v1";
    static void draw(Ctx& c, const OsakaState& s, const OsakaEventState& L, V2 b);
};

}
