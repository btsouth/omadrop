#pragma once
#include "../score.h"
#include "../art.h"
#include <vector>
namespace Journey { struct Ctx; class Canvas; }
#include <array>
namespace Journey::Kit {
// Seven cubic segments, 22 corresponding controls in every hero stage.
struct WaveTrainParametersV2 {
    double x0=-350,x1=2270,waterline=930,depth=360,wavelength=1180;
    double groupPeriod=2400,groupWidth=620,groupOrigin=400;
    double groupFloor=0,baseY=1080,width=1060,heightScale=1,travelScale=1,row=7.2;
    // Optional screen-space life of each crest: it grows between riseFrom and
    // riseTo, then sinks between sinkFrom and sinkTo (crest anchor x).
    double riseFrom=0,riseTo=0,sinkFrom=0,sinkTo=0;
    // Life of the standing wave: faceFlow is how fast its material lines roll
    // across the face into the barrel, swayGain rocks it forward and back
    // once every two beats, pulseGain lifts it on each bass hit.
    double faceFlow=.14,swayGain=0,pulseGain=0;
    // pulseDelay holds each bass lift until the rolling swell reaches the
    // wave's row; footSwell scales the small swell drawn before the hero's foot.
    double pulseDelay=0,footSwell=1;
    bool surgeEnabled=false;
    // The shared set clock moves the crests: slow approach while the music
    // charges a set, a fast break that sinks the crest whole when it lands.
    bool setCycle=false;
    Col body=hex(0x285579),bottom=hex(0x102955),underprint=hex(0x1d4673),
        foam=hex(0xdcd7ba),lines=hex(0x7397a4);
};
constexpr int WaveTrainFingerCountV2=38,WaveTrainDropletLimitV2=192;
struct WaveTrainDropletV2 {
    V2 position,velocity;double age=0,life=0,size=0;unsigned serial=0;
};
struct WaveTrainFingerStateV2 {double extension=0,flick=0;};
// Set choreography read from the shared print clock.
struct WaveTrainCueV2 {double charge=0,approach=0,crash=0,crashStart=-1000,crashAge=1e9,strength=0;};
struct WaveTrainPoseV2 {
    double amplitude=800,stage=0,baseWidth=1060,lean=0,phaseSpeed=65,lipThrow=0;
    double lipStage=-1; // negative selects the direct static study profile
    double seconds=0,distance=0,flow=0,energy=0,tempo=90,sway=0;
    // Crest anchor travel when a set cycle drives the crests, the
    // quiet settle and the current break.
    double anchor=0,quiet=0,crashStart=-1000,crashAge=1e9,crashStrength=0,crashStage=0;
    std::array<double,6> bands{};
    std::array<WaveTrainFingerStateV2,WaveTrainFingerCountV2> fingers{};
    std::array<WaveTrainDropletV2,WaveTrainDropletLimitV2> droplets{};
};
struct CriticalSpringV2 {
    double value=0,velocity=0;
    void advance(double target,double omega,double dt);
};
// Reusable detail controller; the lab can hold the body at an authored stage.
class WaveTrainFoamMotionV2 {
public:
    void advance(const Audio&,const Score&,WaveTrainPoseV2&,double seconds,double dt,const WaveTrainParametersV2& = {});
private:
    double burstCrash_=-1000;bool impactThrown_=true;
    std::array<CriticalSpringV2,WaveTrainFingerCountV2> fingerLength_{};
    std::array<double,WaveTrainFingerCountV2> flick_{},flickVelocity_{};
    std::array<double,6> previousBand_{},lastBandOnset_{{-1,-1,-1,-1,-1,-1}};
    std::array<V2,WaveTrainFingerCountV2> previousTip_{};
    bool tipsReady_=false;
    unsigned rng_=0x75a31f29,serial_=0;
    std::uint64_t lastKick_=0;
    double sprayClock_=0;
};
class WaveTrainMotionV2 {
public:
    WaveTrainMotionV2();
    void advance(const Audio&,const Score&,double seconds,double dt,const WaveTrainParametersV2& = {},const WaveTrainCueV2& = {});
    const WaveTrainPoseV2& pose() const { return pose_; }
private:
    CriticalSpringV2 amplitude_,stage_,speed_,throw_,lean_,quiet_;
    double setU_=-1,setBase_=0,crashFrom_=0,lastCrash_=-1000;bool crashing_=false;
    double lip_=0,lipVelocity_=0;
    WaveTrainPoseV2 pose_;
    WaveTrainFoamMotionV2 foam_;
};
struct WaveTrainProfileV2 {
    // Tapered foam stroke; paint adds the offset blue underprint.
    struct Strand {std::vector<V2> centre,left,right;double alpha=1;};
    struct Finger {
        V2 root,tip;double lipIndex=0,length=0,angle=0,opacity=1;int band=0,clump=0;bool tendril=false;
        std::vector<V2> centre,left,right;
        std::vector<Strand> twigs;
        Strand shadow; // pointed blue claw rising behind the cream one
    };
    struct Clump {int count=0;double begin=0,end=0;};
    struct Whitecap {std::vector<V2> edge,inside;};
    struct Crest {
        double a=0,envelope=0,stage=0,sink=0,drop=0;
        std::vector<V2> boundary,outerLip,foamRim,foamInside;
        std::array<std::vector<V2>,16> contours;
        std::array<std::vector<V2>,6> bandEdges; // fixed outer-to-inner tone bands
        // Tone stripes rolling across the face on the flow clock; each closed
        // outline carries its tone index and whether its leading edge is keyed.
        struct FlowBand {std::vector<V2> outline,lead;int tone=0;};
        std::vector<FlowBand> flowBands;
        std::array<double,16> contourAlpha{};
        std::vector<Finger> fingers;
        std::vector<Strand> lace,tangle,falling;
        std::vector<V2> snow;std::vector<double> snowSize; // drifting foam dots
        std::vector<Whitecap> whitecaps;
        std::vector<Clump> clumps;double foamThickness=0,gapFraction=1;
    };
    std::vector<V2> surface;
    std::vector<Whitecap> swellCaps;
    std::vector<Crest> crests;
    std::vector<std::vector<V2>> boundaries;
    int hero=-1;
    V2 impact{};double impactSink=0; // forward lip of the breaking set
};
struct WaveTrainV2 {
    static constexpr const char* name="wave-train-v2";
    static WaveTrainPoseV2 worldPose(const Ctx&);
    static const WaveTrainProfileV2& worldProfile(const Ctx&,const WaveTrainParametersV2&);
    static double surfaceY(const WaveTrainProfileV2&,double x,double sea);
    // Full rendered lip/details, not just the cubic body.
    static double exclusionRight(const WaveTrainProfileV2::Crest&);
    static double envelope(double a,const WaveTrainPoseV2&,const WaveTrainParametersV2&);
    static WaveTrainProfileV2 profile(const WaveTrainPoseV2&,const WaveTrainParametersV2&);
    static void paint(Canvas&,const WaveTrainProfileV2&,const WaveTrainPoseV2&,const WaveTrainParametersV2&);
    static void drawWorld(Ctx&,const WaveTrainParametersV2&);
    static void draw(Ctx&,const WaveTrainPoseV2&,const WaveTrainParametersV2&);
};
}
