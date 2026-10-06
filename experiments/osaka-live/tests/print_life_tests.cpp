#include "headless.h"
#include "kit/print-life.h"
#include <QCoreApplication>
#include <iostream>
#include <stdexcept>
using namespace Journey;using namespace Journey::Kit;
void need(bool b,const char*s){if(!b)throw std::runtime_error(s);}
int main(int argc,char**argv){QCoreApplication app(argc,argv);try{
 const std::string mode=argc>1?argv[1]:"smoke";HeadlessContext context;QString error;need(context.create(error),"EGL");Gpu gpu;need(gpu.init(error),"GPU");
 Score score;Schedule schedule(1);std::vector<std::unique_ptr<Canvas>> canvases;Ctx c{gpu,14,{},&score,1,&canvases,&schedule};
 PrintLifeParametersV1 p;p.x=900;p.y=600;p.width=110;p.height=170;p.surgeEnabled=true;
 auto paint=[&](Canvas&cv){if(mode=="smoke")SmokePlumeV1::paint(cv,c,p);else if(mode=="birds")BirdFlockV1::paint(cv,c,p);else if(mode=="fish")LeapingFishV1::paint(cv,c,p);else if(mode=="dragon")SeaCreatureV1::paint(cv,c,p);else PrintMomentsV1::paint(cv,c,p);};
 auto capture=[&](){gpu.begin(640,360);Canvas cv;paint(cv);need(!cv.empty(),"missing expected piece");
 for(const auto&v:cv.vertices())need(std::isfinite(v.x)&&std::isfinite(v.y),"nonfinite geometry");
 gpu.over(cv);FinishParams f;f.bloom=f.vignette=f.grain=0;gpu.finish(f,nullptr);std::vector<unsigned char> rgb;gpu.readRgb(rgb);return rgb;};
 if(mode=="smoke"){
 auto quiet=capture();need(quiet==capture(),"plume nondeterministic");for(auto&v:score.bandBody)v.fill(.8);c.a.bands.fill(.8);
 need(quiet!=capture(),"plume ignores bass");auto loud=capture();schedule.print.surgeStart=12;need(loud!=capture(),"eruption missing");
 score.bassHits={{13.9,1,1}};need(loud!=capture(),"vent beat missing");
 }else if(mode=="birds"){
 schedule.print.flock.start=10;schedule.print.flock.duration=12;schedule.print.flock.count=7;schedule.print.flock.height=.5;
 p.x=0;p.y=300;p.width=1920;p.height=180;
 auto birds=BirdFlockV1::poses(c,p);need(birds.size()==7,"flock count");auto original=birds[0];
 p.nearEvents=true;p.y=160;const double highSize=BirdFlockV1::poses(c,p)[0].size;
 p.y=550;const double horizonSize=BirdFlockV1::poses(c,p)[0].size;
 need(horizonSize<highSize*.4,"horizon birds do not recede");
 p.nearEvents=false;p.y=300;auto rgb=capture();need(rgb==capture(),"bird RGB nondeterminism");
 score.onsets={{13.9,1,1}};need(BirdFlockV1::poses(c,p)[0].flap!=original.flap,"onset ignores flap");need(rgb!=capture(),"flap not rendered");
 schedule.print.surgeStart=12;need(BirdFlockV1::poses(c,p)[0].y<original.y,"surge ignores scatter");
 schedule.print.surgeStart=-1000;c.t=25;need(BirdFlockV1::poses(c,p).empty(),"birds fail to exit");
 }else if(mode=="dragon"){
 c.t=240;schedule.print.dragon.start=234;schedule.print.dragon.duration=18;p.x=1515;p.width=230;p.height=95;p.scale=.85;
 auto rgb=capture();need(rgb==capture(),"dragon nondeterminism");for(auto&v:score.bandBody)v.fill(.8);need(rgb!=capture(),"dragon ignores bass");
 p.x=1240;p.width=110;p.height=38;p.scale=.32;p.row=.65;
 Canvas distant;paint(distant);auto bounds=distant.bounds();
 need(bounds[0]>1150 && bounds[2]<1340 && bounds[1]>565 && bounds[3]<695,"distant creature overlaps boat lanes");
 auto small=capture();need(small==capture() && small!=rgb,"distant creature not distinct/deterministic");
 c.t=260;Canvas cv;paint(cv);need(cv.empty(),"dragon fails to submerge");
 }else if(mode=="fish"){
 c.t=14;schedule.print.events[int(PrintMoment::Fish)].start=13;schedule.print.events[int(PrintMoment::Fish)].direction=1;
 p.x=1000;p.width=500;p.height=100;p.row=9;
 auto rgb=capture();need(rgb==capture(),"fish nondeterminism");c.a.bands[0]=.8;need(rgb!=capture(),"fish ignores bass");p.seed++;need(rgb!=capture(),"fish ignores seed");
 c.t=25;Canvas cv;paint(cv);need(cv.empty(),"fish fails to land");
 }else if(mode=="swell"){
 p.domain=3;p.x=0;p.y=1010;p.width=1920;p.height=130;
 schedule.print.foregroundSwell.start=12;schedule.print.foregroundSwell.duration=11;c.t=17;
 auto rgb=capture();need(rgb==capture(),"foreground swell nondeterminism");
 Canvas cv;paint(cv);auto bounds=cv.bounds();need(bounds[1]>940 && bounds[3]<=1141,"foreground swell leaves bottom band");
 for(auto&v:score.bandBody)v.fill(.8);score.bassHits={{16.9,1,1}};need(rgb!=capture(),"foreground crest ignores music");
 c.t=25;Canvas done;paint(done);need(done.empty(),"foreground swell fails to settle");
 }else{
 p.x=0;p.y=310;p.width=1920;p.height=280;
 for(int i=0;i<int(PrintMoment::Count);++i){auto&v=schedule.print.events[i];v.start=10;v.duration=8;v.direction=1;v.height=.5;}
 for(int domain=0;domain<3;++domain){p.domain=domain;if(domain==2){p.x=1086;p.y=476;}score={};c.a={};auto rgb=capture();need(rgb==capture(),"moment RGB nondeterminism");
 c.a.bands.fill(.8);for(auto&v:score.bandBody)v.fill(.8);score.onsets={{c.t-.1,1,1}};score.bandIntegrals.fill(2);
 need(rgb!=capture(),"moment ignores music");c.t+=.1;need(rgb!=capture(),"moment has no motion");}
 c.t=30;Canvas cv;paint(cv);need(cv.empty(),"moments fail to finish");
 }
 std::cout<<"PASS "<<mode<<": exact RGB, parameters, music or motion, complete exit\n";
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
