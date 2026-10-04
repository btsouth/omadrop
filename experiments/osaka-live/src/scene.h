#pragma once

#include <array>

namespace Journey {
struct Audio {
    std::array<double, 6> bands{};
    double bass = 0;
    double accent = 0;
    // Bounded 0..1 swell from a measured rise; not chorus/drop recognition.
    double surge = 0;
};
}
