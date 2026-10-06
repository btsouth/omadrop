#pragma once

#include "composition.h"
#include "onset.h"

namespace Journey::Kit {
struct GenericWindowV1 {
    static constexpr const char* name = "generic-window-v1";
    static double level(const Ctx&, const OsakaWindowNodeV1&);
    // The same band envelope for other pieces: band, optional steady base, kick and onset.
    static double envelope(const Ctx&, int band, bool always, bool kick, bool onset);
    static bool draw(Ctx&, const OsakaWindowNodeV1&);
};
}
