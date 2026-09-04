#include "adaptive_render_quality.h"

#include <cassert>
#include <cmath>
#include <iostream>
#include <limits>

int main() {
    AdaptiveRenderQuality quality;
    assert(quality.quality() == 1.0f);
    assert(!quality.observe(std::numeric_limits<double>::quiet_NaN(), 1.0,
                            false));
    assert(quality.level() == 0);

    for (int frame = 0; frame < 29; ++frame) {
        assert(!quality.observe(10.0, 1.0, false));
    }
    assert(quality.observe(10.0, 1.0, false));
    assert(quality.level() == 1);
    assert(std::abs(quality.quality() - 0.72f) < 0.001f);

    // Cooldown prevents a rapid second reduction even under severe load.
    for (int frame = 0; frame < 300; ++frame) {
        assert(!quality.observe(10.0, 1.0, false));
    }
    for (int frame = 0; frame < 29; ++frame) {
        assert(!quality.observe(10.0, 1.0, false));
    }
    assert(quality.observe(10.0, 1.0, false));
    assert(quality.level() == 2);
    assert(std::abs(quality.quality() - 0.50f) < 0.001f);

    // A pending transition may accumulate evidence but never changes quality
    // until the composition has settled.
    AdaptiveRenderQuality transitionQuality;
    for (int frame = 0; frame < 40; ++frame) {
        assert(!transitionQuality.observe(25.0, 2.0, true));
    }
    assert(transitionQuality.level() == 0);
    assert(transitionQuality.observe(10.0, 1.0, false));

    // Recovery is deliberately much slower than reduction, preventing visible
    // oscillation when a scene sits near the performance threshold.
    for (int frame = 0; frame < 300; ++frame) {
        assert(!quality.observe(1.0, 1.0, false));
    }
    for (int frame = 0; frame < 3599; ++frame) {
        assert(!quality.observe(1.0, 1.0, false));
    }
    assert(quality.observe(1.0, 1.0, false));
    assert(quality.level() == 1);

    std::cout << "adaptive render quality passed\n";
}
