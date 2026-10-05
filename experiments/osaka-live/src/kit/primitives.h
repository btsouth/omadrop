#pragma once
#include "../world.h"
#include "palette.h"

namespace Journey::Kit {
struct OsakaWarmPaneV1 {
    static constexpr const char* name = "osaka-warm-pane-v1";
    static void draw(Canvas& cv, double x, double y, double w, double h, double level, const Col* tint);
};
inline void warmPane(Canvas& cv, double x, double y, double w, double h, double level, const Col* tint = nullptr) { OsakaWarmPaneV1::draw(cv, x, y, w, h, level, tint); }

struct OsakaDarkPaneV1 {
    static constexpr const char* name = "osaka-dark-pane-v1";
    static void draw(Canvas& cv, double x, double y, double w, double h);
};
inline void darkPane(Canvas& cv, double x, double y, double w, double h) { OsakaDarkPaneV1::draw(cv, x, y, w, h); }

struct OsakaLatticeV1 {
    static constexpr const char* name = "osaka-lattice-v1";
    static void draw(Canvas& cv, double x, double y, double w, double h, int cols, int rows, Col frame, double lw);
};
inline void lattice(Canvas& cv, double x, double y, double w, double h, int cols, int rows, Col frame = INK, double lw = 1.5) { OsakaLatticeV1::draw(cv, x, y, w, h, cols, rows, frame, lw); }

struct OsakaRoofV1 {
    static constexpr const char* name = "osaka-roof-v1";
    static void draw(Canvas& cv, double x0, double x1, double ye, double yr, double ov, Col col, Col col2, Col rim, double th, bool tiles, double rimA);
};
inline void roof(Canvas& cv, double x0, double x1, double ye, double yr, double ov, Col col, Col col2, Col rim = RIM, double th = 7, bool tiles = true, double rimA = 0.55) { OsakaRoofV1::draw(cv, x0, x1, ye, yr, ov, col, col2, rim, th, tiles, rimA); }

}
