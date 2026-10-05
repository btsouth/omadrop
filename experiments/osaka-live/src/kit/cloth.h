#pragma once
#include "../world.h"
#include "../rig.h"
#include "osaka-legacy.h"

namespace Journey::Kit {
struct OsakaStreetClothV1 {
    static constexpr const char* name = "osaka-street-cloth-v1";
    static void draw(Canvas& cv, const RigIn& r, Col cloth);
};

inline void streetFigure(Canvas& cv, const RigIn& r, Col cloth) { OsakaStreetClothV1::draw(cv, r, cloth); }

}
