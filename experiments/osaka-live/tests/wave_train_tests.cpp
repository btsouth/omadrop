#include "kit/wave-train.h"
#include "audio.h"
#include <QFile>
#include <iostream>
#include <set>
using namespace Journey;using namespace Journey::Kit;
namespace {
void require(bool ok,const char* why) {if(!ok){std::cerr<<why<<'\n';std::exit(1);}}
double cross(V2 a,V2 b) {return a.x*b.y-a.y*b.x;}
bool intersects(V2 a,V2 b,V2 c,V2 d) {
    const double x=cross(b-a,c-a),y=cross(b-a,d-a),z=cross(d-c,a-c),w=cross(d-c,b-c);
    return ((x>1e-5 && y<-1e-5)||(x<-1e-5 && y>1e-5)) && ((z>1e-5 && w<-1e-5)||(z<-1e-5 && w>1e-5));
}
void simple(const WaveTrainProfileV2& f) {
    if(f.boundaries.size()!=1) {
        for(const auto& points:f.boundaries) {
            double area=0;for(size_t i=0;i<points.size();++i)area+=cross(points[i],points[(i+1)%points.size()]);
            std::cerr<<"boundary points="<<points.size()<<" area="<<area*.5<<" first="<<points.front().x<<","<<points.front().y<<'\n';
        }
        for(const auto& crest:f.crests)std::cerr<<"crest a="<<crest.a<<" Q="<<crest.q<<'\n';
    }
    require(f.boundaries.size()==1,"water silhouette is disconnected or has a closed hole");
    for(const auto& p:f.boundaries) {
        require(p.size()>=3,"degenerate boundary");
        for(size_t i=0;i<p.size();++i) {
            require(std::isfinite(p[i].x)&&std::isfinite(p[i].y),"nonfinite boundary");
            for(size_t j=i+2;j<p.size();++j)if(!(i==0&&j+1==p.size()))
                require(!intersects(p[i],p[(i+1)%p.size()],p[j],p[(j+1)%p.size()]),"self-intersection in drawn exterior");
        }
    }
}
bool same(const WaveTrainProfileV2& a,const WaveTrainProfileV2& b) {
    if(a.boundaries.size()!=b.boundaries.size())return false;
    for(size_t i=0;i<a.boundaries.size();++i) {
        if(a.boundaries[i].size()!=b.boundaries[i].size())return false;
        for(size_t j=0;j<a.boundaries[i].size();++j)if((a.boundaries[i][j]-b.boundaries[i][j]).len()!=0)return false;
    }return true;
}
}
int main(int argc,char** argv) {
    const std::string mode=argc>1?argv[1]:"profile";WaveTrainParametersV2 p;
    if(mode=="profile") {
        bool rawLoop=false;
        for(int t=0;t<30;++t)for(int j=0;j<12;++j) {
            WaveTrainPoseV2 s;s.q=lerp(.2,1.6,j/11.);s.amplitude=330;s.lipThrow=165;s.distance=t*88;
            auto f=WaveTrainV2::profile(s,p);simple(f);
            if(t==0 && j==11)for(size_t i=0;i+1<f.raw.size();++i)for(size_t k=i+2;k+1<f.raw.size();++k)
                rawLoop|=intersects(f.raw[i],f.raw[i+1],f.raw[k],f.raw[k+1]);
        }
        require(rawLoop,"sweep did not exercise an actual Gerstner loop");
    }else if(mode=="deterministic") {
        WaveTrainMotionV2 a,b;Score score;Audio audio;audio.bands.fill(.2);audio.bassLevel=.3;
        for(int i=0;i<3600;++i){const double t=(i+1)/60.;score.advance(audio,t,1/60.);a.advance(audio,score,t,1/60.);b.advance(audio,score,t,1/60.);}
        require(same(WaveTrainV2::profile(a.pose(),p),WaveTrainV2::profile(b.pose(),p)),"non-deterministic profile");
        CriticalSpringV2 x,y;x.advance(1,2,1);for(int i=0;i<60;++i)y.advance(1,2,1/60.);
        require(std::abs(x.value-y.value)<1e-12,"spring depends on chunk size");
    }else if(mode=="no-crash") {
        bool curled=false,settled=false;
        for(int i=0;i<=600;++i) {
            WaveTrainPoseV2 s;s.q=1.6;s.amplitude=440;s.lipThrow=165;s.distance=i*8.8;
            auto f=WaveTrainV2::profile(s,p);
            for(const auto& crest:f.crests)if(!crest.outerLip.empty()) {
                curled=true;for(auto v:crest.outerLip)require(v.y<p.waterline-s.amplitude*crest.envelope*.30,"lip plunges to sea");
                if(crest.q>1.3)require(crest.outerLip.back().x<crest.outerLip[crest.outerLip.size()-2].x,"strong throw straightens the curling tip");
            }
            if(i>200 && f.crests.size() && f.crests[0].envelope<.15)settled=true;
        }
        WaveTrainPoseV2 born;born.q=1.6;
        require(WaveTrainV2::envelope(380,born,p)>.9,"crest did not enter group peak");
        born.distance=1800;
        require(WaveTrainV2::envelope(380+born.distance,born,p)<.15,"same travelling crest did not fade out of group");
        require(curled&&settled,"no curl or envelope decay exercised");
    }else if(mode=="continuous"||mode=="fixture") {
        QFile fixture;if(mode=="fixture") {
            require(argc==3,"fixture path missing");fixture.setFileName(argv[2]);require(fixture.open(QIODevice::ReadOnly),"cannot open fixture");
            require(fixture.size()>=60ll*44100*8,"short fixture");
        }
        StreamingAudio analyzer;WaveTrainMotionV2 motion;Score score;Audio audio;
        std::set<int> qs,amps;std::set<std::pair<int,int>> shapes;int curls=0;double maxStep=0,oldQ=.2;
        for(int i=0;i<3600;++i) {
            const double now=(i+1)/60.;
            auto consume=[&](const Audio& a,double dt){audio=a;score.advance(a,now,dt);motion.advance(a,score,now,dt);};
            if(mode=="fixture") {
                auto pcm=fixture.read(735*8);require(pcm.size()==735*8,"short hop");analyzer.push(reinterpret_cast<const float*>(pcm.constData()),735,consume);
            }else {
                Audio a;const double e=.5+.48*std::sin(now*.28);a.bands.fill(.40*e);a.bassLevel=.6*e;a.surge=.4*e;consume(a,1/60.);
            }
            const auto& s=motion.pose();maxStep=std::max(maxStep,std::abs(s.q-oldQ));oldQ=s.q;
            qs.insert(int(s.q*100));amps.insert(int(s.amplitude));
            if(mode=="fixture" ? i%2==1 : i%15==0) {
                auto f=WaveTrainV2::profile(s,p);simple(f);
                for(const auto& crest:f.crests)if(!crest.outerLip.empty())++curls;
                // A ratio of vertical displacement and face slope changes even
                // after amplitude normalization, defeating two scaled poses.
                const auto& v=f.surface[f.surface.size()/3];const auto& n=f.surface[f.surface.size()/3+2];
                shapes.insert({int(100*(p.waterline-v.y)/s.amplitude),int(100*(n.y-v.y)/std::max(.01,n.x-v.x))});
            }
        }
        std::cout<<"Q bins="<<qs.size()<<" amplitude bins="<<amps.size()<<" normalized shapes="<<shapes.size()<<" curl samples="<<curls<<" max Q hop="<<maxStep<<'\n';
        require(qs.size()>70&&amps.size()>100&&shapes.size()>180,"response collapses into too few states");
        require(curls>0,"no full curls over fixture");require(maxStep<.04,"Q pops between hops");
    }else return 2;
    std::cout<<"PASS "<<mode<<'\n';
}
