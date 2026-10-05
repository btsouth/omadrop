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

struct OsakaNorenV1 {
    static constexpr const char* name = "osaka-noren-v1";
    static void draw(Ctx& c, const OsakaState& s, const OsakaLegacyLife& L, Canvas& l, double t, double yx);
};

}
