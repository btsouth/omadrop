#include "network.h"
#include <functional>
#pragma once
#include "../world.h"
#include "../rig.h"
#include "events.h"

namespace Journey::Kit {
struct OsakaStrandsV1 {
    static constexpr const char* name = "osaka-strands-v1";
    static void draw(Ctx& c, const OsakaState& s, const OsakaEventState& L, Canvas& cv, Canvas& l, const Spans& spans, const std::array<std::array<V2, 4>, 6>& outs, const std::function<double(int,double)>& dipAt, double t, double land, bool far);
};

}
