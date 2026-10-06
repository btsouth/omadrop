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
        f.fillRect(214 + ox, 890, 118, 9, roomCol, 0.9);
        for (double lx : {224.0, 322.0}) f.fillRect(lx + ox, 899, 7, 34, roomCol, 0.9);
        f.fillRect(214 + ox, 934, 256, 34, roomCol, 0.55);
        lattice(f, 70 + ox, 640, 134, 328, 3, 6, INK, 1.6);
        lattice(f, 346 + ox, 640, 124, 328, 3, 6, INK, 1.6);
        f.fillRect(-80 + ox, 968, 720, 14, INK);
        f.fillRect(-80 + ox, 982, 640, 98, wall);
        f.line(-80 + ox, 968, 640 + ox, 968, 1.2, RIM, 0.3);
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
        const double x0 = 575 + ox, x1 = 1262 + ox;
        for (double xx = x0; xx <= x1 + 1; xx += 62) cv.line(xx, 936, xx, 864, 5, INK);
        cv.line(x0, 866, x1, 866, 5, INK);
        cv.line(x0, 896, x1, 896, 3, INK);
        cv.line(x0, 863, x1, 863, 1.0, MINT, 0.5);
    }, {s.cam});
}
void OsakaCartFrameV1::draw(Ctx& c, const OsakaState& s, Canvas& p, double yx) {
    c.retain(p, "yatai-frame", [&](Canvas& p) {
        p.fillRect(yx, 858, 178, 62, INK);
        for (double wx : {yx + 34, yx + 146}) p.disc(wx, 918, 19, INK);
        for (double px : {yx + 6, yx + 172}) p.line(px, 860, px, 742, 5, INK);
        p.color(INK);
        p.moveTo(yx - 22, 748); p.curveTo(yx + 40, 716, yx + 138, 716, yx + 200, 748); p.lineTo(yx + 200, 757); p.lineTo(yx - 22, 757); p.closePath();
        p.fill();
        p.color(MINT, 0.45);
        p.moveTo(yx - 22, 748); p.curveTo(yx + 40, 716, yx + 138, 716, yx + 200, 748);
        p.stroke(1.2);
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
