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
void OsakaRightHouse2V1::draw(Ctx& c, const OsakaState& s, Canvas& cv, double x0, Col wall, Col rf, Col rf2) {
    c.retain(cv, "right-house-2", [&](Canvas& cv) {
        cv.fillRect(x0, 690, 252, 250, mix(wall, Col(0.03f, 0.13f, 0.10f), 0.5));
        roof(cv, x0, x0 + 250, 690, 628, 22, mix(rf, Col(0.03f, 0.14f, 0.105f), 0.5), mix(rf2, Col(0.10f, 0.38f, 0.28f), 0.4), RIM, 7);
    }, {s.cam});
}
void OsakaRightHouse3V1::draw(Ctx& c, const OsakaState& s, Canvas& cv, double x0, Col wall, Col rf, Col rf2) {
    c.retain(cv, "right-house-3", [&](Canvas& cv) {
        cv.fillRect(x0, 600, 470, 340, wall);
        roof(cv, x0, x0 + 470, 600, 512, 34, rf, rf2, RIM, 9);
        cv.color(rf);
        cv.moveTo(x0 - 44, 770); cv.lineTo(x0 + 480, 770); cv.lineTo(x0 + 480, 786); cv.lineTo(x0 - 52, 786); cv.closePath();
        cv.fill();
        cv.line(x0 - 44, 770, x0 + 480, 770, 1.4, RIM, 0.4);
    }, {s.cam});
}
}
