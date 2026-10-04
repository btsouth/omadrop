#pragma once

#include <array>

namespace Journey {
struct Audio {
    std::array<double, 6> bands{};
    // Three-second causal RMS before the shared quiet-music gain.
    double preGainLevel = 0;
    double bass = 0;
    double accent = 0;
    // Bounded 0..1 swell from a measured rise; not chorus/drop recognition.
    double surge = 0;
};
}
