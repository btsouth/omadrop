#include "headless.h"
#include "kit/swell-lines.h"
#include <QCoreApplication>
#include <cstring>
#include <iostream>
#include <stdexcept>
using namespace Journey;using namespace Journey::Kit;
void require(bool ok,const char* text){if(!ok)throw std::runtime_error(text);}
int main(int argc,char** argv){
 QCoreApplication app(argc,argv);
 try {
    if(argc>1 && std::string(argv[1])=="--no-crossing") {
        Gpu gpu;Audio audio;audio.bands.fill(1);Score score;Schedule schedule(1);
        // Empty baseline gives maximum lift; all envelope bodies, kick and
        // scheduled surge are simultaneously at their upper bounds.
        for(auto& body:score.bandBody)body.fill(1);
        SwellLinesParametersV1 p;p.amplitude=1.5;p.amplitudeGain=4;
        p.bandGain=2;p.liftGain=1;p.kickGain=.5;p.surgeEnabled=true;
        Ctx c{gpu,0,audio,&score,1,nullptr,&schedule};
        double minGap=1e9,maxLift=0;std::size_t comparisons=0;
        for(int rows:{6,12,24})for(double falloff:{1.,1.45,3.})for(int seed:{0,71,211}) {
            p.rows=rows;p.depthFalloff=falloff;p.seed=seed;
            for(int tick=0;tick<=180;++tick) {
                c.t=tick;score.bassHits={{c.t,1,1}};
                schedule.print.surgeStart=c.t-5;
                require(c.kick(5)==1,"maximum kick missing");
                require(schedule.print.surge(c.t)==1,"maximum surge missing");
                maxLift=std::max(maxLift,c.lift(0));
                for(int row=0;row+1<rows;++row) {
                    const auto a=SwellLinesV1::field(c,row,p),b=SwellLinesV1::field(c,row+1,p);
                    // Envelope proof includes every possible printed-mark
                    // offset, not only these finite position samples.
                    require(a.base+a.displacementLimit<b.base-b.displacementLimit,"row envelopes cross");
                    for(int x=-30;x<=2000;x+=20) {
                        const double gap=b.y(x,-100)-a.y(x,100);
                        minGap=std::min(minGap,gap);require(gap>0,"rows cross at maximum controls");++comparisons;
                    }
                }
            }
        }
        require(maxLift==1.6,"maximum lift missing");
        // Soft response remains larger under loud drive instead of clipping.
        p=SwellLinesParametersV1{};c.t=14;score={};audio={};c.a=audio;
        const double quiet=SwellLinesV1::field(c,7,p).y(900);
        audio.bands.fill(1);c.a=audio;for(auto& body:score.bandBody)body.fill(1);
        require(quiet!=SwellLinesV1::field(c,7,p).y(900),"soft limit erases music response");
        std::cout<<"PASS: "<<comparisons<<" max-input row/mark comparisons; analytic envelopes never cross; min gap "<<minGap<<" px, band 1, lift "<<maxLift<<", kick 1, surge 1\n";
        return 0;
    }
    HeadlessContext context;QString error;require(context.create(error),"EGL failed");
    Gpu gpu;require(gpu.init(error),"GPU init failed");
    SwellLinesParametersV1 p;std::vector<std::unique_ptr<Canvas>> canvases;Score score;Audio audio;
    auto capture=[&](double t){
        gpu.begin(640,360);Ctx c{gpu,t,audio,&score,1,&canvases,nullptr};Canvas cv(gpu.pixelScale());
        SwellLinesV1::paint(cv,c,p);gpu.over(cv);
        FinishParams finish;finish.bloom=finish.vignette=finish.grain=0;gpu.finish(finish,nullptr);
        std::vector<unsigned char> rgb;gpu.readRgb(rgb);return rgb;
    };
    const auto quiet=capture(14);require(quiet==capture(14),"swell output not deterministic");
    require(quiet!=capture(194),"swell visibly repeats after three minutes");
    for(int b=0;b<6;++b){audio={};audio.bands[b]=.8;score={};for(auto& body:score.bandBody)body[b]=.8;
        require(quiet!=capture(14),"depth band did not change swell output");}
    audio={};score={};score.bassHits.push_back({14,1,1});require(quiet!=capture(14),"kick did not surge crests");
    score={};p.seed++;require(quiet!=capture(14),"seed did not change output");
    Ctx c{gpu,14,audio,&score,1,&canvases,nullptr};
    const auto field=SwellLinesV1::field(c,5,p);
    const auto area=SwellLinesV1::responsePath(c,5,p,12);
    require(area.contains(QPointF(900,field.y(900))),"response footprint misses its crest");
    require(!area.contains(QPointF(900,p.region.top())),"response footprint includes unrelated water");
    p.exclusions={p.region};require(SwellLinesV1::responsePath(c,5,p,12).isEmpty(),"response footprint ignores exclusions");
    const auto blank=capture(14);require(blank!=quiet,"exclusion did not hide swell");
    require(SwellLinesV1::band(0,12)==5 && SwellLinesV1::band(11,12)==0,"depth mapping reversed");
    std::cout<<"PASS: swell exact deterministic RGB, six depth bands, kick, seed, long clock and exclusions\n";
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
