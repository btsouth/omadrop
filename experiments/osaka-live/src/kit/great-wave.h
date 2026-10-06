#pragma once
#include "../parts.h"
#include <QRectF>
namespace Journey::Kit {
struct GreatWaveParametersV1 {
    bool surgeEnabled=false;
    bool anchorRight=false;
    double x=0,y=1100,width=1200,baseHeight=650,maxRise=210,curlAmount=.65;
    int clawCount=18,seed=71;
    double clawSize=1,lowGain=2.4,swellGain=.35,kickGain=.22,onsetGain=.18;
    Col body=hex(0x285579),bottom=hex(0x102955),underprint=hex(0x1d4673),
        foam=hex(0xdcd7ba),lines=hex(0x7397a4);
};
// A travelling set settles into the sea. There is no plunge or crash state.
struct GreatWavePoseV1 {
    double rise=0,height=0,curl=0,flick=0;
    double time=0,phase=0,growth=0,travel=0,energy=0,setStart=0,setDuration=36;
    unsigned setCycle=0;
};
struct WaveTipResponseV1 { int band=0; double drive=0,extension=1,flick=0; };
struct GreatWaveV1 {
    static constexpr const char* name="great-wave-v1";
    static GreatWavePoseV1 pose(const Ctx&,const GreatWaveParametersV1&);
    static WaveTipResponseV1 tip(const Ctx&,const GreatWaveParametersV1&,int finger);
    static V2 map(V2,const GreatWavePoseV1&,const GreatWaveParametersV1&);
    static QRectF responseArea(const GreatWaveParametersV1&);
    static void paint(Canvas& body,Canvas& flow,Canvas& foam,const Ctx&,const GreatWaveParametersV1&);
    static void draw(Ctx&,const GreatWaveParametersV1&);
};
}
