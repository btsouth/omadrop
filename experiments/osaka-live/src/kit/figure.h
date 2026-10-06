#pragma once
#include "../rig.h"
#include "../world.h"
#include "cloth.h"
namespace Journey::Kit {
// Absolute design-pixel targets enter the existing solver unchanged.
struct OsakaFigureV1 {
    static constexpr const char* name = "osaka-figure-v1";
    static void draw(Canvas& cv, const RigIn& pose, Col color) { drawBody(cv, solve(pose), color); }
    static void street(Canvas& cv, const RigIn& pose, Col cloth) { streetFigure(cv, pose, cloth); }
};
// Declarative four-knot clip; evaluation delegates to today's exact easing.
struct ActionWindow {
    double start, fadeIn, end, fadeOut;
    double at(double t) const { return window(t, start, fadeIn, end, fadeOut); }
    double gesture(const Ctx& c) const { return c.gesture(start, fadeIn, end, fadeOut); }
};
}
