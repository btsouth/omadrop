#pragma once
#include "../world.h"
#include "primitives.h"
#include "pane.h"
#include "layout.h"

namespace Journey::Kit {
struct OsakaNearRoomV1 {
    static constexpr const char* name = "osaka-near-room-v1";
    static void draw(Ctx& c, const OsakaState& s, const OsakaLegacyLife& L, Canvas& w, double t, double ox, const NearPane (&U)[4], double (&lv)[4], double& room);
};
struct OsakaRightRoom2V1 {
    static constexpr const char* name = "osaka-right-room-2-v1";
    static void draw(Ctx& c, const OsakaState& s, const OsakaLegacyLife& L, Canvas& w, double x0);
};
struct OsakaRightRoom3V1 {
    static constexpr const char* name = "osaka-right-room-3-v1";
    static void draw(Ctx& c, const OsakaState& s, const OsakaLegacyLife& L, Canvas& w, double t, double x0, const UpperPane (&ups)[4], double& shamisenPane);
};
double flickerOn(double dt);
struct OsakaCartRoomV1 {
    static double level(const OsakaState& s, double t);
    static constexpr const char* name = "osaka-cart-room-v1";
    static void draw(Canvas& b, double yx, double yo);
};
}
