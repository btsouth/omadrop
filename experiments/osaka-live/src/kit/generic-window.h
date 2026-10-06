#pragma once

#include "composition.h"
#include "onset.h"

namespace Journey::Kit {
struct GenericWindowV1 {
    static constexpr const char* name = "generic-window-v1";
    static double level(const Ctx&, const OsakaWindowNodeV1&);
    static bool draw(Ctx&, const OsakaWindowNodeV1&);
};
}
