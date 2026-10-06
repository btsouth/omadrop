#pragma once
#include "../world.h"
#include "../rig.h"
#include "events.h"

namespace Journey::Kit {
struct OsakaMothsV1 {
    static constexpr const char* name = "osaka-moths-v1";
    static void draw(Ctx& c, const OsakaState& s, double px);
};

struct OsakaRailCatV1 {
    static constexpr const char* name = "osaka-rail-cat-v1";
    static void draw(Ctx& c, const OsakaEventState& L, Canvas& p, double t, double ox);
};

struct OsakaSillCatV1 {
    static constexpr const char* name = "osaka-sill-cat-v1";
    static void draw(const OsakaEventState& L, Canvas& sh, double t, double ox);
};

struct OsakaVerandaCatV1 {
    static constexpr const char* name = "osaka-veranda-cat-v1";
    static void draw(Ctx& c, const OsakaState& s, const OsakaEventState& L, Canvas& f, double t, double ox);
};

}
