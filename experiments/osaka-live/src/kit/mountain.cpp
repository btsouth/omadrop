#include "mountain.h"
#include "../osaka_shaders.h"

namespace Journey {
void Kit::OsakaMountainV1::draw(Ctx& c, const MountainLook& m) {
    GpuProfile::Group profileGroup(c.gpu.profile,"drawMountain");
    Canvas& cv = c.canvas();
    c.retain(cv, "mountain", [&](Canvas& cv) {
        const double h = m.base - m.peak;
        cv.linear(0, m.peak, 0, m.base, {{0, m.top, 1}, {0.62f, mix(m.top, m.bot, 0.22), 1}, {1, m.bot, 1}});
        cv.moveTo(m.px - m.width * 3.2, m.base + m.foot);
        for (int i = 0; i <= 160; ++i) {
            const double dx = (i / 160.0 * 2 - 1) * m.width * 3.2;
            const double ax = std::abs(dx);
            double y = m.peak + h * (1 - std::exp(-std::pow(ax / m.width, 1.35))) + 0.012 * h * std::sin((m.px + dx) / 23.0);
            if (ax < m.width * 0.079) y = m.peak + 4 * h / 236 + std::pow(ax / (m.width * 0.079), 2) * 6 * h / 236;
            cv.lineTo(m.px + dx, y);
        }
        cv.lineTo(m.px + m.width * 3.2, m.base + m.foot);
        cv.closePath();
        cv.fill();
        if (m.snow > 0.01) {
            // Snow cap with fingers, grown from the summit.
            const double sc = m.snowScale;
            const double k = m.snow;
            cv.color(m.snowCol, 1.0);
            const double py = m.peak + 3 * sc;
            auto P = [&](double x, double y) { return V2(m.px + x * sc, py + y * sc * k); };
            const V2 pts[] = {P(-22, 0), P(20, 0), P(52, 40), P(36, 30), P(28, 52), P(12, 30), P(2, 58),
                              P(-10, 32), P(-24, 50), P(-30, 28), P(-48, 38)};
            cv.moveTo(pts[0].x, pts[0].y);
            for (int i = 1; i < 11; ++i) cv.lineTo(pts[i].x, pts[i].y);
            cv.closePath();
            cv.fill();
        }
    }, {m.px, m.peak, m.base, m.width, m.foot, m.snow, m.snowScale, m.top.r, m.top.g, m.top.b, m.bot.r, m.bot.g, m.bot.b, m.snowCol.r, m.snowCol.g, m.snowCol.b});
    c.gpu.over(cv, 1, 0, float(m.alpha));
}
}
