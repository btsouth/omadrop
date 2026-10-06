#include "parameters.h"
#include "neon.h"
#include "palette.h"

namespace Journey::Kit {
void OsakaNeonV1::draw(Ctx& c, const OsakaState& s, Canvas& f, Canvas& n, double t, double x0, double& stutter) {
    const auto& p = osakaParameters().signs;
    // Neon 居酒屋 on a dark board, with an occasional stutter; it kicks with the bass.
    stutter = (hash1(std::floor(t * p.stutterRate)) < p.stutterProbability) ? p.stutterLevel : 1.0;
    c.retain(f, "neon-board", [&](Canvas& f) {
        f.fillRect(x0 + p.boardX, p.boardY, p.boardW, p.boardH, Col(float(p.boardR), float(p.boardG), float(p.boardB)));
    }, {s.cam});
    const Col tube = mix(hex(p.magHex), Col(float(p.tubeR), float(p.tubeG), float(p.tubeB)), p.tubeMix);
    c.retain(n, "neon-tubes", [&](Canvas& n) {
        n.color(hex(p.magHex), p.outlineAlpha); n.rect(x0 + p.outlineX, p.outlineY, p.outlineW, p.outlineH); n.stroke(p.outlineWidth);
        for (int j = 0; j < p.glyphCount; ++j) drawSignGlyph(n, j, x0 + p.glyphX, p.glyphY + j * p.glyphStep, p.glyphSize, tube, 1.0);
    }, {s.cam});
    n.glow(x0 + p.glowX, p.glowY, p.glowRadius, hex(p.magHex), p.glowBase + p.glowKick * c.kick(5));

}
double OsakaNeonV1::level(const Ctx& c, double t, double stutter) {
    const auto& p = osakaParameters().signs;
    return (p.levelBase + p.levelSine * std::sin(t * p.levelRate) + p.levelKick * c.kick(5)) * stutter;
}
void OsakaNeonV1::submit(Ctx& c, Canvas& n, double neon) {
    const auto& p = osakaParameters().signs;
    c.gpu.over(n, float(p.overGain * neon));
    c.gpu.add(n, float(p.addGain * neon), p.addBlur);
}
}
