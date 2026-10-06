#pragma once
#include "../world.h"
namespace Journey::Kit {
// Six-stop sunset ramp and paper wash ported from Journey's kanagawaSky.
// Positions and paints belong to the world, with no chapter or palette coupling.
struct GradientSkyParametersV1 {
    struct Stop { double y; Col color; };
    std::vector<Stop> stops;
    Col paperTop, paperBottom;
    double printGrade = 0, grain = 0, energyGrade = 0;
    bool cloudBands = false;
};
struct GradientSkyV1 {
    static constexpr const char* name = "gradient-sky-v1";
    static void draw(Ctx&, const GradientSkyParametersV1&);
};
}
