#pragma once
#include "../world.h"
#include "../rig.h"
#include "events.h"

namespace Journey::Kit {
struct OsakaGrassV1 {
    static constexpr const char* name = "osaka-grass-v1";
    static void draw(const OsakaEventState& L, Canvas& f, Rng& rng, double t, double ox);
};

struct OsakaGrassFlowersV1 {
    static constexpr const char* name = "osaka-grass-flowers-v1";
    static void draw(Canvas& az, Rng& rng, double ox);
};

}
