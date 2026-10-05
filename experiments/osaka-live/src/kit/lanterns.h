#pragma once
#include "../world.h"
#include "../rig.h"
#include "osaka-legacy.h"

namespace Journey::Kit {
struct OsakaPaperLanternV1 {
    static constexpr const char* name = "osaka-paper-lantern-v1";
    static void draw(Canvas& light, V2 at, double rx, double ry, double tilt, Col paper, double bright);
};

inline void paperLantern(Canvas& light, V2 at, double rx, double ry, double tilt, Col paper, double bright) { OsakaPaperLanternV1::draw(light, at, rx, ry, tilt, paper, bright); }

struct OsakaLanternV1 {
    static constexpr const char* name = "osaka-lantern-v1";
    static void draw(Canvas& body, Canvas& light, V2 hand, double swing, double size, double bright);
};

inline void lantern(Canvas& body, Canvas& light, V2 hand, double swing, double size, double bright) { OsakaLanternV1::draw(body, light, hand, swing, size, bright); }

}
