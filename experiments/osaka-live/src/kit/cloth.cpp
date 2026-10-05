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

}
