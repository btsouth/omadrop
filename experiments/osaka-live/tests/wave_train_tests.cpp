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
    require(!f.boundaries.empty(),"missing travelling wave");
    for(const auto& p:f.boundaries) {
        require(p.size()==112,"fixed sample count changed");
        for(size_t i=0;i<p.size();++i) {
            require(std::isfinite(p[i].x)&&std::isfinite(p[i].y),"nonfinite boundary");
            for(size_t j=i+2;j<p.size();++j)if(!(i==0&&j+1==p.size())) {
                if(intersects(p[i],p[(i+1)%p.size()],p[j],p[(j+1)%p.size()])) {
                    std::cerr<<"crossing "<<i<<","<<j<<" stage="<<f.crests[&p-&f.boundaries[0]].stage<<'\n';
                    require(false,"self-intersection in drawn hero");
                }
            }
        }
    }
}
bool same(const WaveTrainProfileV2& a,const WaveTrainProfileV2& b) {
    if(a.boundaries.size()!=b.boundaries.size())return false;
    for(size_t i=0;i<a.boundaries.size();++i)for(size_t j=0;j<a.boundaries[i].size();++j)
        if((a.boundaries[i][j]-b.boundaries[i][j]).len()!=0)return false;
    return true;
}
}
int main(int argc,char** argv) {
    const std::string mode=argc>1?argv[1]:"profile";WaveTrainParametersV2 p;
    if(mode=="profile") {
        double maxStep=0;std::vector<V2> previous;
        for(int j=0;j<=1000;++j) {
            WaveTrainPoseV2 s;s.stage=j*.005;
            auto f=WaveTrainV2::profile(s,p);simple(f);
            const auto& v=f.crests[f.hero].boundary;
            if(!previous.empty())for(size_t k=0;k<v.size();++k)maxStep=std::max(maxStep,(v[k]-previous[k]).len());
            previous=v;
        }
        require(maxStep<2.5,"stage interpolation jumps over 2.5 pixels per .005 stage");
        for(int j=0;j<=100;++j)for(double lag:{-.32,.32})for(double lean:{0.,.08}) {
            WaveTrainPoseV2 s;s.stage=j*.05;s.lipStage=s.stage+lag;s.lipThrow=62;s.lean=lean;s.seconds=j*.37;
            simple(WaveTrainV2::profile(s,p));
        }
        WaveTrainPoseV2 s;s.stage=5;auto f=WaveTrainV2::profile(s,p);const auto& v=f.crests[f.hero].boundary;
        double top=1080,left=3000,right=-3000;
        for(auto q:v){top=std::min(top,q.y);left=std::min(left,q.x);right=std::max(right,q.x);}
        require((1080-top)/1080>.70&&(1080-top)/1080<.75,"S5 height does not match requested frame proportion");
        require((right-left)/1920>.54&&(right-left)/1920<.58,"S5 base does not match requested frame proportion");
        std::cout<<"max stage step px="<<maxStep<<" S5 height fraction="<<(1080-top)/1080<<" width fraction="<<(right-left)/1920<<'\n';
    }else if(mode=="deterministic") {
        WaveTrainMotionV2 a,b;Score score;Audio audio;audio.bands.fill(.2);audio.bassLevel=.3;
        for(int i=0;i<3600;++i){const double t=(i+1)/60.;score.advance(audio,t,1/60.);a.advance(audio,score,t,1/60.);b.advance(audio,score,t,1/60.);}
        require(same(WaveTrainV2::profile(a.pose(),p),WaveTrainV2::profile(b.pose(),p)),"non-deterministic profile");
        for(double level:{.12,.22,.50}) {
            WaveTrainMotionV2 m;Score sc;Audio input;input.bands.fill(level);input.bassLevel=level;
            for(int i=0;i<600;++i){double t=(i+1)/60.;sc.advance(input,t,1/60.);m.advance(input,sc,t,1/60.);}
            const double stage=m.pose().stage;
            if(level==.12)require(stage<=1,"quiet music leaves S0-S1");
            if(level==.22)require(stage>=2&&stage<=4,"building music misses the pitching continuum");
            if(level==.50)require(stage>4.8,"loud music does not reach full curl");
        }
        CriticalSpringV2 x,y;x.advance(1,2,1);for(int i=0;i<60;++i)y.advance(1,2,1/60.);
        require(std::abs(x.value-y.value)<1e-12,"spring depends on chunk size");
    }else if(mode=="no-crash") {
        bool curled=false,settled=false;
        for(int i=0;i<=600;++i) {
            WaveTrainPoseV2 s;s.stage=5;s.amplitude=850;s.lipThrow=62;s.lean=.08;s.distance=i*8.8;s.seconds=i*.1;
            auto f=WaveTrainV2::profile(s,p);simple(f);
            for(const auto& crest:f.crests) {
                if(crest.stage>4.5) {
                    curled=true;require(crest.outerLip.back().y<880,"full curl plunges into foreground sea");
                    require(crest.outerLip.back().x<crest.outerLip[crest.outerLip.size()-2].x,"lip does not turn inward");
                }
                if(crest.envelope<.15){settled=true;require(crest.stage<.1,"departing wave retains a breaking lip");}
            }
        }
        require(curled&&settled,"no curl or group decay exercised");
    }else if(mode=="continuous"||mode=="fixture") {
        QFile fixture;if(mode=="fixture") {
            require(argc==3,"fixture path missing");fixture.setFileName(argv[2]);require(fixture.open(QIODevice::ReadOnly),"cannot open fixture");
            require(fixture.size()>=60ll*44100*8,"short fixture");
        }
        StreamingAudio analyzer;WaveTrainMotionV2 motion;Score score;Audio audio;
        std::set<int> stages,heights;int curls=0;double maxStep=0,maxGeometryStep=0,old=0;
        std::vector<V2> previous;double previousA=-1;
        for(int i=0;i<3600;++i) {
            const double now=(i+1)/60.;
            auto consume=[&](const Audio& a,double dt){audio=a;score.advance(a,now,dt);motion.advance(a,score,now,dt);};
            if(mode=="fixture") {
                auto pcm=fixture.read(735*8);require(pcm.size()==735*8,"short hop");analyzer.push(reinterpret_cast<const float*>(pcm.constData()),735,consume);
            }else {
                Audio a;const double e=.5+.48*std::sin(now*.28);a.bands.fill(.40*e);a.bassLevel=.6*e;a.surge=.4*e;consume(a,1/60.);
            }
            const auto& s=motion.pose();maxStep=std::max(maxStep,std::abs(s.stage-old));old=s.stage;
            stages.insert(int(s.stage*100));heights.insert(int(s.amplitude));
            if(i%2==1) {
                auto f=WaveTrainV2::profile(s,p);simple(f);
                const auto& hero=f.crests[f.hero];if(hero.stage>4.8)++curls;
                if(!previous.empty()&&std::abs(hero.a-previousA)<10)
                    for(size_t k=0;k<hero.boundary.size();++k)maxGeometryStep=std::max(maxGeometryStep,(hero.boundary[k]-previous[k]).len());
                previous=hero.boundary;previousA=hero.a;
            }
        }
        std::cout<<"stage bins="<<stages.size()<<" height bins="<<heights.size()<<" curls="<<curls<<" max stage hop="<<maxStep<<" max geometry frame px="<<maxGeometryStep<<'\n';
        require(stages.size()>180&&heights.size()>100,"too few musical states");
        require(curls>0,"no full curls over fixture");require(maxStep<.15,"stage pops between hops");
        require(maxGeometryStep<25,"geometry jumps between adjacent music frames");
    }else return 2;
    std::cout<<"PASS "<<mode<<'\n';
}
