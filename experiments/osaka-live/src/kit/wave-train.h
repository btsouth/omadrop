#pragma once
#include "../world.h"
#include <array>
namespace Journey::Kit {
// Seven cubic segments, 22 corresponding controls in every hero stage.
struct WaveTrainParametersV2 {
    double x0=-350,x1=2270,waterline=930,depth=360,wavelength=1180;
    double groupPeriod=2400,groupWidth=620,groupOrigin=400;
    Col body=hex(0x285579),bottom=hex(0x102955),underprint=hex(0x1d4673),
        foam=hex(0xdcd7ba),lines=hex(0x7397a4);
};
struct WaveTrainPoseV2 {
    double amplitude=800,stage=0,baseWidth=1060,lean=0,phaseSpeed=65,lipThrow=0;
    double lipStage=-1; // negative selects the direct static study profile
    double seconds=0,distance=0,flow=0,energy=0,tempo=90;
};
struct CriticalSpringV2 {
    double value=0,velocity=0;
    void advance(double target,double omega,double dt);
};
class WaveTrainMotionV2 {
public:
    WaveTrainMotionV2();
    void advance(const Audio&,const Score&,double seconds,double dt);
    const WaveTrainPoseV2& pose() const { return pose_; }
private:
    CriticalSpringV2 amplitude_,stage_,speed_,throw_,lean_;
    double lip_=0,lipVelocity_=0;
    WaveTrainPoseV2 pose_;
};
struct WaveTrainProfileV2 {
    struct Crest {
        double a=0,envelope=0,stage=0;
        std::vector<V2> boundary,outerLip,foamInside;
        std::array<std::vector<V2>,16> contours;
    };
    std::vector<V2> surface;
    std::vector<Crest> crests;
    std::vector<std::vector<V2>> boundaries;
    int hero=-1;
};
struct WaveTrainV2 {
    static constexpr const char* name="wave-train-v2";
    static double envelope(double a,const WaveTrainPoseV2&,const WaveTrainParametersV2&);
    static WaveTrainProfileV2 profile(const WaveTrainPoseV2&,const WaveTrainParametersV2&);
    static void paint(Canvas&,const WaveTrainProfileV2&,const WaveTrainPoseV2&,const WaveTrainParametersV2&);
    static void draw(Ctx&,const WaveTrainPoseV2&,const WaveTrainParametersV2&);
};
}
