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

double GenericWindowV1::level(const Ctx& c, const OsakaWindowNodeV1& window) {
    double value = BandGain * c.band(window.band) + LiftGain * c.lift(window.band);
    if (window.always) value += Base;
    if (window.kick) value += KickGain * c.kick(6);
    if (window.onset) value += OnsetGain * onsetFlash(c, OnsetDecay, OnsetMaxAge);
    return value;
}

bool GenericWindowV1::draw(Ctx& c, const OsakaWindowNodeV1& window) {
    const double value = level(c, window);
    if (value <= 0.01) return false;
    const double alpha = std::min(1.0, 0.95 * std::min(value, 1.0) + 0.05);
    const float gain = float(0.82 + 0.30 * value);
    const auto& art = *osakaWorld().art;
    return art.fillGradient(c.canvas(), QString::fromStdString(window.id),
                            WARM_T * gain, WARM_B * gain, alpha);
}
}
