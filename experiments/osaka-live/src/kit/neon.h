#pragma once
#include "../world.h"
#include "palette.h"

namespace Journey::Kit {
struct OsakaNeonV1 {
    static constexpr const char* name = "osaka-neon-v1";
    static void submit(Ctx& c, Canvas& n, double neon);
    static double level(const Ctx& c, double t, double stutter);
    static void draw(Ctx& c, const OsakaState& s, Canvas& f, Canvas& n, double t, double x0, double& stutter);
};
}
