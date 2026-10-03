#include "scene.h"

#include <QPainterPath>
#include <QLinearGradient>
#include <QPolygonF>
#include <algorithm>
#include <cmath>
#include <vector>

namespace Journey {
namespace {
constexpr double W = 1600, H = 900, pi = 3.141592653589793;
double clamp(double x) { return std::clamp(x, 0.0, 1.0); }
double ease(double x) { x = clamp(x); return x * x * (3 - 2 * x); }
double mix(double a, double b, double x) { return a + (b - a) * x; }
double hash(int n) { return std::fmod(std::sin(n * 127.1 + 19.73) * 43758.5453 + 43759, 1.0); }
QColor color(const char* hex) { return QColor(QString::fromLatin1(hex)); }
QColor blend(QColor a, QColor b, double x) {
    return QColor::fromRgbF(mix(a.redF(), b.redF(), x), mix(a.greenF(), b.greenF(), x), mix(a.blueF(), b.blueF(), x));
}
QColor alpha(QColor c, double a) { c.setAlphaF(clamp(a)); return c; }
void line(QPainter& p, QPointF a, QPointF b, QColor c, double width = 1) {
    p.setPen(QPen(c, width, Qt::SolidLine, Qt::RoundCap)); p.drawLine(a, b);
}
void polygon(QPainter& p, std::initializer_list<QPointF> points, QColor fill, QColor stroke = Qt::transparent) {
    p.setBrush(fill); p.setPen(QPen(stroke, 1)); p.drawPolygon(QPolygonF(QVector<QPointF>(points)));
}

struct Building { double x, width, roof, bottom; int seed; };
std::vector<Building> buildings(int layer) {
    std::vector<Building> result;
    double x = -30;
    for (int i = 0; x < W + 40; i++) {
        const int seed = layer * 119 + i * 7;
        const double width = 43 + hash(seed + 2) * (layer == 0 ? 67 : 105);
        const double bottom = 455 + layer * 62;
        const double h = 40 + hash(seed + 6) * (layer == 0 ? 90 : 225);
        result.push_back({x, width, bottom - h, bottom, seed});
        x += width + 12 + hash(seed + 3) * 24;
    }
    return result;
}
const std::vector<Building>& blocks(int layer) {
    static const std::vector<Building> rows[] = {buildings(0), buildings(1), buildings(2)};
    return rows[layer];
}

double waterY(double x, int row, double t, const Audio& a) {
    const double phase = x * .0045 + row * .87 - t * .11;
    const double swell = (13 + row * 2.8) * (1 + .45 * a.bass);
    return 535 + row * 37 + swell * std::sin(phase) + 9 * std::sin(x * .010 + row * .8 + t * .06);
}

// The same skyline vertices become wave contours, rather than cutting to a second image.
QPainterPath shoreline(int layer, double transition, double time, const Audio& a) {
    QPainterPath path;
    path.moveTo(-20, H + 20);
    for (double x = -20; x <= W + 20; x += 3) {
        double city = 455 + layer * 62;
        for (const auto& b : blocks(layer)) if (x >= b.x && x <= b.x + b.width) { city = b.roof; break; }
        const double local = ease((transition - .14 * std::sin(x * .002 + layer)) / .86);
        path.lineTo(x, mix(city, waterY(x, layer, time, a), local));
    }
    path.lineTo(W + 20, H + 20); path.closeSubpath(); return path;
}

void sky(QPainter& p, double t, double journey, const Audio& a) {
    const QColor jade = color("#233831"), ink = color("#1f1f28");
    QLinearGradient bg(0, 0, 0, H);
    bg.setColorAt(0, blend(color("#101c1b"), color("#171b2c"), journey));
    bg.setColorAt(.6, blend(jade, color("#333a50"), journey));
    bg.setColorAt(1, blend(color("#142b26"), color("#161c30"), journey));
    p.fillRect(QRectF(0, 0, W, H), bg);

    // A fixed landmark carries through both worlds.
    p.setPen(Qt::NoPen);
    p.setBrush(blend(color("#d9d5ba"), color("#dcd7ba"), journey));
    p.drawEllipse(QPointF(1218, 192), 89, 89);
    p.setBrush(alpha(blend(jade, ink, journey), .12));
    for (int i = 0; i < 21; i++) p.drawRect(QRectF(1129, 120 + i * 7, 179, 1));
    for (int i = 0; i < 210; i++) {
        const double x = hash(i * 3) * W, y = 35 + hash(i * 3 + 1) * 330;
        const double activity = a.bands[(i / 35) % 6];
        p.setBrush(alpha(color("#c1c497"), .1 + activity * .19));
        const double size = i % 17 == 0 ? 2.5 : 1;
        p.drawRect(QRectF(x, y, size, size));
    }
    // Long horizontal strokes are drawn as part of the skyline's typography.
    for (int i = 0; i < 8; i++) {
        const double y = 96 + i * 44;
        const double x = 150 + i * 75 + std::sin(t * .025 + i) * 10;
        line(p, {x, y}, {x + 95 + i * 23, y}, alpha(color("#90b99f"), .12 * (1 - journey)), .8);
    }
    for (int row = 0; row < 3; row++) {
        QPainterPath mountains; mountains.moveTo(0, H);
        mountains.lineTo(0, 446 + row * 26);
        for (int i = 0; i < 34; i++) {
            const double x = i * 52;
            const double y = 437 + row * 26 - hash(i + row * 36) * 45;
            mountains.lineTo(x, y);
        }
        mountains.lineTo(W, H); mountains.closeSubpath();
        p.fillPath(mountains, alpha(blend(color("#254c40"), color("#2a3448"), journey), .38));
    }
}

void facade(QPainter& p, const Building& b, int layer, double t, double tr, const Audio& a) {
    const double remain = 1 - ease(tr * 1.8);
    if (remain <= 0) return;
    const QColor stroke = alpha(color("#80a98b"), (.16 + layer * .08) * remain);
    p.setOpacity(remain);
    const double depth = 10 + layer * 5;
    polygon(p, {{b.x + b.width, b.roof}, {b.x + b.width + depth, b.roof - depth * .6},
                {b.x + b.width + depth, b.bottom}, {b.x + b.width, b.bottom}}, color("#102c27"));
    polygon(p, {{b.x, b.roof}, {b.x + depth, b.roof - depth * .6},
                {b.x + b.width + depth, b.roof - depth * .6}, {b.x + b.width, b.roof}}, color("#36584a"));
    line(p, {b.x, b.roof}, {b.x, b.bottom}, stroke);
    line(p, {b.x + b.width, b.roof}, {b.x + b.width, b.bottom}, stroke);
    line(p, {b.x, b.roof}, {b.x + b.width, b.roof}, stroke);
    const int floors = std::max(1, int((b.bottom - b.roof - 14) / 21));
    const int cols = std::max(1, int((b.width - 16) / 16));
    for (int floor = 0; floor < floors; floor++) {
        const double y = b.roof + 13 + floor * 21;
        if (floor % 3 == 0) line(p, {b.x + 4, y + 14}, {b.x + b.width - 4, y + 14}, alpha(stroke, .25));
        for (int col = 0; col < cols; col++) {
            const double x = b.x + 9 + col * (b.width - 16) / cols;
            const int id = b.seed + floor * 17 + col;
            const double level = a.bands[(col + floor + layer) % 6];
            const double lit = clamp((level - hash(id) * .5) * 1.5);
            const QColor dark = color("#152c25");
            const QColor warm = id % 7 == 0 ? color("#c69581") : color("#b5ca9b");
            p.fillRect(QRectF(x, y, 6 + layer, 10), blend(dark, warm, .14 + lit * .75));
            if (col % 3 == 0) line(p, {x + 3, y}, {x + 3, y + 10}, alpha(color("#142b26"), .7));
        }
    }
    // Antennae, bridges and rooftop gardens break up the rectangular rhythm.
    if (b.seed % 3 == 0) {
        line(p, {b.x + b.width * .65, b.roof}, {b.x + b.width * .65, b.roof - 24}, stroke);
        line(p, {b.x + b.width * .65 - 9, b.roof - 16}, {b.x + b.width * .65 + 9, b.roof - 16}, stroke);
    }
    if (b.seed % 4 == 0) for (int i = 0; i < 3; i++) {
        p.setBrush(color("#63956d")); p.setPen(Qt::NoPen);
        p.drawEllipse(QPointF(b.x + 12 + i * 12, b.roof - 5), 4, 5 + 2 * a.bands[3]);
    }
    if (layer > 0 && b.seed % 5 == 0) {
        const double x = b.x + b.width - 16, y = b.roof + 18;
        const QColor sign = b.seed % 2 ? color("#8cd3cb") : color("#d2689c");
        p.fillRect(QRectF(x - 3, y - 4, 15, 48), color("#102820"));
        p.setPen(QPen(alpha(sign, .65 + a.bands[4] * .25), 1.5));
        for (int i = 0; i < 3; i++) {
            p.drawLine(QPointF(x, y + i * 13), QPointF(x + 6, y + i * 13));
            p.drawLine(QPointF(x + 3, y + i * 13 - 2), QPointF(x + 3, y + i * 13 + 7));
            p.drawLine(QPointF(x, y + i * 13 + 5), QPointF(x + 6, y + i * 13 + 5));
        }
    }
    if (layer > 0 && b.seed % 3 == 1) for (int floor = 2; floor < floors; floor += 3) {
        const double y = b.roof + floor * 21 + 11;
        p.fillRect(QRectF(b.x + 3, y, b.width - 6, 3), color("#102c25"));
        line(p, {b.x + 3, y - 5}, {b.x + b.width - 3, y - 5}, stroke, .8);
        for (int col = 0; col < 5; col++) {
            const double x = b.x + 5 + col * (b.width - 10) / 4;
            line(p, {x, y - 5}, {x, y + 2}, stroke, .8);
        }
    }
    p.setOpacity(1);
    Q_UNUSED(t);
}

void farCity(QPainter& p, double t, double tr, const Audio& a) {
    const QColor fronts[] = {color("#2b5142"), color("#23483e"), color("#1b3b32")};
    const QColor water[] = {color("#465e79"), color("#354d6a"), color("#263e5b")};
    for (int layer = 0; layer < 3; layer++) {
        p.fillPath(shoreline(layer, tr, t, a), blend(fronts[layer], water[layer], tr));
        for (const auto& b : blocks(layer)) facade(p, b, layer, t, tr, a);
    }
}

void water(QPainter& p, double t, double tr, const Audio& a) {
    const double onset = mix(.32, 1, tr);
    for (int row = 0; row < 10; row++) {
        QPainterPath path;
        for (int i = 0; i <= 200; i++) {
            const double x = i * 8;
            const double y = waterY(x, row, t, a);
            if (i == 0) path.moveTo(x, y); else path.lineTo(x, y);
        }
        p.setPen(QPen(alpha(blend(color("#80ad93"), color("#91acbd"), tr), onset * (.16 + (row % 3 == 0 ? .16 : 0))), row % 3 == 0 ? 1.6 : .8));
        p.setBrush(Qt::NoBrush); p.drawPath(path);
        for (int j = 0; j < 9; j++) {
            const double x = hash(j * 13 + row * 19) * W;
            const double y = waterY(x, row, t, a);
            const double level = a.bands[(j + row) % 6];
            line(p, {x, y + 6}, {x + 7 + level * 18, y + 6}, alpha(color("#b5c6b0"), .28 * onset), 1);
        }
    }
}

QPainterPath cityMonument() {
    QPainterPath path; path.moveTo(-40, 930);
    path.lineTo(-40, 708); path.lineTo(125, 708); path.lineTo(125, 552);
    path.lineTo(260, 552); path.lineTo(260, 345); path.lineTo(345, 345);
    path.lineTo(345, 315); path.lineTo(427, 315); path.lineTo(427, 345);
    path.lineTo(502, 345); path.lineTo(502, 567); path.lineTo(638, 567);
    path.lineTo(638, 679); path.lineTo(778, 679); path.lineTo(778, 810);
    path.lineTo(1040, 810); path.lineTo(1040, 930); path.closeSubpath(); return path;
}
QPainterPath seaMonument(double time, const Audio& a) {
    const double weight = a.bass * 28;
    const double sway = std::sin(time * .14) * 9;
    QPainterPath path; path.moveTo(-40, 930);
    path.cubicTo(40, 850, 90, 492, 289, 311 - weight);
    path.cubicTo(437, 171 - weight, 638, 175, 796 + sway, 251);
    path.cubicTo(936, 319, 936, 434, 778, 496);
    path.cubicTo(859, 422, 824, 337, 692, 327);
    path.cubicTo(535, 315, 489, 455, 601, 587 + weight * .5);
    path.cubicTo(777, 798, 1071, 675, 1640, 740);
    path.lineTo(1640, 930); path.closeSubpath(); return path;
}
QPainterPath morph(const QPainterPath& a, const QPainterPath& b, double t) {
    QPainterPath path;
    for (int i = 0; i <= 320; i++) {
        const double pos = i / 320.0;
        const QPointF from = a.pointAtPercent(pos), to = b.pointAtPercent(pos);
        const QPointF point = from * (1 - t) + to * t;
        if (!i) path.moveTo(point); else path.lineTo(point);
    }
    path.closeSubpath(); return path;
}

void monument(QPainter& p, double t, double tr, const Audio& a) {
    const QPainterPath city = cityMonument(), sea = seaMonument(t, a);
    const QPainterPath body = morph(city, sea, tr);
    p.setPen(QPen(blend(color("#779b80"), color("#b5c7c1"), tr), 1.1));
    p.setBrush(blend(color("#12332c"), color("#2d4f67"), tr)); p.drawPath(body);

    p.save(); p.setClipPath(body);
    // Architectural rules curve into the carved ribs of the wave.
    QPainterPath edge;
    edge.moveTo(-40, 930);
    edge.cubicTo(40, 850, 90, 492, 289, 311 - a.bass * 28);
    edge.cubicTo(437, 171 - a.bass * 28, 638, 175, 796 + std::sin(t * .14) * 9, 251);
    edge.cubicTo(936, 319, 936, 434, 778, 496);
    std::array<QPointF, 281> crestPoints;
    for (int j = 0; j <= 280; j++) crestPoints[j] = edge.pointAtPercent(j / 280.0);
    for (int rib = 0; rib < 27; rib++) {
        QPainterPath thread;
        for (int j = 0; j <= 280; j++) {
            const double u = j / 280.0;
            const QPointF from(36 + rib * 30, 302 + u * 640);
            const QPointF to = crestPoints[j] + QPointF(-rib * 8, rib * 18 + 5);
            const QPointF at = from * (1 - tr) + to * tr;
            if (!j) thread.moveTo(at); else thread.lineTo(at);
        }
        const QColor ribColor = rib % 5 == 0 ? color("#7fb4ca") : rib % 3 == 0 ? color("#7e9cd8") : color("#182f48");
        p.setPen(QPen(alpha(blend(color("#568b68"), ribColor, tr), .73), rib % 5 == 0 ? 2.2 : 1.6));
        p.setBrush(Qt::NoBrush); p.drawPath(thread);
    }
    if (tr > .1) {
        p.save(); p.setClipRect(QRectF(0, 635, W, H - 635), Qt::IntersectClip);
        for (int rib = 0; rib < 18; rib++) {
            QPainterPath tail;
            const double offset = rib * 13;
            tail.moveTo(320 - offset * .4, 950);
            tail.cubicTo(465 - offset * .4, 807 + offset, 595, 655 + offset, 785, 751 + offset);
            tail.cubicTo(1040, 845 + offset, 1230, 767 + offset, 1630, 792 + offset);
            p.setPen(QPen(alpha(rib % 3 ? color("#7fb4ca") : color("#132f46"), tr * .5), rib % 3 ? 1 : 2));
            p.drawPath(tail);
        }
        p.restore();
    }
    const double cityRemain = 1 - ease(tr * 2);
    for (int floor = 0; floor < 29; floor++) {
        const double y = 329 + floor * 20;
        line(p, {20, y}, {1000, y}, alpha(color("#7a9b73"), .18 * cityRemain));
    }
    p.setOpacity(cityRemain);
    // Nested panes give the tower the proportions and asymmetric divisions of a tiled desktop.
    for (int i = 0; i < 4; i++) {
        const double y = 386 + i * 55;
        const double split = 322 + a.bands[i + 1] * 66;
        p.fillRect(QRectF(279, y, split - 279, 43), color("#1e4938"));
        p.fillRect(QRectF(split + 7, y, 481 - split - 7, 43), color("#1a3c30"));
        line(p, {279, y + 8}, {split, y + 8}, color("#81b8a8"), .8);
        line(p, {split + 7, y + 8}, {481, y + 8}, color("#509475"), .8);
        const double level = a.bands[i + 1];
        for (int j = 0; j < 4; j++) line(p, {split + 13, y + 16 + j * 6}, {mix(split + 20, 475, hash(i * 4 + j)), y + 16 + j * 6}, alpha(color("#c1c497"), .3 + level * .4), .9);
    }
    p.setOpacity(1);
    p.restore();

    // Facade lights keep their identity as they leave the architecture and become foam.
    for (int i = 0; i < 430; i++) {
        const double cx = 32 + (i % 28) * 28;
        const double cy = 355 + (i / 28) * 27;
        if (!city.contains({cx, cy})) continue;
        const double u = hash(i + 34);
        const double angle = -pi * .83 + u * pi * 1.13;
        const double radius = 195 + hash(i + 95) * 80;
        const QPointF destination(605 + std::cos(angle) * radius + (i % 13) * 2,
                                 405 + std::sin(angle) * radius * .83 - a.accent * hash(i) * 20);
        const double individual = ease((tr - hash(i + 56) * .12) / .88);
        const QPointF at = QPointF(cx, cy) * (1 - individual) + destination * individual;
        const double lit = .2 + .8 * clamp(a.bands[(i / 28 + i % 28) % 6] * 1.8 - hash(i) * .3);
        p.setBrush(alpha(blend(color("#9dbb89"), color("#dcd7ba"), individual), mix(lit, .62 + a.bands[5] * .25, individual)));
        p.setPen(Qt::NoPen);
        p.save(); p.translate(at); p.rotate(individual * (hash(i) * 120 - 60));
        p.drawRect(QRectF(-2, -4, mix(6, 3, individual), mix(10, 4, individual))); p.restore();
    }
    if (tr > .45) {
        const double opacity = ease((tr - .45) / .55);
        p.setBrush(Qt::NoBrush);
        for (int i = 0; i < 24; i++) {
            const double x = 353 + i * 18 + hash(i) * 13;
            const double y = 215 + std::pow((x - 568) / 220, 2) * 56 - a.bass * 12;
            const double curl = 8 + hash(i + 40) * 14;
            QPainterPath crest;
            crest.moveTo(x, y + curl * .7);
            crest.cubicTo(x + curl * .1, y - curl, x + curl, y - curl * 1.8, x + curl * 1.35, y - curl * .6);
            crest.cubicTo(x + curl * .8, y - curl, x + curl * .6, y, x + curl * .5, y + curl * .4);
            p.setPen(QPen(alpha(color("#dcd7ba"), opacity * .84), 1.6 + hash(i) * 2)); p.drawPath(crest);
        }
    }
}

void promenade(QPainter& p, double t, double tr, const Audio& a) {
    const double remaining = 1 - ease(tr * 1.6);
    if (remaining <= 0) return;
    p.setOpacity(remaining);
    // A long tiled arcade occupies the right bank, with legible depth and a canal beneath.
    polygon(p, {{995, 619}, {1530, 448}, {1650, 502}, {1110, 687}}, color("#3c6350"), color("#759f83"));
    polygon(p, {{1110, 687}, {1650, 502}, {1650, 850}, {1110, 890}}, color("#162f29"), color("#476c55"));
    polygon(p, {{995, 619}, {1110, 687}, {1110, 890}, {995, 807}}, color("#21463a"), color("#476c55"));
    for (int i = 0; i < 21; i++) {
        const double u = i / 21.0;
        const double x = mix(1120, 1650, u), roof = mix(686, 503, u);
        line(p, {x, roof}, {x, mix(890, 850, u)}, alpha(color("#87af91"), .26), 1.5);
        for (int j = 0; j < 6; j++) {
            const double y = roof + 25 + j * 28;
            const double lit = a.bands[(i / 3 + j) % 6];
            polygon(p, {{x + 5, y}, {x + 18, y - 4.4}, {x + 18, y + 9.6}, {x + 5, y + 14}}, blend(color("#294e3f"), color("#d3bc97"), .15 + lit * .75));
        }
    }
    for (int i = 0; i < 15; i++) {
        const double x = 1010 + i * 37, y = 614 - i * 12;
        line(p, {x, y}, {x + 89, y + 45}, color("#799e7d"), .9);
    }
    // Signal-like pulses travel along the rail, while the rail itself holds still.
    for (int i = 0; i < 8; i++) {
        const double x = 850 + i * 88, y = 866 - (x - 850) * .07;
        line(p, {x, y}, {x, y - 29}, color("#66886e"), 2);
        p.setPen(Qt::NoPen); p.setBrush(alpha(color("#c8d4af"), .25 + a.bands[i % 6] * .7));
        p.drawEllipse(QPointF(x, y - 31), 2.5, 3.5);
    }
    line(p, {830, 840}, {1650, 783}, color("#789d7c"), 1.5);
    p.setOpacity(1); Q_UNUSED(t);
}

void paper(QPainter& p) {
    // Static print grain belongs to the material, so it does not crawl with the music.
    p.setPen(Qt::NoPen);
    for (int i = 0; i < 4300; i++) {
        p.setBrush(alpha(i % 3 ? color("#dcd7ba") : color("#111a20"), .035));
        p.drawRect(QRectF(hash(i * 5) * W, hash(i * 5 + 1) * H, 1, 1));
    }
}
}

void paint(QPainter& painter, QSize size, double seconds, const Audio& source) {
    Audio a = source;
    for (auto& band : a.bands) band = std::isfinite(band) ? clamp(band) : 0;
    a.bass = std::isfinite(a.bass) ? clamp(a.bass) : 0;
    a.accent = std::isfinite(a.accent) ? clamp(a.accent) : 0;
    const double t = std::isfinite(seconds) ? std::max(0.0, seconds) : 0;
    const double tr = ease((t - 26) / 18);
    painter.save();
    painter.setRenderHint(QPainter::Antialiasing);
    painter.fillRect(QRect(QPoint(), size), color("#101c1b"));
    const double scale = std::min(size.width() / W, size.height() / H);
    painter.translate((size.width() - W * scale) / 2, (size.height() - H * scale) / 2);
    painter.scale(scale, scale);
    sky(painter, t, tr, a);
    farCity(painter, t, tr, a);
    water(painter, t, tr, a);
    promenade(painter, t, tr, a);
    monument(painter, t, tr, a);
    paper(painter);
    painter.restore();
}
}
