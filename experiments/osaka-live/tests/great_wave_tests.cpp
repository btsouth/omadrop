#include "headless.h"
#include "kit/great-wave.h"
#include <QCoreApplication>
#include <iostream>
#include <stdexcept>
using namespace Journey;using namespace Journey::Kit;
void require(bool b,const char* s){if(!b)throw std::runtime_error(s);}
int main(int argc,char**argv){QCoreApplication app(argc,argv);try{
 HeadlessContext context;QString error;require(context.create(error),"EGL failed");Gpu gpu;require(gpu.init(error),"GPU init failed");
 GreatWaveParametersV1 p;Score score;Audio audio;Schedule schedule(1);
 std::vector<std::unique_ptr<Canvas>> canvases;Ctx c{gpu,14,audio,&score,1,&canvases,&schedule};
 auto capture=[&](){gpu.begin(640,360);canvases.clear();c.next=0;GreatWaveV1::draw(c,p);FinishParams f;f.bloom=f.vignette=f.grain=0;gpu.finish(f,nullptr);std::vector<unsigned char> rgb;gpu.readRgb(rgb);return rgb;};
 auto quiet=capture();require(quiet==capture(),"not exact deterministic RGB");
 const auto rest=GreatWaveV1::pose(c,p);schedule.print.wave.energy=.9;
 const auto loud=GreatWaveV1::pose(c,p);require(loud.height>rest.height+100 && loud.curl>rest.curl,"set energy does not lift/curl");
 require(quiet!=capture(),"loud and quiet render identically");
 auto frozen=loud;auto moving=loud;moving.time+=3;
 const auto a=GreatWaveV1::map({220,130},frozen,p),b=GreatWaveV1::map({220,130},moving,p);
 require((b-a).len()>3,"wave is a scaled still");
 audio.bands.fill(.4);audio.bassLevel=.5;double lastHeight=0,lastX=0,maxStep=0;unsigned cycle=0;
 for(int i=0;i<=180*60;++i){c.t=i/60.;score.advance(audio,c.t,1./60);schedule.advance(c.t,audio,score);
  const auto s=GreatWaveV1::pose(c,p);require(s.setDuration>=20 && s.setDuration<=40,"set cycle outside 20-40 s");
  require(std::isfinite(s.height) && s.height>=0 && s.height<=p.baseHeight+p.maxRise,"unbounded wave height");
  require(s.curl>=0 && s.curl<=p.curlAmount,"unbounded curl");
  if(i){maxStep=std::max(maxStep,std::abs(s.height-lastHeight));
    if(cycle==s.setCycle)require(s.travel>=lastX-1e-8,"set travels backwards");
    else require(s.height<.1 && lastHeight<.1,"set pops at seam");}
  if(s.growth>.02)require(GreatWaveV1::map({1000,560},s,p).y<p.y,"lip plunges into sea");
  lastHeight=s.height;lastX=s.travel;cycle=s.setCycle;
 }
 require(cycle>=4 && maxStep<8,"missing cycles or uneased motion");
 // Across eight hours the stateless geometry stays bounded and never crashes.
 c.schedule=nullptr;for(int i=0;i<=28800;++i){c.t=i;for(auto& band:score.bandBody)band.fill(i%3?100:0);
  auto s=GreatWaveV1::pose(c,p);require(s.height>=0 && s.height<=p.baseHeight+p.maxRise,"eight-hour bound");}
 std::cout<<"PASS exact RGB, captured set energy, spatial deformation, 20-40 s travel/settle seams, no crash; max height step "<<maxStep<<" px\n";
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
