#pragma once
#include "../world.h"

namespace Journey::Kit {
struct Shell {
    double burst;   // bloom time
    V2 at;          // bloom centre (mock coordinates, before parallax)
    double size;    // final radius scale
    int kind;       // 0 chrysanthemum, 1 peony, 2 ring, 3 golden willow
    Col a, b;       // star colour, then the colour it cools to
    double strength;
    double tilt;    // ring tilt / rotation seed
};

struct OsakaLegacyLife {
    double t = 0;
    double wind = 0;      // gust strength
    Surge surge;          // flock burst / rocket launch
    double bloom = -1;    // firework bloom time
    double look = 0;      // figures look up
    double hush = 0;      // the measured breakdown before the rocket
    double scale = 1;     // fallback surge is smaller
    std::vector<Shell> shells;
};
}
