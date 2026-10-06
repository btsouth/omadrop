#include "light-wave.h"
#include "network.h"
#include "lanterns.h"
#include "festoon.h"
#include "palette.h"
#include "primitives.h"
#include "pane.h"
#include "haze.h"
#include "../rig.h"
#include "../osaka_shaders.h"
#include <cmath>
#include <vector>

namespace Journey::Kit {
using Life = OsakaEventState;
void OsakaFestoonV1::draw(Ctx& c, const OsakaState& s, const Life& L, V2 b) {
    GpuProfile::Group profileGroup(c.gpu.profile,"festoon");
    const double t = c.t;
    const V2 a(628 - s.cam * 1.05, 594);
    if (std::max(a.x, b.x) < -40 || std::min(a.x, b.x) > 1960) return;
    const WireSpan w{a, b, 64};
    Canvas& p = c.canvas();
    Canvas& l = c.canvas();
    std::vector<V2> pts;
    for (int k = 0; k <= 50; ++k) {
        V2 q = wireAt(w, k / 50.0);
        q.y += L.wind * 3 * std::sin(t * 2.3) * 4 * (k / 50.0) * (1 - k / 50.0);
        pts.push_back(q);
    }
    p.polyline(pts, 1.3, INK, 0.95);
    constexpr int N = 13;
    // Recent onsets with their running index: each onset lights the next
    // lantern, so a light steps along the string with the beat and bounces
    // back at the ends.
    struct Beat { double t, strength; int lantern; };
    std::vector<Beat> beats;
    if (c.score && !c.score->onsets.empty()) {
        const auto& on = c.score->onsets;
        for (auto it = on.rbegin(); it != on.rend(); ++it) {
            if (it->t > t) continue;
            if (t - it->t > 0.9) break;
            const int cyc=int(it->serial % (2*N-2));
            beats.push_back({it->t, it->strength, cyc < N ? cyc : 2 * N - 2 - cyc});
        }
    }
    const double kick = c.kick(5);
    for (int i = 0; i < N; ++i) {
        const double u = (i + 0.5) / N;
        V2 top = wireAt(w, u);
        top.y += L.wind * 3 * std::sin(t * 2.3) * 4 * u * (1 - u);
        const double sw = 0.07 * std::sin(t * 1.6 + i * 0.9) + L.wind * 0.35 * std::sin(t * 3.1 + i * 0.7);
        const double cord = 12 + 4 * (i % 2);
        const V2 at = top + V2(std::sin(sw) * cord, std::cos(sw) * cord + 7);
        p.line(top.x, top.y, at.x - std::sin(sw) * 7, at.y - 7, 0.9, INK);
        const double on = s.chapter ? c.schedule->pane(t,3.0+0.11*i+0.05*c.jit(700+i)) : 1.0;
        double lv = 0.30 - 0.14 * L.hush + 0.14 * c.band(i % 6) + 0.40 * kick;
        for (const Beat& b : beats) {
            const double d = std::abs(i - b.lantern);
            if (d < 1.5) lv += (d < 0.5 ? 1.7 : 0.35) * (0.5 + 0.5 * b.strength) * std::exp(-(t - b.t) * 4.0);
        }
        OsakaFestoonLightWaveV1::apply(L, t, at, lv);
        lv *= on;
        if (lv < 0.02) { paperLantern(l, at, 8, 10.5, sw * 0.6, Col(0.18f, 0.06f, 0.05f), 0.4); continue; }
        const Col paper = i % 3 == 1 ? Col(0.98f, 0.82f, 0.52f) : Col(0.96f, 0.30f, 0.20f);
        l.glow(at.x, at.y, 24 + 26 * std::min(lv, 1.8), mix(paper, WARM_T, 0.4), 0.32 * std::min(lv, 1.8));
        paperLantern(l, at, 8, 10.5, sw * 0.6, paper, std::min(1.5, lv));
    }
    c.gpu.over(p);
    c.gpu.over(l, 1.35f);
    c.gpu.add(l, 0.45f, 14);
}

}
