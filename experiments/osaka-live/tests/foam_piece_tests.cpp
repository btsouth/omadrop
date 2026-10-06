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
    if(argc>1 && std::string(argv[1])=="--live-mask") {
        Gpu gpu;Audio audio;Score score;Schedule schedule(1);StaticGeometry geometry;
        FoamFlecksParametersV1 p;p.masksWaveTrain=true;p.count=240;
        p.waveTrain.groupPeriod=1800;p.waveTrain.groupWidth=1500;p.waveTrain.groupFloor=.94;
        p.waveTrain.heightScale=.88;p.waveTrain.waterline=965;p.waveTrain.travelScale=.2;
        p.swell.amplitude=.52;p.swell.amplitudeGain=3.5;p.swell.bandGain=1.15;
        p.swell.liftGain=.5;p.swell.kickGain=.35;p.swell.surgeEnabled=true;p.responseGain=1.7;
        Ctx c{gpu,0,audio,&score,1,nullptr,&schedule,&geometry};std::size_t triangles=0;
        for(int i=1;i<=5400;++i) {
            c.t=i/60.;audio.bands.fill(i<1200?.01:.8);audio.bassLevel=audio.bands[0];c.a=audio;
            score.advance(audio,c.t,1/60.);schedule.waveTrain.advance(audio,score,c.t,1/60.,p.waveTrain);
            if(i%900)continue;
            const auto mask=FoamFlecksV1::exclusionPath(c,p);
            const auto& field=WaveTrainV2::worldProfile(c,p.waveTrain);
            for(const auto& crest:field.crests)for(auto q:crest.boundary)
                require(mask.intersects(QRectF(q.x-.1,q.y-.1,.2,.2)),"mask misses live body/barrel envelope");
            Canvas cv;FoamFlecksV1::paint(cv,c,p);
            require(!cv.vertices().empty(),"live mask removes all sea foam");
            const auto& v=cv.vertices();
            for(const auto& cmd:cv.commands())for(int k=cmd.first;k+2<cmd.first+cmd.count;k+=3) {
                QPainterPath triangle;triangle.moveTo(v[k].x,v[k].y);triangle.lineTo(v[k+1].x,v[k+1].y);
                triangle.lineTo(v[k+2].x,v[k+2].y);triangle.closeSubpath();
                require(!mask.intersects(triangle),"sea fleck enters live hero body or barrel");++triangles;
            }
        }
        std::cout<<"PASS: live 90 s moving hero/body/barrel mask, "<<triangles<<" retained foam triangles outside envelope\n";
        return 0;
    }
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
    score={};score.onsets.push_back({14,1,1});require(quiet==capture(14),"onset pops at discovery");
    const auto onset=capture(14.18);score={};require(onset!=capture(14.18),"eased onset did not brighten depth rows");
    score={};p.swell.seed++;require(quiet!=capture(14),"seed did not change output");
    p.swell.exclusions={p.swell.region};const auto blank=capture(14);require(blank!=quiet,"exclusion did not hide foam");
    require(SwellLinesV1::band(0,12)==5 && SwellLinesV1::band(11,12)==0,"depth mapping reversed");
    std::cout<<"PASS: foam exact deterministic RGB, six depth bands, kick, onset, seed, long clock and exclusions\n";
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
