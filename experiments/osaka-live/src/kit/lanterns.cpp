#include "lanterns.h"
#include "palette.h"
#include "primitives.h"
#include "pane.h"
#include "haze.h"
#include "../rig.h"
#include "../osaka_shaders.h"
#include <cmath>
#include <vector>

namespace Journey::Kit {
using Life = OsakaLegacyLife;
void OsakaPaperLanternV1::draw(Canvas& light, V2 at, double rx, double ry, double tilt, Col paper, double bright) {
    light.save();
    light.translate(at.x, at.y);
    light.rotate(tilt);
    const float k = float(0.55 + 0.45 * bright);
    light.radial(-rx * 0.2, -ry * 0.15, std::max(rx, ry) * 1.15,
                 {{0, mix(paper, Col(1, 0.95f, 0.8f), 0.55) * k, 1}, {0.6f, paper * k, 1}, {1, mix(paper, INK, 0.45) * k, 1}});
    light.ellipse(0, 0, rx, ry);
    light.fill();
    for (int j = -2; j <= 2; ++j) {
        const double y = j * ry * 0.33, half = rx * std::sqrt(std::max(0.0, 1 - (y * y) / (ry * ry))) * 0.96;
        light.line(-half, y, half, y, std::max(0.5, ry * 0.07), mix(paper, INK, 0.7), 0.55);
    }
    light.fillRect(-rx * 0.62, -ry - ry * 0.16, rx * 1.24, ry * 0.22, INK);
    light.fillRect(-rx * 0.62, ry - ry * 0.06, rx * 1.24, ry * 0.22, INK);
    light.restore();
}

void OsakaLanternV1::draw(Canvas& body, Canvas& light, V2 hand, double swing, double size, double bright) {
    const V2 top = hand + V2(14 * size, -6 * size);
    const V2 hang = top + V2(std::sin(swing) * 26 * size, std::cos(swing) * 26 * size);
    body.line(hand.x, hand.y, top.x, top.y, 2.6 * size, INK);
    body.line(top.x, top.y, hang.x, hang.y - 12 * size, 1.4 * size, INK);
    light.glow(hang.x, hang.y, 54 * size, Col(1.0f, 0.60f, 0.28f), 0.34 * bright);
    paperLantern(light, hang, 9 * size, 12 * size, swing * 0.6, Col(0.98f, 0.62f, 0.30f), 0.7 + 0.3 * bright);
}

}
