#include "neon.h"
#include "palette.h"

namespace Journey::Kit {
void OsakaNeonV1::draw(Ctx& c, const OsakaState& s, Canvas& f, Canvas& n, double t, double x0, double& stutter) {
    // Neon 居酒屋 on a dark board, with an occasional stutter; it kicks with the bass.
    stutter = (hash1(std::floor(t * 6)) < 0.04) ? 0.35 : 1.0;
    c.retain(f, "neon-board", [&](Canvas& f) {
        f.fillRect(x0 + 289, 792, 40, 126, Col(0.016f, 0.035f, 0.03f));
    }, {s.cam});
    const Col tube = mix(MAG, Col(1.0f, 0.92f, 0.97f), 0.35);
    c.retain(n, "neon-tubes", [&](Canvas& n) {
        n.color(MAG, 0.9); n.rect(x0 + 291, 794, 36, 122); n.stroke(1.6);
        for (int j = 0; j < 3; ++j) drawSignGlyph(n, j, x0 + 294, 826 + j * 36, 30, tube, 1.0);
    }, {s.cam});
    n.glow(x0 + 309, 856, 120, MAG, 0.30 + 0.25 * c.kick(5));

}
double OsakaNeonV1::level(const Ctx& c, double t, double stutter) {
    return (0.85 + 0.15 * std::sin(t * 5) + 0.5 * c.kick(5)) * stutter;
}
void OsakaNeonV1::submit(Ctx& c, Canvas& n, double neon) {
    c.gpu.over(n, float(1.3 * neon));
    c.gpu.add(n, float(0.35 * neon), 8);
}
}
