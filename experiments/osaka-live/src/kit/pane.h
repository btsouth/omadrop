#pragma once
#include "../world.h"
#include "events.h"

namespace Journey::Kit {
struct OsakaPaneV1 {
    static constexpr const char* name = "osaka-pane-v1";
    static double level(const Ctx& c, const OsakaEventState& L, double onTime, int band, V2 centre);
};
inline double paneLevel(const Ctx& c, const OsakaEventState& L, double onTime, int band, V2 centre) { return OsakaPaneV1::level(c, L, onTime, band, centre); }
}
