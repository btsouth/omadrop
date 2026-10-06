#pragma once
#include "swell-lines.h"
#include "great-wave.h"
namespace Journey::Kit {
struct BoatOnWaterParametersV1 {
    bool surgeEnabled=false,gullVisits=false;
    std::string waterInstance,waveInstance;
    GreatWaveParametersV1 wave;
    WaveTrainParametersV2 waveTrain;
    bool ridesWaveTrain=false;
    bool ridesWave=false;
    SwellLinesParametersV1 swell; // resolved from the named surface, never a second sea
    double x=1400, row=7, length=300, scale=.6;
    double driftX=55, driftRows=.15, driftSpeed=.012;
    int crewCount=5, oarCount=5, seed=101, band=0;
    double rowingTempo=.34, tempoGain=.22, splashGain=.28, kickGain=.18;
    Col hull=hex(0xc0a36e), trim=hex(0xe6c384), ink=hex(0x14141c), foam=hex(0xdcd7ba);
};
struct BoatOnWaterPoseV1 {
    V2 at; double row=0, tilt=0, waterline=0, stroke=0, splash=0, spray=0, brace=0;
};
struct BoatGullPoseV1 {V2 at;double flap=0,alpha=0;bool landed=false;};
struct BoatOnWaterV1 {
    static constexpr const char* name="boat-on-water-v1";
    static BoatGullPoseV1 gullPose(const Ctx&,const BoatOnWaterParametersV1&);
    static V2 keel(const BoatOnWaterParametersV1&,double u);
    static double surfaceY(const Ctx&,const BoatOnWaterParametersV1&,double x,double row);
    static BoatOnWaterPoseV1 pose(const Ctx&,const BoatOnWaterParametersV1&);
    static QPainterPath responsePath(const BoatOnWaterPoseV1&,const BoatOnWaterParametersV1&);
    static void paint(Canvas&,const Ctx&,const BoatOnWaterParametersV1&);
    static void draw(Ctx&,const BoatOnWaterParametersV1&);
};
}
