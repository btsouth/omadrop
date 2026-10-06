#include "ridges.h"
#include "haze.h"

namespace Journey::Kit {
double ridgeY(int seed, double x, double base, double amp, double scale) {
    Rng r(uint64_t(seed) * 977 + 13);
    double y = 0, a = 1;
    for (int o = 0; o < 6; ++o) {
        const double ph = r.uni() * Tau;
        const double fr = (0.6 + r.uni() * 0.8) * std::pow(2.0, o) / scale;
        y += a * std::sin(x * fr + ph);
        a *= 0.5;
    }
    return base - amp * (0.5 + 0.5 * y / 1.6);
}

namespace {
void ridge(Ctx& c, const OsakaState& s, const OsakaRidgeSpecV1& sp, const std::string& key, int i, bool crest) {
    Canvas& cv = c.canvas();
    c.retain(cv, key, [&](Canvas& cv) {
        cv.linear(0, sp.base - sp.amp, 0, sp.base + 26,
                  {{0, sp.top, 1}, {0.62f, mix(sp.top, sp.bot, 0.22), 1}, {1, sp.bot, 1}});
        cv.moveTo(-20, 1080);
        for (double x = -20; x <= 1941; x += 4) {
            double y = ridgeY(sp.seed, x + s.cam * sp.par, sp.base, sp.amp, sp.scale);
            if (crest) y = ridgeY(sp.seed, x + s.cam * sp.par, sp.base + 6, 60, 300);
            cv.lineTo(x, y);
        }
        cv.lineTo(1941, 1080);
        cv.closePath();
        cv.fill();
    }, {s.cam});
    c.gpu.over(cv, 1, 0, float(s.land));
    hazeBand(c, sp.base + 26, 34, 0.25, 0.5, s.cam * sp.par + c.t * 6, 31 + i, Col(0.36f, 0.92f, 0.66f), 0.26 * s.land);
}
}

void OsakaRidgesV1::draw(Ctx& c, const OsakaState& s) {
    GpuProfile::Group profileGroup(c.gpu.profile,"ridges");
    const OsakaRidgeSpecV1 specs[3] = {
        {21, 622, 250, 620, 0.04, Col(0.050f, 0.300f, 0.220f), Col(0.30f, 0.84f, 0.60f)},
        {22, 640, 128, 330, 0.07, Col(0.036f, 0.215f, 0.160f), Col(0.24f, 0.74f, 0.52f)},
        {23, 656, 84, 210, 0.11, Col(0.024f, 0.140f, 0.105f), Col(0.17f, 0.60f, 0.42f)},
    };
    for (int i = 0; i < 3; ++i) ridge(c, s, specs[i], "ridge-" + std::to_string(i), i, i == 0);
}

void OsakaRidgesV1::draw(Ctx& c, const OsakaState& s, const std::vector<OsakaRidgeSpecV1>& ridges, const std::string& key) {
    GpuProfile::Group profileGroup(c.gpu.profile,"ridges");
    for (std::size_t i = 0; i < ridges.size(); ++i) ridge(c, s, ridges[i], "ridge-" + key + "-" + std::to_string(i), int(i), false);
}
}
