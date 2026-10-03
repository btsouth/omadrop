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
    std::function<void()> afterTown;   // after downhill roofs (fog goes here)
    std::function<void()> afterYatai;  // the boat at the quay
    std::function<void()> afterWires;
};
void drawOsakaBackdrop(Ctx& c, const OsakaState& s, bool disc, bool mountain);
void drawOsakaForeground(Ctx& c, const OsakaState& s, const OsakaHooks& hooks);
void osakaFog(Ctx& c, double amount, double top, double bottom, Col col);
// Diffuse glow of the valley city and downhill town seen through fog.
void osakaGlowThrough(Ctx& c, const OsakaState& s, double amount);
std::array<std::array<V2, 4>, 6> osakaOutRuns(double cam);
// Lantern-bearer position (mock x) at movie time t.
double bearerX(double t);

// Kanagawa.
struct KState {
    double t = 0;        // movie time
    double grade = 1;    // 0 = jade duotone, 1 = woodblock palette
    double rise = 0;     // great wave growth
    double breakAt = -1; // wave break time
    bool sun = true, boats = true, wave = true, plovers = true;
};
void kanagawaSky(Ctx& c);
void kanagawaMist(Ctx& c);
void kanagawaSea(Ctx& c);
void kanagawaSun(Ctx& c);
void kanagawaFuji(Ctx& c, double grade);
void kanagawaForeground(Ctx& c, const KState& k);
void gradeComposite(Ctx& c, int tex, double grade, double opacity);
std::array<std::vector<V2>, 6> waveStrands(double t, double rise);
// Crossing: six lines on the water, handed to the wave strands in Kanagawa.
void crossingStrands(Ctx& c, double t);
double crossingCam(double t);
// The flock formation after the surge, in screen space; colour per world.
void drawFlock(Ctx& c, Col col, double size);
double kanagawaRise(const Ctx& c);
Surge kanagawaBreak(const Ctx& c);
Col duotone(Col c, double g);
// Boat shared by the crossing (ink) and Kanagawa (woodblock); style 0..1.
// bearerAt: 0 stern .. 1 bow; bearerH: figure height (0 = 74 * scale).
void drawBoat(Ctx& c, Canvas& body, Canvas& light, V2 at, double len, double tilt, int crew, double style, double scale,
              bool bearer, double stroke, double bearerAt = 1, double bearerH = 0, double bearerWalk = 0);
// Six lines on the water in the crossing: from touchdown to the right.
}
