#include "downhill.h"
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
void OsakaDownhillRowsV1::draw(Ctx& c, const OsakaState& s, const Life& L) {
    GpuProfile::Group profileGroup(c.gpu.profile,"downhillRoofs");
    Rng rng(55);
    struct Row { double yb, sc, par; int n; Col wall, rf2; };
    const Row rows[3] = {{850, 0.40, 0.42, 10, Col(0.050f, 0.290f, 0.210f), Col(0.085f, 0.420f, 0.300f)},
                         {905, 0.56, 0.55, 8, Col(0.038f, 0.215f, 0.158f), Col(0.070f, 0.340f, 0.245f)},
                         {975, 0.76, 0.70, 6, Col(0.026f, 0.150f, 0.110f), Col(0.055f, 0.265f, 0.192f)}};
    for (int row = 0; row < 3; ++row) {
        const Row& R = rows[row];
        Canvas& cv = c.canvas();
        Canvas& cw = c.canvas();
        Canvas* staticRoofs = c.retainedBuilder(cv, "downhill-roofs-" + std::to_string(row), {s.cam});
        const Col rf = mix(R.wall, R.rf2, 0.3);
        double x = 540 - s.cam * R.par + rng.uni() * 40;
        for (int i = 0; i < R.n; ++i) {
            const double w = (150 + rng.uni() * 130) * R.sc, hgt = (120 + rng.uni() * 70) * R.sc;
            const double ye = R.yb - hgt + rng.normal() * 8, yr = ye - (36 + rng.uni() * 26) * R.sc;
            const bool antenna = rng.uni() < 0.5;
            if (staticRoofs) {
                staticRoofs->fillRect(x, ye, w, 400, R.wall);
                roof(*staticRoofs, x, x + w, ye, yr, 16 * R.sc, rf, R.rf2, MINT, 5 * R.sc, true, 0.30);
                if (antenna) {
                    staticRoofs->line(x + w * 0.3, yr, x + w * 0.3, yr - 30 * R.sc, 1.2, R.wall);
                    staticRoofs->line(x + w * 0.3 - 9 * R.sc, yr - 24 * R.sc, x + w * 0.3 + 9 * R.sc, yr - 24 * R.sc, 1.2, R.wall);
                }
            }
            const int nw = int(1 + rng.uni() * 3);
            for (int j = 0; j < nw; ++j) {
                const double r1 = rng.uni(), r2 = rng.uni(), r3 = rng.uni(), r4 = rng.uni();
                const bool tinted = rng.uni() >= 0.8;
                if (r1 >= 0.82) continue;
                const double wx = x + w * (0.12 + 0.6 * r2), wy = ye + (14 + r3 * 26) * R.sc;
                const double ww = (26 + r4 * 22) * R.sc, wh = 26 * R.sc;
                const int key = row * 100 + i * 7 + j;
                // Windows wake through the first twelve seconds; a few were already lit.
                const double h = hash2(key, 5.5 + c.seed);
                const double on = h < 0.14 ? -100 : 2.5 + 10.5 * hash2(key, 9.1 + c.seed);
                const double level = s.chapter || on < 0 ? paneLevel(c, L, on, key % 6, {wx, wy}) : 1.0;
                warmPane(cw, wx, wy, ww, wh, level, tinted ? &BCYAN : nullptr);
                if (level > 0.01) c.retain(cw, "downhill-lattice-" + std::to_string(key), [&](Canvas& cw) {
                    lattice(cw, wx, wy, ww, wh, 2, 2, R.wall, 1.0);
                }, {s.cam});
            }
            x += w + (8 + rng.uni() * 46) * R.sc;
        }
        c.gpu.over(cv, 1, 0, float(s.land));
        c.gpu.over(cw, 1.25f, 0, float(s.land));
        hazeBand(c, R.yb + 2, 26, 0.16, 0, 0, 0, Col(0.3f, 0.9f, 0.62f), s.land);
    }
}

}
