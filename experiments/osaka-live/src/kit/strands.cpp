#include "strands.h"
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
void OsakaStrandsV1::draw(Ctx& c, const OsakaState& s, const Life& L, Canvas& cv, Canvas& l, const Spans& spans, const std::array<std::array<V2, 4>, 6>& outs, const std::function<double(int,double)>& dipAt, double t, double land, bool far) {
    // Each strand has its own gauge (the bass strand heaviest) and a moonlit
    // upper edge; it hums with a soft glow that follows its band.
    const double gauge[6] = {2.1, 1.9, 1.6, 1.35, 1.15, 0.95};
    const double spanScale[6] = {1.0, 1.0, 0.88, 0.53, 0.35, 0.26};
    auto strand = [&](const std::vector<V2>& pts, int i, double sc, double alpha, bool out = false) {
        cv.polyline(pts, gauge[i] * sc, INK, 0.95 * alpha);
        std::vector<V2> rim(pts);
        for (V2& q : rim) q.y -= 0.55 * gauge[i] * sc;
        cv.polyline(rim, 0.6 * sc, RIM, 0.32 * alpha);
        // Bound the continuous core, not the travelling band accents. A drop
        // must not turn a whole conductor into an opaque white stroke.
        const double lift = c.lift(i);
        const double thump = (i < 2 ? 0.14 : 0.06) * c.kick(7);
        double hum = (0.035 + 0.11 * c.band(i) + 0.26 * lift + thump) / (1 + 0.7 * lift) * alpha * (far ? 0.20 : 1.0);
        double width = 1.25 + 0.35 * clamp01(lift);
        // Retain the established quay-to-water instrument as it enters view.
        // The house/moon span and diagonal valley bundle remain restrained.
        const double quay=out?0.25:0;
        hum = lerp(hum, std::min(0.6, (0.05 + 0.12 * c.band(i) + 0.46 * lift) * alpha), quay);
        width = lerp(width, 2.2 + 1.5 * lift, quay);
        if (hum > 0.01) l.polyline(pts, width * sc, i < 2 ? WARM_T : PULSE[i], hum);
    };
    for (std::size_t si = 0; si < spans.size(); ++si) {
        if ((si >= 2) != far) continue;
        const double al = si < 2 ? 1.0 : land;
        if (al < 0.01) continue;
        for (int i = 0; i < 6; ++i) {
            const WireSpan& w = spans[si][std::size_t(i)];
            if (std::min(w.p0.x, w.p1.x) > 1960 || std::max(w.p0.x, w.p1.x) < -40) continue;
            std::vector<V2> pts;
            for (int k = 0; k <= 60; ++k) {
                V2 q = wireAt(w, k / 60.0);
                if (si == 1) q.y += dipAt(i, k / 60.0);
                q.y += L.wind * 2.0 * std::sin(t * 2.1 + i) * 4 * (k / 60.0) * (1 - k / 60.0);
                pts.push_back(q);
            }
            strand(pts, i, spanScale[si], al);
        }
    }
    for (int i = 0; i < 6 && !far; ++i) {
        for (int seg = 0; seg < 3; ++seg) {
            const V2 p0 = outs[std::size_t(i)][std::size_t(seg)], p1 = outs[std::size_t(i)][std::size_t(seg + 1)];
            if (std::min(p0.x, p1.x) > 1960 || std::max(p0.x, p1.x) < -40) continue;
            std::vector<V2> pts;
            for (int k = 0; k <= 60; ++k) {
                V2 q = wireAt({p0, p1, OsakaWireNetworkV1::sag(seg, i)}, k / 60.0);
                q.y += L.wind * 2.0 * std::sin(t * 2.1 + i) * 4 * (k / 60.0) * (1 - k / 60.0);
                pts.push_back(q);
            }
            strand(pts, i, 1.0, seg == 0 ? 1.0 : s.outAlpha, true);
        }
    }
}

}
