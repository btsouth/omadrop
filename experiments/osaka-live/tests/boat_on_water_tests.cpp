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
 // Foreground hull vertices never enter the drawn wave body as sets pass.
 p.ridesWave=true;p.x=660;p.row=7.6;p.wave.baseHeight=340;p.wave.maxRise=650;
 Schedule schedule(1);c.schedule=&schedule;score={};audio.bands.fill(.4);
 double lowestClearance=1e9,highestLift=0,largestStep=0,lastY=0;
 for(int i=1;i<=180*60;++i){c.t=i/60.;score.advance(audio,c.t,1./60);schedule.advance(c.t,audio,score);
   auto far=p;far.x=1140;far.row=5.6;far.scale=.5;far.length=285;
   require(BoatOnWaterV1::pose(c,far).at.y>600,"distant boat rides foreground height");
   const auto boat=BoatOnWaterV1::pose(c,p);require(std::abs(boat.tilt)<=.55,"boat tips over");const auto field=GreatWaveV1::field(c,p.wave);
   for(int j=0;j<=16;++j){double qx=p.length*p.scale*(j/16.-.5)*1.18;
     const double x=boat.at.x+qx*std::cos(boat.tilt)-26*p.scale*std::sin(boat.tilt);
     const double y=boat.at.y+qx*std::sin(boat.tilt)+26*p.scale*std::cos(boat.tilt);
     const double clearance=BoatOnWaterV1::surfaceY(c,p,x,boat.row)-y;
     lowestClearance=std::min(lowestClearance,clearance);require(clearance>=-1e-6,"hull enters wave height field");}
   auto swellOnly=p;swellOnly.ridesWave=false;
   highestLift=std::max(highestLift,BoatOnWaterV1::pose(c,swellOnly).at.y-boat.at.y);
   if(i>1)largestStep=std::max(largestStep,std::abs(boat.at.y-lastY));lastY=boat.at.y;
 }
 require(highestLift>60 && largestStep<12,"boat does not ride smoothly over passing set");
 // Compare the rigid hull's support against actual body triangles, not only
 // the field implementation. All samples must stay outside the water fill.
 for(int i=0;i<180;++i){c.t=i;Canvas body,flow,foam;GreatWaveV1::paint(body,flow,foam,c,p.wave);
   const auto boat=BoatOnWaterV1::pose(c,p);const auto&vertices=body.vertices();
   for(int j=0;j<=10;++j){double qx=p.length*p.scale*(j/10.-.5);
     V2 q=boat.at+V2(qx*std::cos(boat.tilt)-25*p.scale*std::sin(boat.tilt),qx*std::sin(boat.tilt)+25*p.scale*std::cos(boat.tilt));
     for(const auto&cmd:body.commands())for(int k=cmd.first;k+2<cmd.first+cmd.count;k+=3){
       auto a=vertices[k],b=vertices[k+1],v=vertices[k+2];
       auto cross=[&](auto a,auto b){return (b.x-a.x)*(q.y-a.y)-(b.y-a.y)*(q.x-a.x);};
       double x=cross(a,b),y=cross(b,v),z=cross(v,a);
       require(!((x>1e-5 && y>1e-5 && z>1e-5)||(x<-1e-5 && y<-1e-5 && z<-1e-5)),"hull inside rendered wave triangles");
     }
   }
 }
 std::cout<<"Wave hull minimum clearance "<<lowestClearance<<", maximum lift "<<highestLift<<", maximum step "<<largestStep<<" px\n";
 require(maxContact<.5 && maxPitchError<1e-8,"boat misses weighted swell support tolerance");
 std::cout<<"PASS: exact RGB, rowing integral, band splash, kick spray, seed, eight-hour bounded seamless drift; contact error "<<maxContact<<" px (tolerance 0.5), pitch error "<<maxPitchError<<'\n';
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
