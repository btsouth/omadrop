#pragma once
#include "../world.h"
namespace Journey::Kit {
double ridgeY(int seed, double x, double base, double amp, double scale);
struct OsakaRidgesV1 {
    static constexpr const char* name = "osaka-ridges-v1";
    static void draw(Ctx& c, const OsakaState& s);
};
}
