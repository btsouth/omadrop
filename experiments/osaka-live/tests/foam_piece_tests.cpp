#include "headless.h"
#include "kit/foam-flecks.h"
#include <QCoreApplication>
#include <cstring>
#include <iostream>
#include <stdexcept>
using namespace Journey;using namespace Journey::Kit;
void require(bool ok,const char* text){if(!ok)throw std::runtime_error(text);}
int main(int argc,char** argv){
 QCoreApplication app(argc,argv);
 try {
    HeadlessContext context;QString error;require(context.create(error),"EGL failed");
    Gpu gpu;require(gpu.init(error),"GPU init failed");
    FoamFlecksParametersV1 p;std::vector<std::unique_ptr<Canvas>> canvases;Score score;Audio audio;
    auto capture=[&](double t){
        gpu.begin(640,360);Ctx c{gpu,t,audio,&score,1,&canvases,nullptr};Canvas cv(gpu.pixelScale());
        FoamFlecksV1::paint(cv,c,p);gpu.over(cv);
        FinishParams finish;finish.bloom=finish.vignette=finish.grain=0;gpu.finish(finish,nullptr);
        std::vector<unsigned char> rgb;gpu.readRgb(rgb);return rgb;
    };
    const auto quiet=capture(14);require(quiet==capture(14),"foam output not deterministic");
    require(quiet!=capture(194),"foam visibly repeats after three minutes");
    for(int b=0;b<6;++b){audio={};audio.bands[b]=.8;score={};for(auto& body:score.bandBody)body[b]=.8;
        require(quiet!=capture(14),"depth band did not change foam output");}
    audio={};score={};score.bassHits.push_back({14,1,1});require(quiet!=capture(14),"kick did not surge crests");
    score={};score.onsets.push_back({14,1,1});require(quiet!=capture(14),"onset did not brighten fragments");
    score={};p.swell.seed++;require(quiet!=capture(14),"seed did not change output");
    p.swell.exclusions={p.swell.region};const auto blank=capture(14);require(blank!=quiet,"exclusion did not hide foam");
    require(SwellLinesV1::band(0,12)==5 && SwellLinesV1::band(11,12)==0,"depth mapping reversed");
    std::cout<<"PASS: foam exact deterministic RGB, six depth bands, kick, onset, seed, long clock and exclusions\n";
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
