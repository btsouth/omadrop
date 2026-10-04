#pragma once

// Persistent elements shared by every world: the bass disc, the mountain,
// soft haze bands, and the Osaka stage pieces the crossing carries away.
#include "world.h"
#include <functional>
#include <vector>

namespace Journey {
struct DiscLook {
    V2 pos;
    double r = 108;
    Col col, col2, halo, ring;
    double veil = 1, tex = 1, energy = 0.5;
    double haloA = 70, haloB = 40, haloC = 0.30, haloD = 0.2, haloFar = 0.13;
    double restRings = 1.0;
};
void drawDisc(Ctx& c, const DiscLook& d, double camForClouds);

struct MountainLook {
    double px, peak, base, width;
    Col top, bot;
    double snow = 0, snowScale = 1, foot = 30;
    Col snowCol;
    double alpha = 1;
};
void drawMountain(Ctx& c, const MountainLook& m);

void hazeBand(Ctx& c, double y0, double sigma, double lo, double hi, double shift, double seed, Col col,
              double gain, V2 noise = {480, 108});

// Osaka stage split so the crossing can interleave fog and the next world.
struct OsakaHooks {
    std::function<void()> town; // optional crossing shoreline compositor
    std::function<void()> afterTown;   // after downhill roofs (fog goes here)
    std::function<void()> afterYatai;  // the boat at the quay
    std::function<void()> afterWires;
    std::function<void()> afterReflections; // crossing boats above the street reflection
};
// The crossing replaces the sky's neighbours, the disc and the mountain at
// their place in the backdrop's depth order.
struct BackdropHooks {
    std::function<void()> afterSky;
    std::function<void()> disc;
    std::function<void()> mountain;
    std::function<void()> beforeCoast; // sea behind the retiring shoreline
    std::function<void()> coast; // optional crossing shoreline compositor
    std::function<void()> afterValley; // distant fog, before the nearer ridge
};
void drawOsakaBackdrop(Ctx& c, const OsakaState& s, bool disc, bool mountain, const BackdropHooks* hooks = nullptr);
void drawOsakaForeground(Ctx& c, const OsakaState& s, const OsakaHooks& hooks);
void drawOsakaCoast(Ctx& c, const OsakaState& s, const BackdropHooks* hooks);
void drawOsakaDistantTown(Ctx& c, const OsakaState& s);
void osakaFog(Ctx& c, double amount, double top, double bottom, Col col);
// Diffuse glow of the valley city and downhill town seen through fog.
void osakaGlowThrough(Ctx& c, const OsakaState& s, double amount);
std::array<std::array<V2, 4>, 6> osakaOutRuns(double cam);
// Lantern-bearer position (mock x) at movie time t.
double bearerX(double t);

}
