#pragma once
#include "../art.h"
#include <array>
#include <vector>
namespace Journey { class Canvas; }
namespace Journey::Kit {
// An authored great wave in the manner of Hokusai's print. Seven key poses
// (swell, rise, crest, curl, plunge, impact, wash) each give ten face points
// and ten back points in matching order; every layer derives from those two
// lines, so any phase between two poses interpolates point by point.
// The foam edge is one cream mass broken into hooked talons, not separate
// claws, and the landing throws sheets of water edged with the same talons.
struct HeroWaveStateV1 {
    double phase=0;          // 0 swell, 1 rise, 2 crest, 3 curl, 4 plunge, 5 impact, 6 wash
    double sink=0,shift=0;   // lowered whole into the sea (0..1) and moved sideways
    double scale=1;          // each set has its own size, about the foot
    double seconds=0,flow=0; // clocks: flow runs faster when the music is loud
    double sway=0,energy=0,pulse=0,flick=0; // beat sway, loudness, bass lift, onset flick
    double grasp=0;          // smoothed reach of the claws toward the viewer
    std::array<double,4> hitAge{{1e9,1e9,1e9,1e9}},hitStrength{}; // recent loud hits throw spray
    unsigned seed=1;
};
struct HeroImpactV1 {
    double age=1e9,strength=0; // seconds since the lip landed
    V2 at{1112,945};
    unsigned seed=1;
    bool active() const { return age>=0 && age<10; }
};
struct HeroWaveShapeV1 {
    std::vector<V2> face,back; // matched samples, foot to lip tip
    double grow[3]{};          // outer crest, inner crest, hanging fringe
    double mantleFrom=.22,mantleTo=.64,mantleDepth=.3,tongues=1;
    double lipFrom=.42,lipDepth=.13,stripeTo=.6,backTone=.55,specks=1,lean=0;
    V2 tip() const { return back.empty()?V2{}:back.back(); }
};
struct HeroWaveV1 {
    static constexpr const char* name="hero-wave-v1";
    static constexpr int Poses=7;
    static HeroWaveShapeV1 shape(const HeroWaveStateV1&);
    // Closed body: the back from foot to tip, then the face back to its foot.
    static std::vector<V2> outline(const HeroWaveShapeV1&,int count=112);
    static std::vector<V2> outerLip(const HeroWaveShapeV1&);
    static double right(const HeroWaveShapeV1&);
    static void paint(Canvas&,const HeroWaveShapeV1&,const HeroWaveStateV1&);
    // The landing: water thrown up in talon-edged sheets, a crown along the
    // sea, spray, spreading rings and foam lace that lingers.
    static void paintImpact(Canvas&,const HeroImpactV1&,double seconds);
    // Two near swells across the print, in front of the foot and the landing.
    static void paintFoot(Canvas&,double seconds,double flow,double energy,double pulse);
};
}
