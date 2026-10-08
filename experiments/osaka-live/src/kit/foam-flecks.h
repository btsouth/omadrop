#pragma once
#include "swell-lines.h"
#include "wave-train.h"
namespace Journey::Kit {
struct FoamFlecksParametersV1 {
    SwellLinesParametersV1 swell;
    std::string waveInstance;
    bool masksWaveTrain=false;
    WaveTrainParametersV2 waveTrain;
    int count=120;
    double responseGain=0;
    double sizeMin=.25,sizeMax=1.15,onsetGain=.24;
    Col color=hex(0xdcd7ba),underprint=hex(0x0e2347);
    // Breaking crests born on onsets and kicks, rippling outward across rows
    // on the shared swell crests. Zero keeps the fleck-only foam.
    int breakers=0;
    double breakerScale=1,breakerLife=1.6;
    Col face=hex(0x7fb4ca),body=hex(0x2d4f67);
    // Snow dots flung from the largest crests of strong kicks.
    int spray=0;
    double sprayThreshold=.5;
};
struct FoamFlecksV1 {
    static constexpr const char* name="foam-flecks-v1";
    static QPainterPath exclusionPath(const Ctx&,const FoamFlecksParametersV1&);
    static void paint(Canvas&,const Ctx&,const FoamFlecksParametersV1&);
    static void draw(Ctx&,const FoamFlecksParametersV1&);
};
}
