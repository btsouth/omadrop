#pragma once
#include "../world.h"
#include <QPainterPath>
namespace Journey::Kit {
// Q is nondimensional horizontal steepness. In normalized sea coordinates
// x=a-Q*A*sin(theta), y=-A*cos(theta); design pixels stretch the vertical
// scale independently, so amplitude can follow bass without retuning wavelength.
struct WaveTrainParametersV2 {
    double x0=-350,x1=2270,waterline=890,depth=420,wavelength=1180;
    double groupPeriod=2400,groupWidth=450,groupOrigin=400;
    Col body=hex(0x285579),bottom=hex(0x102955),underprint=hex(0x1d4673),
        foam=hex(0xdcd7ba),lines=hex(0x7397a4);
};
struct WaveTrainPoseV2 {
    double amplitude=100,q=.2,phaseSpeed=65,lipThrow=0;
    double seconds=0,distance=0,flow=0,energy=0,tempo=90;
};
struct CriticalSpringV2 {
    double value=0,velocity=0;
    void advance(double target,double omega,double dt);
};
// One instance per piece; advance at analyzer hop timestamps, never render fps.
class WaveTrainMotionV2 {
public:
    WaveTrainMotionV2();
    void advance(const Audio&,const Score&,double seconds,double dt);
    const WaveTrainPoseV2& pose() const { return pose_; }
private:
    CriticalSpringV2 amplitude_,q_,speed_,throw_;
    WaveTrainPoseV2 pose_;
};
struct WaveTrainProfileV2 {
    struct Crest { double a=0,envelope=0,q=0; std::vector<V2> outerLip; };
    std::vector<V2> surface,raw;
    std::vector<Crest> crests;
    QPainterPath silhouette;
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
