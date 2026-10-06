#include "parameters.h"
#include "light-wave.h"
#include "pane.h"

namespace Journey::Kit {
double OsakaPaneV1::level(const Ctx& c, const OsakaEventState& L, double onTime, int band, V2 centre) {
    const auto& p = osakaParameters().windows;
    const double on = onTime <= p.alwaysOnCutoff ? 1.0 : c.schedule->pane(c.t, onTime);
    if (on <= 0) return 0;
    // Absolute level keeps the room warm; the relative lift makes each pane
    // visibly answer its own frequency role in dense mixes.
    // A shared bass kick lets the whole town blink together on the beat.
    double level = on * (p.base - p.hush * L.hush + p.band * c.band(band) + p.lift * c.lift(band) + p.kick * c.kick(6));
    OsakaLightWaveV1::pane(c, L, on, centre, level);
    return level;
}
}
