#include "headless.h"
#include "kit/boat-on-water.h"
#include <QCoreApplication>
#include <iostream>
#include <stdexcept>
using namespace Journey;using namespace Journey::Kit;
void require(bool b,const char* text){if(!b)throw std::runtime_error(text);}
int main(int argc,char** argv){QCoreApplication app(argc,argv);try{
 HeadlessContext context;QString error;require(context.create(error),"EGL failed");
 Gpu gpu;require(gpu.init(error),"GPU init failed");BoatOnWaterParametersV1 p;
 Score score;Audio audio;std::vector<std::unique_ptr<Canvas>> canvases;Ctx c{gpu,14,audio,&score,1,&canvases,nullptr};
 auto capture=[&](){gpu.begin(960,540);Canvas cv(gpu.pixelScale());BoatOnWaterV1::paint(cv,c,p);gpu.over(cv);
    FinishParams f;f.bloom=f.vignette=f.grain=0;gpu.finish(f,nullptr);std::vector<unsigned char> rgb;gpu.readRgb(rgb);return rgb;};
 const auto quiet=capture();require(quiet==capture(),"boat RGB not deterministic");
 const auto rest=BoatOnWaterV1::pose(c,p);score.bandIntegrals[p.band]=3;
 require(BoatOnWaterV1::pose(c,p).stroke>rest.stroke,"integral does not change rowing clock");
 require(capture()!=quiet,"music does not move jointed rowers and oars");
 score={};p.tempoGain=0;p.swell.amplitude=p.swell.bandGain=p.swell.liftGain=p.swell.kickGain=0;
 c.t=(1.2-hash2(p.seed,43))/p.rowingTempo;
 const auto flat=capture();c.a.bands[p.band]=.9;
 require(BoatOnWaterV1::pose(c,p).splash>rest.splash && flat!=capture(),"band does not light oar contact rings");
 c.a={};score.bassHits.push_back({c.t,1,1});
 require(BoatOnWaterV1::pose(c,p).spray>0 && flat!=capture(),"kick does not spray at bow");
 score={};p=BoatOnWaterParametersV1{};const auto seeded=capture();p.seed++;
 require(seeded!=capture(),"boat seed ignored");p=BoatOnWaterParametersV1{};c.t=194;
 require(capture()!=quiet,"boat visibly repeats after three minutes");
 // Eight hours, music/silence, fractional depths and continuous seeded lanes.
 double maxContact=0,maxPitchError=0;BoatOnWaterPoseV1 previous;bool first=true;
 for(int i=0;i<=28800;++i){c.t=i;c.a.bands.fill(i%3?.8:0);for(auto& body:score.bandBody)body.fill(i%3?.8:0);
    const auto s=BoatOnWaterV1::pose(c,p);const double half=p.length*p.scale*.45;
    const double l=BoatOnWaterV1::surfaceY(c,p,s.at.x-half,s.row),r=BoatOnWaterV1::surfaceY(c,p,s.at.x+half,s.row);
    const double expected=(l+2*BoatOnWaterV1::surfaceY(c,p,s.at.x,s.row)+r)*.25;
    maxContact=std::max(maxContact,std::abs(s.at.y+14*p.scale*std::cos(s.tilt)-expected));
    maxPitchError=std::max(maxPitchError,std::abs(std::tan(s.tilt)-(r-l)/(2*half)));
    const int lo=int(s.row);const double x=s.at.x;
    const double shared=lerp(SwellLinesV1::field(c,lo,p.swell).y(x),SwellLinesV1::field(c,lo+1,p.swell).y(x),s.row-lo);
    require(std::abs(shared-BoatOnWaterV1::surfaceY(c,p,x,s.row))<1e-8,"not the swell-lines field");
    require(std::isfinite(s.at.y) && std::abs(s.at.x-p.x)<=p.driftX && std::abs(s.row-p.row)<=p.driftRows,"unbounded boat path");
    if(!first)require(std::abs(s.at.x-previous.at.x)<1 && std::abs(s.row-previous.row)<.01,"lane wrap or path seam");
    previous=s;first=false;
 }
 require(maxContact<.5 && maxPitchError<1e-8,"boat misses weighted swell support tolerance");
 std::cout<<"PASS: exact RGB, rowing integral, band splash, kick spray, seed, eight-hour bounded seamless drift; contact error "<<maxContact<<" px (tolerance 0.5), pitch error "<<maxPitchError<<'\n';
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
