#pragma once
#include "../world.h"
#include "primitives.h"
#include "layout.h"

namespace Journey::Kit {
struct OsakaNearHouseV1 {
    static constexpr const char* name = "osaka-near-house-v1";
    static void draw(Ctx& c, const OsakaState& s, Canvas& cv, double x0, double x1, Col wall, Col rf, Col rf2);
};
struct OsakaRightHouse2V1 {
    static constexpr const char* name = "osaka-right-house-2-v1";
    static void draw(Ctx& c, const OsakaState& s, Canvas& cv, double x0, Col wall, Col rf, Col rf2);
};
struct OsakaRightHouse3V1 {
    static constexpr const char* name = "osaka-right-house-3-v1";
    static void draw(Ctx& c, const OsakaState& s, Canvas& cv, double x0, Col wall, Col rf, Col rf2);
};
struct OsakaDeckV1 {
    static constexpr const char* name = "osaka-near-house-deck-v1";
    static void draw(Ctx& c, const OsakaState& s, Canvas& f, double ox, Col roomCol, Col wall);
};
struct OsakaStreetV1 {
    static constexpr const char* name = "osaka-street-surface-v1";
    static void draw(Ctx& c, const OsakaState& s, Canvas& cv, double qx);
};
struct OsakaRailingV1 {
    static constexpr const char* name = "osaka-street-railing-v1";
    static void draw(Ctx& c, const OsakaState& s, Canvas& cv, double qx, double ox);
};
struct OsakaCartFrameV1 {
    static constexpr const char* name = "osaka-yatai-frame-v1";
    static void draw(Ctx& c, const OsakaState& s, Canvas& p, double yx);
};
struct OsakaNearMaskV1 {
    static constexpr const char* name = "osaka-near-house-mask-v1";
    static void draw(Ctx& c, const OsakaState& s, Canvas& mask, double ox, const NearPane (&U)[4]);
};
struct OsakaShamisenMaskV1 {
    static constexpr const char* name = "osaka-shamisen-mask-v1";
    static void draw(Ctx& c, const OsakaState& s, Canvas& mask, double x0);
};
}
