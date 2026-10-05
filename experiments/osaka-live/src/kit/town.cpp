#include "town.h"
#include "primitives.h"

namespace Journey::Kit {
void OsakaNearHouseV1::draw(Ctx& c, const OsakaState& s, Canvas& cv, double x0, double x1, Col wall, Col rf, Col rf2) {
    c.retain(cv, "near-house", [&](Canvas& cv) {
        cv.fillRect(x0, 250, x1 - x0, 830, wall);
        roof(cv, x0, x1, 262, 132, 64, rf, rf2, RIM, 11);
        cv.color(rf);
        cv.moveTo(x0, 556); cv.lineTo(x1 + 96, 580); cv.lineTo(x1 + 100, 596); cv.lineTo(x0, 596); cv.closePath();
        cv.fill();
        cv.line(x0, 556, x1 + 96, 580, 1.5, RIM, 0.45);
        cv.color(RIM, 0.07);
        for (double xx = x0 + 30; xx < x1 + 90; xx += 15) { cv.moveTo(xx, 558 + (xx - x0) * 0.037); cv.lineTo(xx + 4, 596); }
        cv.stroke(1);
        cv.line(x1, 276, x1, 578, 1.4, RIM, 0.28);
    }, {s.cam});
}
}
