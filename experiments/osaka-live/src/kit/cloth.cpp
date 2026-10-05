#include "cloth.h"
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
void OsakaStreetClothV1::draw(Canvas& cv, const RigIn& r, Col cloth) {
    const Body b = solve(r);
    drawBody(cv, b, INK);
    const V2 side = V2(std::cos(r.lean), r.facing * std::sin(r.lean)) * r.facing;
    const V2 shoulder = b.shoulder + side * (0.045 * r.h);
    const V2 waist = r.hip + side * (0.045 * r.h);
    cv.capsule(shoulder, waist, 0.021 * r.h, 0.024 * r.h, mix(INK, cloth, 0.22));
    if (r.obi) cv.line(r.hip.x - 0.05 * r.h, r.hip.y - 0.03 * r.h,
                       r.hip.x + 0.06 * r.h, r.hip.y - 0.03 * r.h, 0.05 * r.h, mix(INK, cloth, 0.28));
}

void OsakaNorenV1::draw(Ctx& c, const OsakaState& s, const Life& L, Canvas& l, double t, double yx) {
    // Noren drop when the cart opens, then sway and flutter.
    const double wind = L.wind;
    for (int i = 0; i < 4; ++i) {
        const double nx = yx + 10 + i * 40;
        const double drop = s.chapter ? springStep(t - (5.1 + i * 0.07), 1.6, 5.0) : 1.0;
        const double len = 4 + 22 * std::max(0.0, drop);
        const double sway = std::sin(t * 1.3 + i * 0.4) * 3 + wind * 7 * std::sin(t * 6 + i);
        const double bx = sway * 0.5 + wind * 4;
        l.color(mix(RED, INK, 0.42), 0.98);
        l.moveTo(nx, 757); l.lineTo(nx + 37, 757);
        l.lineTo(nx + 37 + sway * (i % 2 * 2 - 1) * 0.4 + wind * 4, 757 + len);
        l.lineTo(nx + bx, 757 + len);
        l.closePath();
        l.fill();
        // One kana per panel, riding the cloth's sway.
        drawSignGlyph(l, 3 + i, nx + 10 + bx * 0.6, 757 + len - 4, 17, mix(CREAM, WARM_T, 0.2), 0.92 * sstep(0.6, 0.95, drop));
    }
}

void OsakaIzakayaClothV1::draw(const Life& L, Canvas& f, double t, double x0) {
    const double wind = L.wind;
    for (int i = 0; i < 4; ++i) {
        // Noren flutter in the breeze.
        const double nx = x0 + 44 + i * 57;
        const double flut = std::sin(t * 1.2 + i) * 3 + wind * 10 * (0.6 + 0.4 * std::sin(t * 7 + i * 1.7));
        f.color(mix(JADE, INK, 0.45));
        f.moveTo(nx, 800); f.lineTo(nx + 52, 800);
        f.lineTo(nx + 52 + flut, 836 - wind * 6); f.lineTo(nx + flut, 836 - wind * 6 + std::sin(t * 1.2 + i) * 3);
        f.closePath();
        f.fill();
    }
}

}
