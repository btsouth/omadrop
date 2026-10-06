#pragma once
#include "../world.h"
#include "../rig.h"
#include "events.h"

namespace Journey::Kit {
struct OsakaStreetClothV1 {
    static constexpr const char* name = "osaka-street-cloth-v1";
    static void draw(Canvas& cv, const RigIn& r, Col cloth);
};

inline void streetFigure(Canvas& cv, const RigIn& r, Col cloth) { OsakaStreetClothV1::draw(cv, r, cloth); }

struct OsakaNorenV1 {
    static constexpr const char* name = "osaka-noren-v1";
    static void draw(Ctx& c, const OsakaState& s, const OsakaEventState& L, Canvas& l, double t, double yx);
};

struct OsakaIzakayaClothV1 {
    static constexpr const char* name = "osaka-izakaya-cloth-v1";
    static void draw(const OsakaEventState& L, Canvas& f, double t, double x0);
};

}
