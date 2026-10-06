#pragma once
#include "../world.h"
#include "parameters.h"
namespace Journey {
namespace Kit {
struct OsakaHazeV1 {
    static constexpr const char* name = "osaka-haze-v1";
    static void draw(Ctx& c, double y0, double sigma, double lo, double hi, double shift, double seed, Col col,
              double gain, V2 noise);
};
}
inline void hazeBand(Ctx& c, double y0, double sigma, double lo, double hi, double shift, double seed, Col col,
              double gain, V2 noise = {Kit::osakaParameters().haze.noiseX, Kit::osakaParameters().haze.noiseY}) { Kit::OsakaHazeV1::draw(c, y0, sigma, lo, hi, shift, seed, col, gain, noise); }
}
