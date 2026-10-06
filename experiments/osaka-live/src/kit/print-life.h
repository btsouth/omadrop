#pragma once
#include "swell-lines.h"
namespace Journey::Kit {
// Model-space ink pieces. Placement and palette belong to the world folder.
struct PrintLifeParametersV1 {
    double x=0,y=0,width=1920,height=500,scale=1,gain=1,speed=1;
    int count=7,seed=71,band=0;
    bool surgeEnabled=false;
    std::string waterInstance;
    SwellLinesParametersV1 swell;
    Col color=hex(0x7397a4),accent=hex(0xdcd7ba),ink=hex(0x223249);
};
struct SmokePlumeV1 {static constexpr const char* name="smoke-plume-v1";static void paint(Canvas&,const Ctx&,const PrintLifeParametersV1&);};
struct PrintMomentsV1 {static constexpr const char* name="print-moments-v1";static void paint(Canvas&,const Ctx&,const PrintLifeParametersV1&);};
}
