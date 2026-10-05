#pragma once
#include "../world.h"
#include "../rig.h"

namespace Journey::Kit {
struct OsakaShellV1 {
    static constexpr const char* name = "osaka-shell-v1";
    double burst;   // bloom time
    V2 at;          // bloom centre (mock coordinates, before parallax)
    double size;    // final radius scale
    int kind;       // 0 chrysanthemum, 1 peony, 2 ring, 3 golden willow
    Col a, b;       // star colour, then the colour it cools to
    double strength;
    double tilt;    // ring tilt / rotation seed
};

using Shell = OsakaShellV1;

struct OsakaLifeV1 {
    static constexpr const char* name = "osaka-life-v1";
    double t = 0;
    double wind = 0;      // gust strength
    Surge surge;          // flock burst / rocket launch
    double bloom = -1;    // firework bloom time
    double look = 0;      // figures look up
    double hush = 0;      // the measured breakdown before the rocket
    double scale = 1;     // fallback surge is smaller
    std::vector<Shell> shells;
};
using OsakaEventState = OsakaLifeV1;

using OsakaLegacyLife = OsakaEventState;
struct OsakaEventsV1 {
    static constexpr const char* name = "osaka-events-v1";
    static OsakaEventState at(const Ctx& c);
};

}
