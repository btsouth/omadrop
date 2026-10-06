#include "headless.h"
#include "kit/great-wave.h"
#include <QCoreApplication>
#include <iostream>
#include <stdexcept>
using namespace Journey;using namespace Journey::Kit;
void require(bool b,const char* s){if(!b)throw std::runtime_error(s);}
int main(int argc,char** argv){QCoreApplication app(argc,argv);try{
    HeadlessContext context;QString error;require(context.create(error),"EGL failed");
    Gpu gpu;require(gpu.init(error),"GPU init failed");GreatWaveParametersV1 p;
    std::vector<std::unique_ptr<Canvas>> canvases;Score score;Audio audio;
    Ctx c{gpu,14,audio,&score,1,&canvases,nullptr};
    auto capture=[&](){gpu.begin(640,360);canvases.clear();c.next=0;GreatWaveV1::draw(c,p);
        FinishParams f;f.bloom=f.vignette=f.grain=0;gpu.finish(f,nullptr);std::vector<unsigned char> rgb;gpu.readRgb(rgb);return rgb;};
    const auto quiet=capture();require(quiet==capture(),"not exact deterministic RGB");
    const auto resting=GreatWaveV1::pose(c,p);require(resting.height==p.baseHeight,"quiet wave absent");
    audio.bands[0]=audio.bands[1]=.8;
    double previous=resting.height,maxStep=0;
    for(int i=0;i<600;++i){c.t=(i+1)/60.;score.advance(audio,c.t,1./60);const auto s=GreatWaveV1::pose(c,p);
        maxStep=std::max(maxStep,std::abs(s.height-previous));previous=s.height;
        require(s.rise>=0 && s.rise<=p.maxRise && s.height>=p.baseHeight,"height exceeds standing bounds");}
    require(maxStep<6,"body jumps faster than an eased beat");
    const auto loud=GreatWaveV1::pose(c,p);require(loud.height>resting.height+100 && loud.curl>resting.curl,"low bands do not lift and curl");
    require(quiet!=capture(),"loud and silence render identically");
    for(int i=0;i<900;++i){audio={};c.t+=1./60;score.advance(audio,c.t,1./60);}
    require(GreatWaveV1::pose(c,p).rise<.01,"wave does not settle");
    score={};c.t=14;score.bassHits.push_back({14,1,1});const double start=GreatWaveV1::pose(c,p).flick;
    c.t=14.1;require(GreatWaveV1::pose(c,p).flick>start,"kick does not flick claws");
    const auto kick=capture();score={};require(kick!=capture(),"kick does not change visible foam");
    score.onsets.push_back({14,1,2});require(GreatWaveV1::pose(c,p).flick>0,"onset does not flick claws");
    score={};score.surges.push_back({14,1,3});c.t=14.8;require(GreatWaveV1::pose(c,p).rise>0,"swell does not lift wave");
    score={};c.t=14;p.seed++;require(quiet!=capture(),"seed ignored");
    p.anchorRight=true;p.x=1920;require(quiet!=capture(),"right anchor ignored");
    // Across eight hours and adversarial full-strength controls the only pose
    // remains standing. No release clock, plunge or crash state exists.
    for(int i=0;i<=28800;++i){c.t=i;for(auto& b:score.bandBody)b.fill(i%3?100:0);
        const auto s=GreatWaveV1::pose(c,p);require(std::isfinite(s.height) && s.height>=p.baseHeight && s.height<=p.baseHeight+p.maxRise,"reachable crash or unbounded rise");
        require(s.curl>=0 && s.curl<=p.curlAmount,"curl exceeds limit");
        const auto lip=GreatWaveV1::map({1000,560},s,p);require(std::isfinite(lip.x) && lip.y<p.y,"lip can plunge into sea");}
    // The surge fan must extend from the curling lip into visible sky, not
    // disappear above a full-height crown. Isolate its right-hand support.
    Schedule schedule(1);c.schedule=&schedule;c.t=20;schedule.print.surgeStart=17;
    p={};p.maxRise=0;p.kickGain=p.onsetGain=0;score={};
    Canvas body0,flow0,foam0;GreatWaveV1::paint(body0,flow0,foam0,c,p);const auto restBounds=foam0.bounds();
    p.surgeEnabled=true;Canvas body1,flow1,foam1;GreatWaveV1::paint(body1,flow1,foam1,c,p);const auto fanBounds=foam1.bounds();
    require(fanBounds[2]>restBounds[2]+40,"surge fan never leaves the visible curling lip");
    std::cout<<"PASS: exact RGB determinism, loud/silence, smooth attack/settle, max rise, kick/onset/swell, seed/side, eight hours without crash\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
