#pragma once
#include "../world.h"
#include <string>
#include <vector>
namespace Journey::Kit {
double ridgeY(int seed, double x, double base, double amp, double scale);
// One background ridge. A world can list its own; Osaka's three are the default.
struct OsakaRidgeSpecV1 { int seed; double base, amp, scale, par; Col top, bot; };
struct OsakaRidgesV1 {
    static constexpr const char* name = "osaka-ridges-v1";
    static void draw(Ctx& c, const OsakaState& s);
    // The same drawing for ridges a world lists itself. `key` keeps their retained geometry apart.
    static void draw(Ctx& c, const OsakaState& s, const std::vector<OsakaRidgeSpecV1>& ridges, const std::string& key);
};
}
