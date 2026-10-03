#include "caption.h"

#include <QPainter>
#include <QPainterPath>
#include <QRadialGradient>
#include <algorithm>
#include <charconv>
#include <cmath>
#include <cstring>

namespace Journey {
namespace {
#include "caption-assets/outlines.inc"

// The generator emits only absolute M/L/Q/C/Z commands with space-separated
// coordinates. This deliberately small reader is not a general SVG parser.
QPainterPath outline(const char* data) {
    QPainterPath path;
    path.setFillRule(Qt::WindingFill);
    const char* cursor = data;
    const char* end = data + std::strlen(data);
    bool valid = true;
    auto space = [&] { while (cursor != end && *cursor == ' ') ++cursor; };
    auto number = [&] {
        space();
        double value = 0;
        const auto result = std::from_chars(cursor, end, value);
        if (result.ec != std::errc{} || !std::isfinite(value)) valid = false;
        cursor = result.ptr;
        return value;
    };
    auto point = [&] {
        const double x = number();
        const double y = number();
        return QPointF(x, y);
    };
    while (cursor != end && valid) {
        space();
        if (cursor == end) break;
        switch (*cursor++) {
        case 'M': path.moveTo(point()); break;
        case 'L': path.lineTo(point()); break;
        case 'Q': {
            const auto control = point();
            const auto target = point();
            path.quadTo(control, target);
            break;
        }
        case 'C': {
            const auto first = point();
            const auto second = point();
            const auto target = point();
            path.cubicTo(first, second, target);
            break;
        }
        case 'Z': path.closeSubpath(); break;
        default: valid = false;
        }
    }
    return valid ? path : QPainterPath{};
}

double smooth(double value) {
    value = std::clamp(value, 0.0, 1.0);
    return value * value * (3 - 2 * value);
}
}

void paintThemeCaption(QPainter& painter, double seconds) {
    if (!std::isfinite(seconds)) return;
    double elapsed;
    if (seconds >= 0 && seconds < 5) elapsed = seconds;
    else if (seconds >= 35 && seconds < 40) elapsed = seconds - 35;
    else return;

    const double entrance = smooth(elapsed / .6);
    const double opacity = entrance * smooth(5 - elapsed);
    if (opacity <= 0) return;
    static const QPainterPath label = outline(themeLabelOutline);
    static const QPainterPath osaka = outline(osakaJadeOutline);
    static const QPainterPath kanagawa = outline(kanagawaOutline);

    painter.save();
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setOpacity(painter.opacity() * opacity);
    painter.translate(0, 8 * (1 - entrance));

    // A feathered local shadow has no rectangular edge or visible panel.
    painter.save();
    painter.translate(210, 804);
    painter.scale(1, .42);
    QRadialGradient backing(0, 0, 310);
    backing.setColorAt(0, QColor(5, 12, 13, 96));
    backing.setColorAt(.45, QColor(5, 12, 13, 60));
    backing.setColorAt(1, QColor(5, 12, 13, 0));
    painter.setPen(Qt::NoPen);
    painter.setBrush(backing);
    painter.drawEllipse(QPointF(0, 0), 310, 310);
    painter.restore();

    painter.translate(80, 774);
    painter.fillPath(label, QColor(212, 217, 203, 205));
    painter.translate(0, 50);
    painter.fillPath(seconds < 5 ? osaka : kanagawa, QColor(231, 227, 211));
    painter.restore();
}
}
