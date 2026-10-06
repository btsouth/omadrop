#include "primitives.h"

namespace Journey::Kit {
void OsakaWarmPaneV1::draw(Canvas& cv, double x, double y, double w, double h, double level, const Col* tint) {
    if (level <= 0.01) return;
    const double a = std::min(1.0, 0.95 * std::min(level, 1.0) + 0.05);
    const float k = float(0.82 + 0.3 * level);
    const Col top = tint ? mix(*tint, INK, 0.12) : WARM_T;
    const Col bot = tint ? *tint : WARM_B;
    cv.linear(0, y, 0, y + h, {{0, top * k, float(a)}, {1, bot * k, float(a)}});
    cv.rect(x, y, w, h);
    cv.fill();
}

void OsakaDarkPaneV1::draw(Canvas& cv, double x, double y, double w, double h) { cv.fillRect(x, y, w, h, Col(0.03f, 0.105f, 0.082f)); }

void OsakaLatticeV1::draw(Canvas& cv, double x, double y, double w, double h, int cols, int rows, Col frame, double lw) {
    cv.color(frame, 1.0);
    for (int i = 1; i < cols; ++i) { cv.moveTo(x + w * i / cols, y); cv.lineTo(x + w * i / cols, y + h); }
    for (int j = 1; j < rows; ++j) { cv.moveTo(x, y + h * j / rows); cv.lineTo(x + w, y + h * j / rows); }
    cv.stroke(lw);
    cv.color(frame, 1.0);
    cv.rect(x, y, w, h);
    cv.stroke(lw * 2.2);
}

void OsakaRoofV1::draw(Canvas& cv, double x0, double x1, double ye, double yr, double ov, Col col, Col col2,
          Col rim, double th, bool tiles, double rimA) {
    const double ins = (ye - yr) * 1.15, dy = ye - yr;
    auto path = [&] {
        cv.moveTo(x0 - ov, ye);
        cv.curveTo(x0 - ov * 0.1, ye - dy * 0.25, x0 + ins * 0.55, yr + dy * 0.2, x0 + ins, yr);
        cv.lineTo(x1 - ins, yr);
        cv.curveTo(x1 - ins * 0.55, yr + dy * 0.2, x1 + ov * 0.1, ye - dy * 0.25, x1 + ov, ye);
        cv.lineTo(x1 + ov, ye + th);
        cv.lineTo(x0 - ov, ye + th);
        cv.closePath();
    };
    cv.linear(0, yr, 0, ye + th, {{0, col2, 1}, {1, col, 1}});
    path();
    cv.fill();
    if (tiles) {
        const int n = std::max(6, int((x1 - x0 + 2 * ov) / 13));
        cv.color(rim, 0.10);
        for (int k = 1; k < n; ++k) {
            const double f = double(k) / n;
            const double ax = x0 + ins + (x1 - x0 - 2 * ins) * f, bx = x0 - ov + (x1 - x0 + 2 * ov) * f;
            cv.moveTo(lerp(ax, bx, 0.03), lerp(yr, ye + th, 0.03));
            cv.lineTo(bx, ye + th);
        }
        cv.stroke(1.0);
    }
    cv.color(rim, rimA);
    cv.moveTo(x0 + ins - 6, yr - 3);
    cv.lineTo(x1 - ins + 6, yr - 3);
    cv.curveTo(x1 - ins * 0.55, yr + dy * 0.2, x1 + ov * 0.1, ye - dy * 0.25, x1 + ov, ye);
    cv.stroke(1.6);
    cv.line(x0 + ins - 8, yr, x1 - ins + 8, yr, 6, col);
}

}
