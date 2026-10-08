#pragma once
#include "../world.h"
#include "swell-lines.h"
namespace Journey::Kit {
// Reduced printed sea and calm texture from Journey ending_water.cpp. All art,
// density, placement and response are world parameters, never chapter state.
struct WaterSurfaceParametersV1 {
    bool surgeEnabled=false;
    double amplitudeGain=0;
    double horizon=612, nearY=1114, x0=-30, x1=2000;
    int rows=7, textureRows=28, glints=90, seed=5, innerLines=3, swellSeed=-1;
    double sampleStep=16, amplitude=.48, wavelength=1, drift=.52, phase=0, flowGain=1;
    Col top=hex(0x376582), bottom=hex(0x102657), crest=hex(0x143154);
    Col texture=hex(0x7397a4), foam=hex(0xe6ddbf), underprint=hex(0x0e2347);
    Col glint=hex(0xe3b87a), hotGlint=hex(0xc94a45);
    double crestOpacity=.9;
    // Woodblock bokashi per swell: light crest falling to a dark trough.
    double shade=0;Col shadeLight=hex(0x7fb4ca);
    double opacity=1, bandGain=.7, liftGain=.35, kickGain=.18;
    double capDensity=.66, capScale=.7, glintX=1310, glintDepth=350;
    // Sun glitter: a perspective column of crisp dashes under glintX that
    // twinkles with the treble and reshuffles on onsets. Zero keeps glints only.
    int glitter=0;
    double glitterWidth=22, glitterSpread=.32;
    // Shared with SwellLines (see swell-lines.h).
    double rowFreedom=.42, rollGain=0, rollDelay=.55, beatGain=0;
    // Foam rims printed on each row where it stands above its rest line;
    // louder music and the rolling bass swell raise more of the sea into foam.
    double crestFoam=0;
};
struct WaterSurfaceV1 {
    static constexpr const char* name = "water-surface-v1";
    static int band(int row, int rows);
    static double rowY(int row, const WaterSurfaceParametersV1&);
    static QRectF responseArea(int row, const WaterSurfaceParametersV1&);
    static void draw(Ctx&, const WaterSurfaceParametersV1&);
};
}
