#pragma once
#include "../world.h"
namespace Journey {
struct DiscLook {
    static constexpr const char* profile = "osaka-disc-v1";
    V2 pos;
    double r = 108;
    Col col, col2, halo, ring;
    double veil = 1, tex = 1, energy = 0.5;
    double haloA = 70, haloB = 40, haloC = 0.30, haloD = 0.2, haloFar = 0.13;
    double restRings = 1.0;
};
namespace Kit {
struct OsakaDiscV1 {
    static constexpr const char* name = "osaka-disc-v1";
    static void draw(Ctx& c, const DiscLook& d, double camForClouds);
};
}
inline void drawDisc(Ctx& c, const DiscLook& d, double camForClouds) { Kit::OsakaDiscV1::draw(c, d, camForClouds); }
}
