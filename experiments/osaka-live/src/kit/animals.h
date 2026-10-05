#pragma once
#include "../world.h"
#include "../rig.h"
#include "osaka-legacy.h"

namespace Journey::Kit {
struct OsakaMothsV1 {
    static constexpr const char* name = "osaka-moths-v1";
    static void draw(Ctx& c, const OsakaState& s, double px);
};

struct OsakaRailCatV1 {
    static constexpr const char* name = "osaka-rail-cat-v1";
    static void draw(Ctx& c, const OsakaLegacyLife& L, Canvas& p, double t, double ox);
};

struct OsakaSillCatV1 {
    static constexpr const char* name = "osaka-sill-cat-v1";
    static void draw(const OsakaLegacyLife& L, Canvas& sh, double t, double ox);
};

struct OsakaVerandaCatV1 {
    static constexpr const char* name = "osaka-veranda-cat-v1";
    static void draw(Ctx& c, const OsakaState& s, const OsakaLegacyLife& L, Canvas& f, double t, double ox);
};

}
