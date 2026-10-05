#include "network.h"
#include "palette.h"
#include "primitives.h"
#include "pane.h"
#include "haze.h"
#include "../rig.h"
#include "../osaka_shaders.h"
#include <cmath>
#include <vector>

namespace Journey::Kit {
using Life = OsakaLegacyLife;
Spans OsakaWireNetworkV1::runs(double cam) {
    std::vector<std::array<V2, 6>> pts;
    std::array<V2, 6> a, b;
    // The run comes over the near house roof and is fixed to its wall
    // brackets, so it moves with the house, not the screen.
    for (int i = 0; i < 6; ++i) a[i] = {-60 - cam * 1.05, 300 + i * 17.0};
    pts.push_back(a);
    for (int i = 0; i < 6; ++i) b[i] = {534 - cam * 1.05, 318 + i * 15.5};
    pts.push_back(b);
    for (const Pole& p : POLES) {
        std::array<V2, 6> q;
        for (int i = 0; i < 6; ++i) q[i] = {p.x - cam * p.par, p.top + 26 * p.sc + i * 15.5 * p.sc};
        pts.push_back(q);
    }
    std::array<V2, 6> e;
    for (int i = 0; i < 6; ++i) e[i] = {1020 - cam * 0.2, 664 + i * 1.6};
    pts.push_back(e);
    Spans spans;
    for (std::size_t k = 0; k + 1 < pts.size(); ++k) {
        std::array<WireSpan, 6> s;
        const double L = std::abs(pts[k + 1][0].x - pts[k][0].x);
        for (int i = 0; i < 6; ++i) s[i] = {pts[k][i], pts[k + 1][i], L * (0.075 + 0.006 * i)};
        spans.push_back(s);
    }
    return spans;
}


std::array<std::array<V2, 4>, 6> OsakaWireNetworkV1::outRuns(double cam) {
    // Per strand: main pole, quay pole, pier pole, touchdown on the sea.
    std::array<std::array<V2, 4>, 6> r;
    for (int i = 0; i < 6; ++i) {
        r[i][0] = {POLES[0].x - cam * 0.9, POLES[0].top + 26 + i * 15.5};
        r[i][1] = {LAST_POLE.x - cam * 0.9, LAST_POLE.top + 26 + i * 15.5};
        r[i][2] = {PIER_POLE.x - cam * 0.9, PIER_POLE.top + 26 * PIER_POLE.sc + i * 15.5 * PIER_POLE.sc};
        r[i][3] = {3060 + i * 34 - cam * 0.9, 742 + i * 31};
    }
    return r;
}

double OsakaWireNetworkV1::sag(int seg, int i) { return seg == 0 ? 70 + 5.0 * i : seg == 1 ? 46 + 3.0 * i : 40 - 3.0 * i; }

V2 OsakaWireNetworkV1::at(const WireSpan& s, double u) {
    return {s.p0.x + (s.p1.x - s.p0.x) * u, s.p0.y + (s.p1.y - s.p0.y) * u + s.sag * 4 * u * (1 - u)};
}

}
