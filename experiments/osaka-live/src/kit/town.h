#pragma once
#include "../world.h"
#include "primitives.h"

namespace Journey::Kit {
struct OsakaNearHouseV1 {
    static constexpr const char* name = "osaka-near-house-v1";
    static void draw(Ctx& c, const OsakaState& s, Canvas& cv, double x0, double x1, Col wall, Col rf, Col rf2);
};
}
