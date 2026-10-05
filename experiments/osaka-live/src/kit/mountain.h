#pragma once
#include "../world.h"
namespace Journey {
struct MountainLook {
    static constexpr const char* profile = "osaka-mountain-v1";
    double px, peak, base, width;
    Col top, bot;
    double snow = 0, snowScale = 1, foot = 30;
    Col snowCol;
    double alpha = 1;
};
namespace Kit {
struct OsakaMountainV1 {
    static constexpr const char* name = "osaka-mountain-v1";
    static void draw(Ctx& c, const MountainLook& m);
};
}
inline void drawMountain(Ctx& c, const MountainLook& m) { Kit::OsakaMountainV1::draw(c, m); }
}
