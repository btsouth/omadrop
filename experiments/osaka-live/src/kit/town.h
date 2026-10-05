#pragma once
#include "../world.h"
#include "primitives.h"

namespace Journey::Kit {
struct OsakaNearHouseV1 {
    static constexpr const char* name = "osaka-near-house-v1";
    static void draw(Ctx& c, const OsakaState& s, Canvas& cv, double x0, double x1, Col wall, Col rf, Col rf2);
};
struct OsakaRightHouse2V1 {
    static constexpr const char* name = "osaka-right-house-2-v1";
    static void draw(Ctx& c, const OsakaState& s, Canvas& cv, double x0, Col wall, Col rf, Col rf2);
};
struct OsakaRightHouse3V1 {
    static constexpr const char* name = "osaka-right-house-3-v1";
    static void draw(Ctx& c, const OsakaState& s, Canvas& cv, double x0, Col wall, Col rf, Col rf2);
};
}
