#include "label-pieces.h"

#include "generic-window.h"
#include "lanterns.h"
#include "neon.h"
#include "palette.h"
#include <QPainterPathStroker>
#include <algorithm>
#include <cmath>

namespace Journey::Kit {
namespace {
// Osaka's strand gauges, one per band, and the warm paper of its hand lantern.
constexpr double Gauge[6] = {2.1, 1.9, 1.6, 1.35, 1.15, 0.95};
const Col LanternPaper(0.98f, 0.62f, 0.30f);
constexpr double LanternRatio = 0.76;   // width to height, as in Osaka's festoon lanterns
constexpr double LanternRadiusY = 10.5; // Osaka's festoon lantern, the size the glow is scaled from

struct Shape {
    QRectF box;
    QPainterPath path;
};

Shape shapeOf(const SvgElement& element) {
    Shape shape;
    shape.path = element.transform.map(element.geometry);
    shape.box = shape.path.boundingRect();
    return shape;
}

const SvgElement& elementOf(const OsakaWorldDescription& world, const OsakaPieceNodeV1& node) {
    return world.art->elements()[node.element];
}

void lantern(Ctx& c, const OsakaEventState& L, const OsakaPieceNodeV1& node, const Shape& shape,
             std::size_t index, Canvas& light) {
    const double ry = std::max(1.0, shape.box.height() / 2), rx = ry * LanternRatio;
    const double k = ry / LanternRadiusY;
    double lv = GenericLanternV1::level(c, node);
    V2 at(shape.box.center().x(), shape.box.center().y());
    double tilt = 0;
    if (node.sway) {
        // The lantern hangs from a point above it and swings as Osaka's festoon does.
        const double phase = 0.9 * double(index);
        const double sw = 0.07 * std::sin(c.t * 1.6 + phase) + L.wind * 0.35 * std::sin(c.t * 3.1 + 0.77 * phase);
        const double cord = 2.0 * ry;
        at = V2(at.x + std::sin(sw) * cord, at.y - cord + std::cos(sw) * cord);
        tilt = sw * 0.6;
    }
    lv = std::min(lv, 1.8);
    light.glow(at.x, at.y, (24 + 26 * lv) * k, mix(LanternPaper, WARM_T, 0.4), 0.32 * lv);
    paperLantern(light, at, rx, ry, tilt, LanternPaper, std::min(1.5, lv));
}

void lamp(Ctx& c, const OsakaPieceNodeV1& node, const Shape& shape, Canvas& light) {
    const double size = std::max(4.0, std::max(shape.box.width(), shape.box.height()));
    const double lv = std::min(GenericLampV1::level(c, node), 1.6);
    const V2 at(shape.box.center().x(), shape.box.center().y());
    light.glow(at.x, at.y, 3.0 * size, mix(WARM_B, WARM_T, 0.5), 0.50 * lv);
    light.glow(at.x, at.y, 0.9 * size, CREAM, 0.70 * std::min(lv, 1.5));
}
}

double GenericLanternV1::level(const Ctx& c, const OsakaPieceNodeV1& node) {
    // Osaka's festoon lantern: a steady glow, the band, and the bass hit.
    double value = 0.30 + 0.14 * c.band(node.band) + 0.35 * c.lift(node.band);
    if (node.kick) value += 0.40 * c.kick(5);
    return value;
}

double GenericLampV1::level(const Ctx& c, const OsakaPieceNodeV1& node) {
    double value = 0.45 + 0.25 * c.band(node.band) + 0.35 * c.lift(node.band);
    if (node.kick) value += 0.50 * c.kick(5);
    return value;
}

double GenericNeonV1::level(const Ctx& c, const OsakaPieceNodeV1& node, std::size_t index) {
    const auto& p = osakaParameters().signs;
    double value = 0.55 + 0.30 * c.band(node.band) + 0.35 * c.lift(node.band);
    if (node.kick) value += p.levelKick * c.kick(5);
    if (node.flicker) {
        // Osaka's neon: a slow shimmer and the occasional stutter.
        const double stutter = hash1(std::floor(c.t * p.stutterRate) + 37.0 * double(index)) < p.stutterProbability
            ? p.stutterLevel : 1.0;
        value = (value + p.levelSine * std::sin(c.t * p.levelRate + double(index))) * stutter;
    }
    return value;
}

double GenericGlowV1::level(const Ctx& c, const OsakaPieceNodeV1& node) {
    return GenericWindowV1::envelope(c, node.band, node.always, node.kick, node.onset);
}

double GenericWireV1::hum(const Ctx& c, const OsakaPieceNodeV1& node) {
    // Osaka's strand hum: it follows the band and thumps with the bass.
    const double lift = c.lift(node.band);
    const double thump = (node.band < 2 ? 0.14 : 0.06) * c.kick(7);
    return (0.035 + 0.11 * c.band(node.band) + 0.26 * lift + thump) / (1 + 0.7 * lift);
}

QPainterPath LabelPiecesV1::area(const OsakaPieceNodeV1& node, const SvgElement& element) {
    const Shape shape = shapeOf(element);
    QPainterPath area;
    if (node.piece == "lantern") {
        const double ry = std::max(1.0, shape.box.height() / 2);
        area.addEllipse(shape.box.center(), ry * 1.4, ry * 1.4);
    } else if (node.piece == "lamp") {
        const double size = std::max(4.0, std::max(shape.box.width(), shape.box.height()));
        area.addEllipse(shape.box.center(), 1.5 * size, 1.5 * size);
    } else if (node.piece == "glow") {
        area.addRect(shape.box.adjusted(-8, -8, 8, 8));
    } else {
        // Neon tubes and wires are thin: measure the strip around the path.
        QPainterPathStroker stroker;
        stroker.setWidth(node.piece == "wire" ? (node.pulse ? 28 : 12) : 20);
        area = stroker.createStroke(shape.path);
    }
    return area;
}

void LabelPiecesV1::draw(Ctx& c, const OsakaEventState& L, const OsakaWorldDescription& world) {
    if (world.pieces.empty()) return;
    GpuProfile::Group profileGroup(c.gpu.profile, "labelPieces");
    const auto& art = *world.art;
    auto has = [&](const char* piece) {
        return std::any_of(world.pieces.begin(), world.pieces.end(),
                           [&](const auto& node) { return node.piece == piece; });
    };

    // Wires first, as Osaka draws its conductors: ink body, then hum and pulses.
    if (has("wire")) {
        Canvas& body = c.canvas();
        Canvas& light = c.canvas();
        static const auto tailFade = [] {
            std::array<double, 16> values{};
            for (int j = 0; j < 16; ++j) values[j] = std::pow(1 - j / 16.0, 1.6);
            return values;
        }();
        for (std::size_t index = 0; index < world.pieces.size(); ++index) {
            const auto& node = world.pieces[index];
            if (node.piece != "wire") continue;
            const int band = node.band;
            const double gauge = Gauge[band];
            const auto polygons = shapeOf(elementOf(world, node)).path.toSubpathPolygons();
            Rng rng(101 + 13 * uint64_t(index));
            for (const auto& polygon : polygons) {
                if (polygon.size() < 2) continue;
                std::vector<V2> points;
                std::vector<double> along{0};
                for (int i = 0; i < polygon.size(); ++i) {
                    points.push_back(V2(polygon[i].x(), polygon[i].y()));
                    if (i) along.push_back(along.back() + (points[i] - points[i - 1]).len());
                }
                const double length = along.back();
                if (length <= 0) continue;
                body.polyline(points, gauge, INK, 0.95);
                std::vector<V2> rim(points);
                for (V2& q : rim) q.y -= 0.55 * gauge;
                body.polyline(rim, 0.6, RIM, 0.32);
                const double lift = c.lift(band);
                const double hum = GenericWireV1::hum(c, node);
                if (hum > 0.01)
                    light.polyline(points, 1.25 + 0.35 * clamp01(lift), band < 2 ? WARM_T : PULSE[band], hum);
                if (!node.pulse) continue;
                auto at = [&](double u) {
                    const double target = clamp01(u) * length;
                    const std::size_t hi = std::size_t(std::lower_bound(along.begin(), along.end(), target) - along.begin());
                    if (hi == 0) return points.front();
                    const double span = along[hi] - along[hi - 1];
                    const double f = span > 0 ? (target - along[hi - 1]) / span : 0;
                    return points[hi - 1] + (points[hi] - points[hi - 1]) * f;
                };
                // Light travelling the strand, faster and brighter as the band rises.
                const double travel = c.score ? c.score->strandPhase[band] : 0;
                const double level = 0.55 + 0.8 * c.band(band) + 0.7 * c.lift(band);
                const int pulses = std::clamp(int(length / 300) + 2, 2, 8);
                const double tail = (0.085 - 0.008 * band) / std::max(0.45, length / 900);
                for (int k = 0; k < pulses; ++k) {
                    const double tt = std::fmod(rng.uni() + travel, 1.0);
                    const double lvl = std::min(1.4, (0.55 + 0.45 * rng.uni()) * level);
                    if (lvl < 0.02) continue;
                    for (int j = 0; j < 16; ++j) {
                        const double tj = tt - tail * j / 16;
                        if (tj < 0) break;
                        const V2 q = at(tj);
                        light.disc(q.x, q.y, (2.4 - 1.6 * j / 16) * (0.8 + 0.5 * lvl), PULSE[band],
                                   tailFade[std::size_t(j)] * std::min(1.0, lvl));
                    }
                    const V2 q = at(tt);
                    light.glow(q.x, q.y, 13 + 14 * lvl, PULSE[band], 0.9 * std::min(1.0, lvl));
                }
            }
        }
        c.gpu.over(body);
        c.gpu.over(light, 1.9f);
        c.gpu.add(light, 0.5f, 12);
    }

    // Glows of the element's own shape.
    for (std::size_t index = 0; index < world.pieces.size(); ++index) {
        const auto& node = world.pieces[index];
        if (node.piece != "glow") continue;
        const double level = std::min(GenericGlowV1::level(c, node), 1.6);
        if (level <= 0.01) continue;
        Canvas& canvas = c.canvas();
        if (!art.replay(canvas, QString::fromStdString(node.id))) continue;
        c.gpu.add(canvas, float(level), 10);
    }

    // Lanterns and lamps share one light canvas, submitted as Osaka submits its lanterns.
    if (has("lantern") || has("lamp")) {
        Canvas& light = c.canvas();
        for (std::size_t index = 0; index < world.pieces.size(); ++index) {
            const auto& node = world.pieces[index];
            if (node.piece == "lantern") lantern(c, L, node, shapeOf(elementOf(world, node)), index, light);
            else if (node.piece == "lamp") lamp(c, node, shapeOf(elementOf(world, node)), light);
        }
        c.gpu.over(light, 1.35f);
        c.gpu.add(light, 0.45f, 14);
    }

    // Neon: each sign has its own level, so each is submitted on its own.
    const auto& signs = osakaParameters().signs;
    for (std::size_t index = 0; index < world.pieces.size(); ++index) {
        const auto& node = world.pieces[index];
        if (node.piece != "neon") continue;
        const double level = std::max(0.0, GenericNeonV1::level(c, node, index));
        if (level <= 0.01) continue;
        Canvas& canvas = c.canvas();
        if (!art.replay(canvas, QString::fromStdString(node.id))) continue;
        c.gpu.over(canvas, float(signs.overGain * level));
        c.gpu.add(canvas, float(signs.addGain * level), float(signs.addBlur));
        c.gpu.add(canvas, float(0.5 * signs.addGain * level), float(3 * signs.addBlur));
    }
}
}
