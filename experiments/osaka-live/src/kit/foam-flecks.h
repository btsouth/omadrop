#pragma once
#include "swell-lines.h"
namespace Journey::Kit {
struct FoamFlecksParametersV1 {
    SwellLinesParametersV1 swell;
    int count=120;
    double responseGain=0;
    double sizeMin=.25,sizeMax=1.15,onsetGain=.24;
    Col color=hex(0xdcd7ba),underprint=hex(0x0e2347);
};
struct FoamFlecksV1 {
    static constexpr const char* name="foam-flecks-v1";
    static void paint(Canvas&,const Ctx&,const FoamFlecksParametersV1&);
    static void draw(Ctx&,const FoamFlecksParametersV1&);
};
}
