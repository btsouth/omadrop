#pragma once
#include "events.h"
#include "figure.h"
namespace Journey::Kit {
struct OsakaWomanFanV1 {
    static constexpr const char* name = "osaka-womanfan-v1";
    static void draw(Ctx& c, const OsakaState& s, const OsakaEventState& L, Canvas& f, double t, double ox);
};
}
