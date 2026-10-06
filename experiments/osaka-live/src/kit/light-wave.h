#pragma once
#include "../world.h"
#include "../rig.h"
#include "events.h"

namespace Journey::Kit {
struct OsakaLightWaveV1 {
    static constexpr const char* name = "osaka-light-wave-v1";
    static void pane(const Ctx& c, const OsakaEventState& L, double on, V2 centre, double& level);
};

struct OsakaFestoonLightWaveV1 {
    static constexpr const char* name = "osaka-festoon-light-wave-v1";
    static void apply(const OsakaEventState& L, double t, V2 at, double& lv);
};

}
