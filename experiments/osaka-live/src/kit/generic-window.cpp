#include "generic-window.h"

#include "palette.h"
#include <algorithm>

namespace Journey::Kit {
namespace {
// These match Osaka's pane defaults and envelope arithmetic.
constexpr double Base = 0.74;
constexpr double BandGain = 0.30;
constexpr double LiftGain = 0.62;
constexpr double KickGain = 0.22;
constexpr double OnsetGain = 0.45;
constexpr double OnsetDecay = 8.0;
constexpr double OnsetMaxAge = 0.40;
}

double GenericWindowV1::envelope(const Ctx& c, int band, bool always, bool kick, bool onset) {
    double value = BandGain * c.band(band) + LiftGain * c.lift(band);
    if (always) value += Base;
    if (kick) value += KickGain * c.kick(6);
    if (onset) value += OnsetGain * onsetFlash(c, OnsetDecay, OnsetMaxAge);
    return value;
}

double GenericWindowV1::level(const Ctx& c, const OsakaWindowNodeV1& window) {
    return envelope(c, window.band, window.always, window.kick, window.onset);
}

bool GenericWindowV1::draw(Ctx& c, const OsakaWindowNodeV1& window) {
    const double value = level(c, window);
    if (value <= 0.01) return false;
    const double alpha = std::min(1.0, 0.95 * std::min(value, 1.0) + 0.05);
    const float gain = float(0.82 + 0.30 * value);
    const auto& art = *osakaWorld().art;
    Canvas& canvas = c.canvas();
    if (!art.fillGradient(canvas, QString::fromStdString(window.id),
                          WARM_T * gain, WARM_B * gain, alpha))
        return false;
    c.gpu.draw(canvas);
    return true;
}
}
