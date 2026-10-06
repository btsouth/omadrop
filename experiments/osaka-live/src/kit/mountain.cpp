#include "parameters.h"
#include "mountain.h"
#include "../osaka_shaders.h"

namespace Journey {
void Kit::OsakaMountainV1::draw(Ctx& c, const MountainLook& m) {
    const auto& p = osakaParameters().mountain;
    GpuProfile::Group profileGroup(c.gpu.profile,"drawMountain");
    Canvas& cv = c.canvas();
    c.retain(cv, "mountain", [&](Canvas& cv) {
        const double h = m.base - m.peak;
        cv.linear(0, m.peak, 0, m.base, {{0, m.top, 1}, {float(p.gradientStop), mix(m.top, m.bot, p.gradientMix), 1}, {1, m.bot, 1}});
        cv.moveTo(m.px - m.width * p.span, m.base + m.foot);
        for (int i = 0; i <= p.samples; ++i) {
            const double dx = (i / double(p.samples) * 2 - 1) * m.width * p.span;
            const double ax = std::abs(dx);
            double y = m.peak + h * (1 - std::exp(-std::pow(ax / m.width, p.shapePower))) + p.rippleGain * h * std::sin((m.px + dx) / p.ripplePeriod);
            if (ax < m.width * p.summitWidth) y = m.peak + p.summitOffset * h / p.summitHeight + std::pow(ax / (m.width * p.summitWidth), 2) * p.summitCurve * h / p.summitHeight;
            cv.lineTo(m.px + dx, y);
        }
        cv.lineTo(m.px + m.width * p.span, m.base + m.foot);
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
