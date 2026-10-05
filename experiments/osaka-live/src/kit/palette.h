#pragma once
#include "../canvas.h"

namespace Journey::Kit {
inline const Col CREAM = hex(0xF7E8B2), AMBER = hex(0xE9B45A), RED = hex(0xFF5345), CYAN = hex(0x2DD5B7);
inline const Col MAG = hex(0xD2689C), BCYAN = hex(0x8CD3CB), BBLUE = hex(0xACD4CF), JADE = hex(0x509475);
inline const Col GLOW(0.30f, 0.86f, 0.60f), MINT(0.62f, 0.95f, 0.78f);
inline const Col INK(0.012f, 0.030f, 0.024f), INK2(0.030f, 0.070f, 0.056f), RIM(0.36f, 0.72f, 0.56f);
inline const Col WARM_T(0.99f, 0.78f, 0.42f), WARM_B(0.98f, 0.90f, 0.66f);
inline const Col SHADOW(0.32f, 0.16f, 0.05f);
inline const Col PULSE[6] = {WARM_B, WARM_B, CYAN, CYAN, BBLUE, BBLUE};
}
