#include "town.h"
#include "primitives.h"
#include "world-art.h"

namespace Journey::Kit {
void OsakaNearHouseV1::draw(Ctx& c, const OsakaState& s, Canvas& cv, double x0, double x1, Col wall, Col rf, Col rf2) {
    c.retain(cv, "near-house", [&](Canvas& cv) {
        drawWorldArt(cv, "near-house-shell", x0 + 80);
        drawWorldArt(cv, "near-house-roof", x0 + 80);
        drawWorldArt(cv, "near-house-eaves", x0 + 80);
    }, {s.cam});
}
void OsakaRightHouse2V1::draw(Ctx& c, const OsakaState& s, Canvas& cv, double x0, Col wall, Col rf, Col rf2) {
    c.retain(cv, "right-house-2", [&](Canvas& cv) {
        drawWorldArt(cv, "right-house-2-shell", x0 - 1262);
        drawWorldArt(cv, "right-house-2-roof", x0 - 1262);
    }, {s.cam});
}
void OsakaRightHouse3V1::draw(Ctx& c, const OsakaState& s, Canvas& cv, double x0, Col wall, Col rf, Col rf2) {
    c.retain(cv, "right-house-3", [&](Canvas& cv) {
        drawWorldArt(cv, "right-house-3-shell", x0 - 1512);
        drawWorldArt(cv, "right-house-3-roof", x0 - 1512);
        drawWorldArt(cv, "right-house-3-eaves", x0 - 1512);
    }, {s.cam});
}
void OsakaDeckV1::draw(Ctx& c, const OsakaState& s, Canvas& f, double ox, Col roomCol, Col wall) {
    c.retain(f, "near-house-deck", [&](Canvas& f) {
        drawWorldArt(f, "near-house-deck", ox);
    }, {s.cam});
}
void OsakaStreetV1::draw(Ctx& c, const OsakaState& s, Canvas& cv, double qx) {
    c.retain(cv, "street-surface", [&](Canvas& cv) {
        cv.linear(0, 934, 0, 1080, {{0, Col(0.040f, 0.150f, 0.112f), 1}, {0.18f, Col(0.020f, 0.075f, 0.058f), 1}, {1, Col(0.008f, 0.024f, 0.020f), 1}});
        cv.rect(-10, 934, std::min(1930.0, qx) + 10, 146);
        cv.fill();
        cv.line(0, 934.5, std::min(1920.0, qx), 934.5, 1.2, MINT, 0.30);
    }, {s.cam});
}
void OsakaRailingV1::draw(Ctx& c, const OsakaState& s, Canvas& cv, double qx, double ox) {
    c.retain(cv, "street-railing", [&](Canvas& cv) {
        if (qx < 2120) {
            for (int k = 0; k < 5; ++k) {
                cv.fillRect(qx + k * 34, 934 + (k + 1) * 26, 36, 300, Col(0.016f + 0.004f * k, 0.060f + 0.012f * k, 0.047f + 0.009f * k));
                cv.line(qx + k * 34, 934.5 + (k + 1) * 26, qx + k * 34 + 36, 934.5 + (k + 1) * 26, 1.2, MINT, 0.25);
            }
            for (int k = 0; k < 3; ++k) cv.line(qx - 30 - k * 120, 936, qx - 30 - k * 120, 900, 9, INK);
        }
        drawWorldArt(cv, "street-railing", ox);
    }, {s.cam});
}
void OsakaCartFrameV1::draw(Ctx& c, const OsakaState& s, Canvas& p, double yx) {
    c.retain(p, "yatai-frame", [&](Canvas& p) {
        drawWorldArt(p, "yatai-frame", yx - 770);
    }, {s.cam});
}
void OsakaNearMaskV1::draw(Ctx& c, const OsakaState& s, Canvas& mask, double ox, const NearPane (&U)[4]) {
    c.retain(mask, "near-house-mask", [&](Canvas& mask) {
            for (const NearPane& q : U) mask.fillRect(q.x + ox, q.y, q.w, q.h, Col(1, 1, 1));
    }, {s.cam});
}
void OsakaShamisenMaskV1::draw(Ctx& c, const OsakaState& s, Canvas& mask, double x0) {
    c.retain(mask, "shamisen-mask", [&](Canvas& mask) {
            mask.fillRect(x0 + 30, 640, 96, 96, Col(1, 1, 1));
    }, {s.cam});
}
}
