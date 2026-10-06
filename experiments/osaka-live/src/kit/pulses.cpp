#include "pulses.h"
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
void OsakaPulseStreamV1::draw(Ctx& c, const OsakaState& s, Canvas& l, const Spans& spans, const std::array<std::array<V2, 4>, 6>& outs, const std::function<double(int,double)>& dipAt, const std::array<double,16>& tailFade, double land, bool far) {
    // Light travelling the strands: one stream per frequency role, faster
    // and brighter as its band rises.
    Rng rng(101);
    for (int i = 0; i < 6; ++i) {
        // Put the stronger band response into moving accents, rather than
        // lighting an entire span. Silence retains the same ambient speed.
        const double travel = (c.score ? c.score->strandPhase[i] : 0);
        const double level = 0.55 + 0.8 * c.band(i) + 0.7 * c.lift(i);
        for (int k = 0; k < 6 + i / 2; ++k) {
            const double u = std::fmod(rng.uni() + travel, 1.0) * 4.0;
            const int si = int(u);
            const double tt = u - si;
            const double al = si < 2 ? 1.0 : 0.22 * land;
            const double lvl = std::min(1.4, (0.55 + 0.45 * rng.uni()) * level) * al;
            if (lvl < 0.02 || (si >= 2) != far) continue;
            const WireSpan& w = spans[std::size_t(si)][std::size_t(i)];
            // Consume the same RNG values, but skip geometry that cannot
            // touch the framebuffer, including its largest 33 px halo.
            if (std::min(w.p0.x, w.p1.x) > 1968 || std::max(w.p0.x, w.p1.x) < -48) continue;
            const double sc = std::array<double, 4>{1.0, 1.0, 0.35, 0.25}[std::size_t(si)];
            const double tail = (0.085 - 0.008 * i) / std::max(0.45, std::abs(w.p1.x - w.p0.x) / 900);
            for (int j = 0; j < 16; ++j) {
                const double tj = tt - tail * j / 16;
                if (tj < 0) break;
                V2 q = wireAt(w, tj);
                if (si == 1) q.y += dipAt(i, tj);
                l.disc(q.x, q.y, (2.4 - 1.6 * j / 16) * sc * (0.8 + 0.5 * lvl), PULSE[i], tailFade[j] * std::min(1.0, lvl));
            }
            V2 q = wireAt(w, tt);
            if (si == 1) q.y += dipAt(i, tt);
            l.glow(q.x, q.y, (13 + 14 * lvl) * sc, PULSE[i], 0.9 * std::min(1.0, lvl));
        }
        Rng quayRng(1012 + i);
        const double quayTravel=travel*1.4;
        for (int k = 0; k < 6; ++k) {
            Rng& pulseRng = k < 4 ? rng : quayRng;
            const double u = std::fmod(pulseRng.uni() + quayTravel, 1.0) * 3.0;
            const int seg = int(u);
            const double tt = u - seg;
            const double lvl = std::min(1.4, (0.55 + 0.45 * pulseRng.uni()) * level) * (seg == 0 ? 1.0 : s.outAlpha);
            if (lvl < 0.02 || far) continue;
            const V2 p0 = outs[std::size_t(i)][std::size_t(seg)], p1 = outs[std::size_t(i)][std::size_t(seg + 1)];
            if (std::min(p0.x, p1.x) > 1968 || std::max(p0.x, p1.x) < -48) continue;
            const WireSpan w{p0, p1, OsakaWireNetworkV1::sag(seg, i)};
            for (int j = 0; j < 16; ++j) {
                const double tj = tt - 0.07 * j / 16;
                if (tj < 0) break;
                const V2 q = wireAt(w, tj);
                l.disc(q.x, q.y, (2.4 - 1.6 * j / 16) * (0.8 + 0.5 * lvl), PULSE[i], tailFade[j] * std::min(1.0, lvl));
            }
            const V2 q = wireAt(w, tt);
            l.glow(q.x, q.y, 13 + 14 * lvl, PULSE[i], 0.9 * std::min(1.0, lvl));
        }
    }
}

}
