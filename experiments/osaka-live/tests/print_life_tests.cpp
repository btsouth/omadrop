#include "headless.h"
#include "kit/print-life.h"
#include <QCoreApplication>
#include <iostream>
#include <stdexcept>
using namespace Journey;using namespace Journey::Kit;
void need(bool b,const char*s){if(!b)throw std::runtime_error(s);}
int main(int argc,char**argv){QCoreApplication app(argc,argv);try{
 HeadlessContext context;QString error;need(context.create(error),"EGL");Gpu gpu;need(gpu.init(error),"GPU");
 Score score;Schedule schedule(1);std::vector<std::unique_ptr<Canvas>> canvases;Ctx c{gpu,14,{},&score,1,&canvases,&schedule};
 PrintLifeParametersV1 p;p.x=900;p.y=600;p.width=110;p.height=170;p.surgeEnabled=true;
 auto capture=[&](){gpu.begin(640,360);Canvas cv;SmokePlumeV1::paint(cv,c,p);gpu.over(cv);FinishParams f;f.bloom=f.vignette=f.grain=0;gpu.finish(f,nullptr);std::vector<unsigned char> rgb;gpu.readRgb(rgb);return rgb;};
 auto quiet=capture();need(quiet==capture(),"plume nondeterministic");for(auto&v:score.bandBody)v.fill(.8);c.a.bands.fill(.8);
 need(quiet!=capture(),"plume ignores bass");auto loud=capture();schedule.print.surgeStart=12;need(loud!=capture(),"eruption missing");
 score.bassHits={{13.9,1,1}};need(loud!=capture(),"vent beat missing");
 schedule.print.flock.start=10;schedule.print.flock.duration=12;schedule.print.flock.count=7;schedule.print.flock.height=.5;
 p.x=0;p.y=300;p.width=1920;p.height=180;score={};schedule.print.surgeStart=-1000;
 auto birds=BirdFlockV1::poses(c,p);need(birds.size()==7,"flock count");auto original=birds[0];
 score.onsets={{13.9,1,1}};need(BirdFlockV1::poses(c,p)[0].flap!=original.flap,"onset ignores flap");
 schedule.print.surgeStart=12;need(BirdFlockV1::poses(c,p)[0].y<original.y,"surge ignores scatter");
 schedule.print.surgeStart=-1000;c.t=25;need(BirdFlockV1::poses(c,p).empty(),"birds fail to exit");
 for(const auto&v:canvases)for(const auto&p:v->vertices())need(std::isfinite(p.x)&&std::isfinite(p.y),"nonfinite geometry");
 std::cout<<"PASS print smoke exact RGB, bass thickening, vent and surge embers\n";
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
