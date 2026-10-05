#include "network.h"
#include <functional>
#pragma once
#include "../world.h"
#include "../rig.h"
#include "osaka-legacy.h"

namespace Journey::Kit {
struct OsakaPulseStreamV1 {
    static constexpr const char* name = "osaka-pulse-stream-v1";
    static void draw(Ctx& c, const OsakaState& s, Canvas& l, const Spans& spans, const std::array<std::array<V2, 4>, 6>& outs, const std::function<double(int,double)>& dipAt, const std::array<double,16>& tailFade, double land, bool far);
};

}
