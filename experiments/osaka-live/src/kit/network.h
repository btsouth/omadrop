#pragma once
#include "../world.h"
#include "../rig.h"
#include "osaka-legacy.h"

namespace Journey::Kit {
struct Pole { double x, top, base, par, sc; };
inline const Pole POLES[3] = {{1474, 96, 946, 0.9, 1.0}, {1318, 560, 800, 0.5, 0.36}, {1158, 640, 760, 0.3, 0.2}};
inline const Pole LAST_POLE = {2240, 150, 946, 0.9, 1.0};
// A shorter pole on the pier carries the run down toward the water.
inline const Pole PIER_POLE = {2650, 470, 1000, 0.9, 0.62};

struct WireSpan { V2 p0, p1; double sag; };
using Spans = std::vector<std::array<WireSpan, 6>>;



struct OsakaWireNetworkV1 {
    static constexpr const char* name = "osaka-wire-network-v1";
    static Spans runs(double cam);
    static std::array<std::array<V2, 4>, 6> outRuns(double cam);
    static double sag(int seg, int i);
    static V2 at(const WireSpan& s, double u);
};
inline Spans wireRuns(double cam) { return OsakaWireNetworkV1::runs(cam); }
inline V2 wireAt(const WireSpan& s, double u) { return OsakaWireNetworkV1::at(s, u); }

struct BirdPlan { double land; int wire; double u; double face; double fromA, fromD; };

}
