#include "scene.h"
#include "caption.h"

#include <QLinearGradient>
#include <QPainterPath>
#include <QPolygonF>
#include <QRadialGradient>
#include <algorithm>
#include <cmath>
#include <vector>

namespace Journey {
namespace {
constexpr double W = 1600, H = 900, pi = 3.141592653589793;
double clamp(double x) { return std::clamp(x, 0.0, 1.0); }
double ease(double x) {
  x = clamp(x);
  return x * x * x * (x * (x * 6 - 15) + 10);
}
double mix(double a, double b, double x) { return a + (b - a) * x; }
double hash(int n) {
  return std::fmod(std::sin(n * 127.1 + 19.73) * 43758.5453 + 43759, 1.0);
}
double wrap(double value, double period) {
  const double remainder = std::fmod(value, period);
  return remainder < 0 ? remainder + period : remainder;
}
QColor c(const char *hex) { return QColor(QString::fromLatin1(hex)); }
QColor alpha(QColor color, double a) {
  color.setAlphaF(clamp(a));
  return color;
}
QColor blend(QColor a, QColor b, double x) {
  return QColor::fromRgbF(mix(a.redF(), b.redF(), x),
                          mix(a.greenF(), b.greenF(), x),
                          mix(a.blueF(), b.blueF(), x));
}
void line(QPainter &p, QPointF a, QPointF b, QColor color, double width = 1) {
  p.setPen(QPen(color, width, Qt::SolidLine, Qt::RoundCap));
  p.drawLine(a, b);
}
void poly(QPainter &p, const std::vector<QPointF> &points, const QBrush &brush,
          QColor stroke = Qt::transparent, double width = 1) {
  QPolygonF polygon;
  for (const auto &point : points)
    polygon << point;
  p.setBrush(brush);
  p.setPen(QPen(stroke, width));
  p.drawPolygon(polygon);
}
void glow(QPainter &p, QPointF center, double rx, double ry, QColor color,
          double strength) {
  if (strength < .001)
    return;
  p.save();
  p.translate(center);
  p.scale(1, ry / rx);
  QRadialGradient g(0, 0, rx);
  g.setColorAt(0, alpha(color, strength));
  g.setColorAt(.25, alpha(color, strength * .4));
  g.setColorAt(1, alpha(color, 0));
  p.setPen(Qt::NoPen);
  p.setBrush(g);
  p.drawEllipse(QPointF(), rx, rx);
  p.restore();
}

struct Camera {
  double x, z, height, horizon;
  QPointF project(double worldX, double worldY, double worldZ) const {
    const double depth = std::max(18.0, worldZ - z);
    const double scale = 710 / depth;
    return {W * .5 + (worldX - x) * scale, horizon + (height - worldY) * scale};
  }
};
struct Building {
  double x, z, width, depth, height;
  int seed, lane;
};
const std::vector<Building> &cityBlocks() {
  static const std::vector<Building> blocks = [] {
    std::vector<Building> result;
    for (int side : {-1, 1})
      for (int lane = 0; lane < 2; lane++)
        for (int row = 0; row < 26; row++) {
          const int seed = row * 19 + lane * 197 + (side + 1) * 211;
          const double width = 35 + hash(seed + 1) * 28;
          const double x = side * (98 + lane * 85 + hash(seed + 2) * 15);
          result.push_back({x - (side < 0 ? width : 0),
                            105 + row * 69 + hash(seed + 4) * 20, width,
                            37 + hash(seed + 5) * 18, 58 + hash(seed + 7) * 147,
                            seed, lane});
        }
    return result;
  }();
  return blocks;
}

void sky(QPainter &p, double t, double sea, const Audio &a) {
  QLinearGradient g(0, 0, 0, H);
  g.setColorAt(0, blend(c("#061915"), c("#101a34"), sea));
  g.setColorAt(.44, blend(c("#174539"), c("#405779"), sea));
  g.setColorAt(1, blend(c("#071f1d"), c("#0a2338"), sea));
  p.fillRect(QRectF(0, 0, W, H), g);
  const QPointF moon(1160 - std::sin(t * .021) * 35, 168 + sea * 15);
  glow(p, moon, 245, 245, blend(c("#87c99f"), c("#95aad1"), sea),
       .15 + a.bass * .08);
  glow(p, {810, 360}, 670, 190, blend(c("#56956f"), c("#7595b8"), sea), .18);
  for (int i = 0; i < 100; i++) {
    const double x = hash(i * 3) * W, y = 22 + hash(i * 3 + 1) * 270;
    p.setBrush(alpha(c("#dcd7ba"), .2 + a.bands[5] * .3));
    p.setPen(Qt::NoPen);
    const double size = i % 19 ? 1 : 2.2;
    p.drawEllipse(QPointF(x, y), size, size);
  }
  p.setPen(Qt::NoPen);
  p.setBrush(blend(c("#d9d5ba"), c("#e8dfb9"), sea));
  p.drawEllipse(moon, 57 + a.bass * 2.5, 57 + a.bass * 2.5);
  // Atmospheric sheets move independently of the musical light response.
  for (int layer = 0; layer < 5; layer++) {
    QPainterPath cloud;
    const double y = 190 + layer * 32;
    cloud.moveTo(-50, y);
    for (int i = 0; i <= 32; i++) {
      const double x = i * 54 - 60;
      const double yy = y + std::sin(x * .004 + layer * 1.7 + t * .025) * 9;
      cloud.lineTo(x, yy);
    }
    cloud.lineTo(1650, y + 25);
    cloud.lineTo(-50, y + 25);
    cloud.closeSubpath();
    p.fillPath(cloud, alpha(blend(c("#2b5d4b"), c("#7895af"), sea), .11));
  }
  for (int layer = 0; layer < 3; layer++) {
    QPainterPath coast;
    coast.moveTo(-30, 500);
    for (int i = 0; i <= 38; i++) {
      const double x = i * 44 - 30;
      const double y =
          365 + layer * 14 - hash(i + layer * 71) * (35 - layer * 7);
      coast.lineTo(x, y);
    }
    coast.lineTo(1650, 500);
    coast.closeSubpath();
    p.fillPath(coast,
               alpha(blend(c("#153f35"), c("#2a4461"), sea), .5 - layer * .1));
  }
  // Small silhouettes cross a large depth rather than circling the frame.
  for (int bird = 0; bird < 8; bird++) {
    const double x = std::fmod(240 + bird * 45 + t * 7, 1800) - 100;
    const double y =
        243 + std::sin(bird * .8) * 23 + std::sin(t * .8 + bird) * 2;
    const double flap = 2.5 + std::sin(t * 2.1 + bird) * 1.5;
    line(p, {x - 4, y - flap}, {x, y}, alpha(c("#071b22"), .55), 1.2);
    line(p, {x, y}, {x + 4, y - flap}, alpha(c("#071b22"), .55), 1.2);
  }
}

void water(QPainter &p, const Camera &camera, double t, double sea,
           const Audio &a) {
  const double horizon = camera.horizon + 8;
  QLinearGradient g(0, horizon, 0, H);
  g.setColorAt(0, blend(c("#285548"), c("#405c76"), sea));
  g.setColorAt(.32, blend(c("#103e35"), c("#214665"), sea));
  g.setColorAt(1, blend(c("#071f23"), c("#102c46"), sea));
  p.fillRect(QRectF(0, horizon, W, H - horizon), g);
  glow(p, {1115, horizon + 130}, 190, 330, c("#d5d7ae"), .12);
  // Perspective ripples: spatial scales grow continuously toward the camera.
  for (int row = 0; row < 70; row++) {
    const double q = (row + .5) / 70;
    const double y = horizon + q * q * (H - horizon + 35);
    QPainterPath ripple;
    for (int i = 0; i <= 100; i++) {
      const double x = i * 17 - 30;
      const double wave =
          std::sin(x * (.022 - q * .016) + t * .55 + row * 1.37);
      const double yy =
          y + wave * (1 + q * 9) * (1 + a.bass * .35 + a.surge * .35);
      if (!i)
        ripple.moveTo(x, yy);
      else
        ripple.lineTo(x, yy);
    }
    const QColor tint =
        row % 7 ? blend(c("#5fa788"), c("#83b6c8"), sea) : c("#d9d5ba");
    p.setPen(QPen(alpha(tint, .035 + q * .095 + a.bands[row % 6] * .11),
                  .5 + q * 1.2));
    p.setBrush(Qt::NoBrush);
    p.drawPath(ripple);
    for (int j = 0; j < 5; j++) {
      const double x = hash(row * 11 + j) * W;
      const double len = 3 + q * 24 + a.bands[j % 6] * 11;
      line(p, {x, y + 4}, {x + len, y + 4}, alpha(tint, .08 + q * .12), .6 + q);
    }
  }
  // The moon reflection breaks into water instead of becoming a solid stripe.
  for (int i = 0; i < 55; i++) {
    const double q = i / 55.0, y = horizon + 20 + q * q * 410;
    const double x = 1130 + std::sin(t * .7 + i * 2.1) * (3 + q * 31);
    line(p, {x - 3 - q * 26, y}, {x + 5 + q * 38, y},
         alpha(c("#dcd7ba"), .10 + a.bands[2] * .15), 1 + q * 2);
  }
}

void building(QPainter &p, const Camera &camera, const Building &b, double t,
              double opacity, const Audio &a) {
  const double z = b.z - camera.z;
  if (z < 40)
    return;
  const double visibility = ease((z - 40) / 35) * opacity;
  const auto v = [&](double x, double y, double depth = 0) {
    return camera.project(x, y, b.z + depth);
  };
  const auto frontLeft = v(b.x, 0), frontRight = v(b.x + b.width, 0);
  const auto rearLeft = v(b.x, 0, b.depth),
             rearRight = v(b.x + b.width, 0, b.depth);
  if (std::max({frontLeft.x(), frontRight.x(), rearLeft.x(), rearRight.x()}) <
          -350 ||
      std::min({frontLeft.x(), frontRight.x(), rearLeft.x(), rearRight.x()}) >
          W + 350)
    return;
  const double fog = clamp((z - 120) / 1900);
  const QColor haze = c("#326958");
  p.save();
  p.setOpacity(visibility);
  const QColor front =
      blend(b.lane ? c("#204c3e") : c("#194638"), haze, fog * .65);
  const QColor side = blend(c("#0b2b27"), haze, fog * .6);
  const double wallX = b.x < 0 ? b.x + b.width : b.x;
  QLinearGradient wall(v(b.x, b.height), v(b.x + b.width, 0));
  wall.setColorAt(0, blend(front, c("#539176"), .26));
  wall.setColorAt(1, front);
  poly(p,
       {v(b.x, 0), v(b.x + b.width, 0), v(b.x + b.width, b.height),
        v(b.x, b.height)},
       wall, alpha(c("#76b296"), .23), .8);
  poly(p,
       {v(wallX, 0), v(wallX, 0, b.depth), v(wallX, b.height, b.depth),
        v(wallX, b.height)},
       side, alpha(c("#509075"), .17), .7);
  poly(p,
       {v(b.x, b.height), v(b.x + b.width, b.height),
        v(b.x + b.width, b.height, b.depth), v(b.x, b.height, b.depth)},
       blend(c("#416f54"), haze, fog * .4), alpha(c("#96bea0"), .4), .8);
  const int floors = std::max(3, int(b.height / 9));
  const int columns = std::max(3, int(b.width / 8));
  // The canal-facing sides carry glass and mullions too, including near walls.
  for (int floor = 0; floor < floors; floor++) {
    const double y = 5 + floor * (b.height - 10) / floors;
    for (int col = 0; col < 4; col++) {
      const double depth = 3 + col * (b.depth - 6) / 4;
      const double width = (b.depth - 6) / 4 * .58;
      const double height = (b.height - 10) / floors * .59;
      const int id = b.seed + floor * 17 + col * 23;
      const double level = .12 + a.bands[(floor + col + b.seed) % 6] * .9;
      const QColor glass =
          hash(id) < .22 ? c("#103a32")
                         : blend(c("#163b34"),
                                 id % 5 ? c("#93bda6") : c("#8cd3cb"), level);
      poly(p,
           {v(wallX, y, depth), v(wallX, y, depth + width),
            v(wallX, y + height, depth + width), v(wallX, y + height, depth)},
           glass, alpha(c("#6e9b84"), .12), .6);
    }
  }
  for (int floor = 0; floor < floors; floor++) {
    const double y = 5 + floor * (b.height - 10) / floors;
    if (floor % 4 == 0)
      line(p, v(b.x + 1, y), v(b.x + b.width - 1, y), alpha(c("#97b38b"), .17),
           .7);
    for (int col = 0; col < columns; col++) {
      const int id = b.seed + floor * 13 + col * 7;
      const double x = b.x + 3 + col * (b.width - 6) / columns;
      const double band = a.bands[(floor / 3 + col + b.seed) % 6];
      const double occupancy = hash(id + 3) > .27 ? 1 : .08;
      const double traveling =
          a.surge *
          (.35 +
           .65 * std::pow(std::max(0.0, std::sin(z * .008 + t * .65)), 3));
      const double lit = occupancy * clamp(.22 + band * 1.4 + traveling * .7);
      const QColor warm = id % 9   ? c("#d8d6a2")
                          : id % 2 ? c("#eaad86")
                                   : c("#7ae1cc");
      const double ww = (b.width - 6) / columns * .5,
                   wh = (b.height - 10) / floors * .54;
      poly(p, {v(x, y), v(x + ww, y), v(x + ww, y + wh), v(x, y + wh)},
           blend(c("#10312b"), warm, lit));
      if (lit > .52 && z < 500 && id % 5 == 0) {
        const QPointF center = v(x + ww * .5, y + wh * .5);
        glow(p, center, std::max(2.0, 710 / z * ww * 1.7),
             std::max(2.0, 710 / z * wh * 1.2), warm, .12 * lit);
      }
      // Reflected facade light is fragmented into gently shearing currents.
      if (occupancy > .5 && floor % 3 == 0 && z < 1000) {
        const auto reflection = v(x, -y - wh);
        if (reflection.y() < H)
          for (int streak = 0; streak < 3; streak++) {
            const double drift = std::sin(t * .7 + floor + col + streak) * 4;
            const double yy = reflection.y() + streak * 3.5;
            line(p, {reflection.x() + drift - 2, yy},
                 {reflection.x() + drift + 710 / z * ww, yy},
                 alpha(warm, lit * .17), 1.1);
          }
      }
    }
  }
  if (b.seed % 3 == 0 && z < 900) {
    // Asymmetric tiled panels give the near towers an Omarchy identity.
    const double x = wallX + (wallX == b.x ? 2 : -11);
    const QColor neon = b.seed % 2 ? c("#85e1cb") : c("#d7b292");
    const double bottom = b.height * .23, top = b.height * .75;
    poly(p, {v(x, bottom), v(x + 8, bottom), v(x + 8, top), v(x, top)},
         c("#0a2923"), alpha(neon, .6), 1);
    for (int panel = 0; panel < 6; panel++) {
      const double y = bottom + panel * (top - bottom) / 6;
      const double level = a.bands[panel] * .8 + .12;
      poly(p,
           {v(x + .7, y + 1), v(x + 1 + level * 6, y + 1),
            v(x + 1 + level * 6, y + 3), v(x + .7, y + 3)},
           alpha(neon, .85));
    }
    glow(p, v(x + 4, (top + bottom) * .5), 710 / z * 17,
         710 / z * (top - bottom) * .8, neon, .17);
  }
  if (b.seed % 4 == 0) {
    const double mast = b.height + 14;
    line(p, v(b.x + b.width * .65, b.height), v(b.x + b.width * .65, mast),
         alpha(c("#9cb99b"), .4), .8);
    line(p, v(b.x + b.width * .65 - 4, mast - 4),
         v(b.x + b.width * .65 + 4, mast - 4), alpha(c("#9cb99b"), .35), .8);
  }
  p.restore();
}

void city(QPainter &p, const Camera &camera, double t, double sea,
          const Audio &a) {
  const double remain = 1 - ease(sea);
  if (remain < .001)
    return;
  std::vector<const Building *> order;
  for (const auto &b : cityBlocks())
    order.push_back(&b);
  std::sort(order.begin(), order.end(), [](const auto *x, const auto *y) {
    return x->z + x->depth > y->z + y->depth;
  });
  for (const auto *b : order)
    building(p, camera, *b, t, remain, a);
  p.save();
  p.setOpacity(remain);
  // Continuous quay geometry forms the route's banks.
  for (int side : {-1, 1}) {
    const double x = side * 87;
    poly(p,
         {camera.project(x, 0, camera.z + 28), camera.project(x, 0, 1900),
          camera.project(x + side * 18, 0, 1900),
          camera.project(x + side * 18, 0, camera.z + 28)},
         c("#142f2a"));
    for (int i = 0; i < 55; i++) {
      const double z = 50 + i * 35;
      if (z - camera.z < 35)
        continue;
      const auto base = camera.project(x, 0, z), top = camera.project(x, 7, z);
      line(p, base, top, alpha(c("#7bad8d"), .7),
           std::max(.5, 710 / (z - camera.z) * .6));
      if (i % 3 == 0) {
        const QColor light = c("#c1c497");
        const double brightness = .35 + a.bands[i % 6] * .55 + a.surge * .25;
        glow(p, top, std::max(3.0, 710 / (z - camera.z) * 4),
             std::max(4.0, 710 / (z - camera.z) * 8), light, .23 * brightness);
        p.setPen(Qt::NoPen);
        p.setBrush(alpha(light, brightness));
        p.drawEllipse(top, 1.3, 1.8);
      }
    }
    line(p, camera.project(x, 5, camera.z + 28), camera.project(x, 5, 1900),
         alpha(c("#a5cab0"), .36), 1.3);
  }
  // A distant elevated tram offers a slower event across the skyline.
  constexpr double bridgeZ = 690;
  const double bx = -155 + std::fmod(t * 7.5, 310.0);
  poly(p,
       {camera.project(-220, 54, bridgeZ), camera.project(220, 54, bridgeZ),
        camera.project(220, 51, bridgeZ), camera.project(-220, 51, bridgeZ)},
       c("#4b806b"), alpha(c("#9ac2a8"), .6), .9);
  poly(p,
       {camera.project(bx, 55, bridgeZ), camera.project(bx + 47, 55, bridgeZ),
        camera.project(bx + 47, 65, bridgeZ), camera.project(bx, 65, bridgeZ)},
       c("#557968"), c("#b9c29b"), .9);
  for (int i = 0; i < 6; i++)
    poly(p,
         {camera.project(bx + 4 + i * 6, 57, bridgeZ),
          camera.project(bx + 8 + i * 6, 57, bridgeZ),
          camera.project(bx + 8 + i * 6, 62, bridgeZ),
          camera.project(bx + 4 + i * 6, 62, bridgeZ)},
         alpha(c("#edcaa0"), .6 + a.bands[3] * .35));
  p.restore();
}

void lanterns(QPainter &p, const Camera &camera, double t, double sea,
              const Audio &a) {
  const double remaining = 1 - sea * .75;
  for (int i = 0; i < 14; i++) {
    const double z = camera.z + 90 + wrap(i * 81 + 1000 - t * 5, 1200.0);
    const double depth = z - camera.z;
    // Both ends of a route are invisible, so recycling never teleports a light.
    const double opacity =
        remaining * ease((depth - 90) / 70) * ease((1290 - depth) / 90);
    if (opacity < .001)
      continue;
    const double x = (hash(i + 36) - .5) * 125;
    QPointF at = camera.project(x, 1, z);
    at.ry() += std::sin(t * .65 + i) * 1.7;
    const double size = std::max(1.5, 710 / (z - camera.z) * 2.4);
    const QColor light = i % 4 ? c("#efc48d") : c("#8be3d0");
    glow(p, at, size * 5, size * 5, light,
         (.22 + a.bands[i % 6] * .12) * opacity);
    p.setPen(Qt::NoPen);
    p.setBrush(alpha(light, .8 * opacity));
    p.drawEllipse(at, size * .6, size);
    for (int j = 0; j < 4; j++)
      line(
          p, {at.x() - size * .6, at.y() + size * 3 + j * 3},
          {at.x() + size * .6 + std::sin(t + j) * 2, at.y() + size * 3 + j * 3},
          alpha(light, (.23 - j * .04) * opacity), 1);
  }
}

void ocean(QPainter &p, double t, double sea, const Audio &a) {
  const double arrival = ease((sea - .12) / .88);
  if (arrival < .001)
    return;
  p.save();
  // The foreground current grows into a coherent curl; no unrelated polygons
  // are interpolated.
  const double rise = arrival * (90 + a.bass * 35 + a.surge * 65);
  const double shift = std::sin((t - 30) * .13) * 22;
  const double crestY = 475 - rise;
  QPainterPath wave;
  wave.moveTo(-70, 920);
  wave.cubicTo(80, 780, 165, crestY + 42, 440 + shift, crestY - 74);
  wave.cubicTo(650 + shift, crestY - 160, 835 + shift, crestY - 66, 890 + shift,
               crestY + 42);
  wave.cubicTo(945 + shift, crestY + 139, 881 + shift, crestY + 209,
               759 + shift, crestY + 210);
  wave.cubicTo(817 + shift, crestY + 157, 818 + shift, crestY + 93, 737 + shift,
               crestY + 70);
  wave.cubicTo(625 + shift, crestY + 20, 531 + shift, crestY + 128, 618 + shift,
               crestY + 258);
  wave.cubicTo(790, 792, 1170, 712, 1670, 815);
  wave.lineTo(1670, 940);
  wave.closeSubpath();
  QLinearGradient material(300, crestY - 70, 750, 900);
  material.setColorAt(0, c("#679db9"));
  material.setColorAt(.23, c("#3b7196"));
  material.setColorAt(.66, c("#183e63"));
  material.setColorAt(1, c("#123044"));
  p.setOpacity(arrival);
  p.fillPath(wave, material);
  p.setPen(QPen(alpha(c("#a8d1d5"), .7), 1.3));
  p.setBrush(Qt::NoBrush);
  p.drawPath(wave);
  p.save();
  p.setClipPath(wave);
  // Each stripe follows its own nested spline, keeping the curl's contours
  // orderly.
  for (int rib = 0; rib < 34; rib++) {
    const double q = rib / 34.0;
    QPainterPath contour;
    contour.moveTo(-40 - rib * 8, 940);
    contour.cubicTo(90 - rib * 4, 780 + rib * 7, 175 - rib * 3,
                    crestY + 55 + rib * 13, 438 + shift - rib * 2,
                    crestY - 62 + rib * 11);
    contour.cubicTo(653 + shift - rib * 1.6, crestY - 139 + rib * 10,
                    825 + shift - rib * 2, crestY - 65 + rib * 13,
                    868 + shift - rib * 2.3, crestY + 43 + rib * 11);
    contour.cubicTo(890 - rib * 2, crestY + 170 + rib * 11, 1080, 790 + rib * 6,
                    1660, 800 + rib * 5);
    const QColor ink = rib % 5 ? c("#81b6cc") : c("#c1d8d7");
    p.setPen(QPen(alpha(ink, .25 + q * .16), rib % 5 ? 1 : 1.8));
    p.drawPath(contour);
  }
  p.restore();
  QPainterPath edge;
  edge.moveTo(440 + shift, crestY - 74);
  edge.cubicTo(650 + shift, crestY - 160, 835 + shift, crestY - 66, 890 + shift,
               crestY + 42);
  edge.cubicTo(945 + shift, crestY + 139, 881 + shift, crestY + 209,
               759 + shift, crestY + 210);
  std::array<QPointF, 121> samples;
  for (int i = 0; i <= 120; i++)
    samples[i] = edge.pointAtPercent(i / 120.0);
  for (int i = 0; i < 120; i++) {
    const double flow = std::fmod(i / 120.0 + t * .014, 1.0) * 120;
    const int index = std::min(119, int(flow));
    QPointF at = samples[index] * (1 - (flow - index)) +
                 samples[index + 1] * (flow - index);
    const double height = 4 + hash(i + 50) * 18 + a.accent * 9 + a.surge * 13;
    const double drift = std::sin(t * .8 + i) * 3;
    at += QPointF(drift, -height * hash(i));
    p.setPen(
        QPen(alpha(c("#e2dec0"), .4 + hash(i + 40) * .5), 1 + hash(i) * 1.5));
    QPainterPath foam;
    foam.moveTo(at);
    foam.quadTo(at + QPointF(4, -height),
                at + QPointF(8 + hash(i) * 8, -height * .2));
    p.drawPath(foam);
    if (i % 2 == 0) {
      p.setPen(Qt::NoPen);
      p.setBrush(alpha(c("#dcd7ba"), .45));
      p.drawEllipse(at + QPointF(4, -height - 7), 1.2, 1.6);
    }
  }
  p.restore();
}

void atmosphere(QPainter &p, double sea) {
  // Darkened edges contain bright local lights without a frame-wide flash.
  QRadialGradient vignette(W * .52, H * .44, 1050);
  vignette.setColorAt(0, Qt::transparent);
  vignette.setColorAt(.65, alpha(c("#031117"), .04));
  vignette.setColorAt(1, alpha(c("#031117"), .52));
  p.fillRect(QRectF(0, 0, W, H), vignette);
  QLinearGradient haze(0, 285, 0, 480);
  haze.setColorAt(0, Qt::transparent);
  haze.setColorAt(.6, alpha(blend(c("#7cb495"), c("#8ca9cb"), sea), .06));
  haze.setColorAt(1, Qt::transparent);
  p.fillRect(QRectF(0, 285, W, 195), haze);
}
} // namespace

void paint(QPainter &p, QSize size, double seconds, const Audio &source) {
  Audio a = source;
  for (auto &band : a.bands)
    band = std::isfinite(band) ? clamp(band) : 0;
  a.bass = std::isfinite(a.bass) ? clamp(a.bass) : 0;
  a.accent = std::isfinite(a.accent) ? clamp(a.accent) : 0;
  a.surge = std::isfinite(a.surge) ? clamp(a.surge) : 0;
  const double t = std::isfinite(seconds) ? std::clamp(seconds, 0.0, 600.0) : 0;
  const double sea = ease((t - 24) / 10);
  const double cityTime = std::min(t, 34.0);
  Camera camera{std::sin(cityTime * .085) * 18, cityTime * 9.5,
                mix(59, 30, sea), mix(327, 371, sea)};
  p.save();
  p.setRenderHint(QPainter::Antialiasing);
  p.fillRect(QRect(QPoint(), size), c("#061915"));
  const double scale = std::min(size.width() / W, size.height() / H);
  p.translate((size.width() - W * scale) / 2, (size.height() - H * scale) / 2);
  p.scale(scale, scale);
  p.setClipRect(QRectF(0, 0, W, H));
  sky(p, t, sea, a);
  water(p, camera, t, sea, a);
  city(p, camera, t, sea, a);
  lanterns(p, camera, t, sea, a);
  ocean(p, t, sea, a);
  atmosphere(p, sea);
  paintThemeCaption(p, t);
  p.restore();
}
} // namespace Journey
