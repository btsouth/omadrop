#pragma once

#include "composition.h"
#include <QPainterPath>

namespace Journey::Kit {
// Music-bound SVG layers other than windows. Each draws with Osaka's lantern,
// neon, glow and strand looks and follows its band.
struct GenericLanternV1 {
    static constexpr const char* name = "generic-lantern-v1";
    static double level(const Ctx&, const OsakaPieceNodeV1&);
};
struct GenericLampV1 {
    static constexpr const char* name = "generic-lamp-v1";
    static double level(const Ctx&, const OsakaPieceNodeV1&);
};
struct GenericNeonV1 {
    static constexpr const char* name = "generic-neon-v1";
    static double level(const Ctx&, const OsakaPieceNodeV1&, std::size_t index);
};
struct GenericGlowV1 {
    static constexpr const char* name = "generic-glow-v1";
    static double level(const Ctx&, const OsakaPieceNodeV1&);
};
struct GenericWireV1 {
    static constexpr const char* name = "generic-wire-v1";
    static double hum(const Ctx&, const OsakaPieceNodeV1&);
};

struct LabelPiecesV1 {
    static void draw(Ctx&, const OsakaEventState&, const OsakaWorldDescription&);
    // Where a piece's light lands, in design pixels. The check command and the
    // tests measure brightness here.
    static QPainterPath area(const OsakaPieceNodeV1&, const SvgElement&);
};
}
