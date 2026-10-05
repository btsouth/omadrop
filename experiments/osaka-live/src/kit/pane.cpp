#include "pane.h"

namespace Journey::Kit {
double OsakaPaneV1::level(const Ctx& c, const OsakaLegacyLife& L, double onTime, int band, V2 centre) {
    const double on = onTime <= -50 ? 1.0 : c.schedule->pane(c.t, onTime);
    if (on <= 0) return 0;
    // Absolute level keeps the room warm; the relative lift makes each pane
    // visibly answer its own frequency role in dense mixes.
    // A shared bass kick lets the whole town blink together on the beat.
    double level = on * (0.74 - 0.20 * L.hush + 0.30 * c.band(band) + 0.62 * c.lift(band) + 0.22 * c.kick(6));
    for (const Shell& sh : L.shells) {
        const double arrive = sh.burst + (centre - sh.at).len() / 1500.0;
        if (c.t > arrive) level += 1.1 * sh.size * sh.strength * std::exp(-(c.t - arrive) * 3.2) * on;
    }
    return level;
}
}
