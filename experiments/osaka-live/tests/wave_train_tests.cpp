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
V2 at(const std::vector<V2>& a,double index) {
    int i=std::min(int(index),int(a.size()-2));return lerp(a[i],a[i+1],index-i);
}
bool inside(V2 q,const std::vector<V2>& boundary) {
    bool result=false;for(size_t i=0,j=boundary.size()-1;i<boundary.size();j=i++) {
        V2 a=boundary[i],b=boundary[j];
        if((a.y>q.y)!=(b.y>q.y) && q.x<(b.x-a.x)*(q.y-a.y)/(b.y-a.y)+a.x)result=!result;
    }return result;
}
std::vector<V2> polygon(const std::vector<V2>& a,const std::vector<V2>& b) {
    auto v=a;v.insert(v.end(),b.rbegin(),b.rend());return v;
}
void fingerSimple(const std::vector<V2>& v) {
    for(size_t i=0;i<v.size();++i)for(size_t j=i+2;j<v.size();++j)if(!(i==0&&j+1==v.size()))
        if(intersects(v[i],v[(i+1)%v.size()],v[j],v[(j+1)%v.size()])) {std::cerr<<" polygon crossing segments="<<i<<","<<j<<'\n';require(false,"finger self-intersection");}
}
void fingerSafe(const WaveTrainProfileV2::Crest& hero) {
    require(hero.fingers.size()==WaveTrainFingerCountV2,"finger count changed");
    for(const auto& f:hero.fingers) {
        require((f.root-at(hero.outerLip,f.lipIndex)).len()<1e-9,"finger detached from lip");
        require(f.length>=0 && f.length<160,"finger exceeds safe length");
        if(hero.stage<=1)require(f.length==0,"stub at S0-S1");
        for(auto q:f.centre) {
            require(std::isfinite(q.x)&&std::isfinite(q.y),"nonfinite finger");
            require((q-f.root).len()<165,"finger escaped attachment neighbourhood");
        }
        if(f.length>1) {
            fingerSimple(polygon(f.left,f.right));
            if(!f.forkLeft.empty())fingerSimple(polygon(f.forkLeft,f.forkRight));
            for(const auto* edge:{&f.left,&f.right,&f.forkLeft,&f.forkRight})for(size_t i=0;i<edge->size();++i) {
                const V2 q=(*edge)[i];require(std::isfinite(q.x)&&std::isfinite(q.y)&&(q-f.root).len()<165,"finger edge outside safe bounds");
                if(i>=2 && inside(q,hero.boundary))require(false,"finger edge pokes through body");
            }
            for(size_t i=2;i<f.centre.size();++i) {
                if(inside(f.centre[i],hero.boundary)){std::cerr<<"body penetration stage="<<hero.stage<<" index="<<f.lipIndex<<" sample="<<i<<'\n';require(false,"claw pokes through body");}
            }
        }
    }
}
bool sameDetails(const WaveTrainPoseV2& a,const WaveTrainPoseV2& b) {
    for(size_t i=0;i<a.droplets.size();++i) {
        const auto& x=a.droplets[i];const auto& y=b.droplets[i];
        if((x.position-y.position).len()!=0||(x.velocity-y.velocity).len()!=0||x.age!=y.age||x.serial!=y.serial||x.life!=y.life)return false;
    }return true;
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
    if(mode=="fingers" || mode=="crossing" || mode=="detail-continuity") {
        double maxStep=0,maxCross=0;std::vector<V2> oldTips;
        for(int i=0;i<=1000;++i) {
            WaveTrainPoseV2 s;s.stage=i*.005;s.seconds=3.5;s.energy=.8;s.lipThrow=62;s.lean=.08;
            s.bands.fill(.6);for(int k=0;k<WaveTrainFingerCountV2;++k)
                s.fingers[k]=mode=="crossing"?WaveTrainFingerStateV2{k%2?.95:.15,k%2?.35:-.35}:WaveTrainFingerStateV2{.9,.30};
            auto shape=WaveTrainV2::profile(s,p);const auto& h=shape.crests[shape.hero];fingerSafe(h);
            std::vector<V2> tips;for(const auto& f:h.fingers)tips.push_back(f.tip);
            if(!oldTips.empty())for(size_t j=0;j<tips.size();++j)maxStep=std::max(maxStep,(tips[j]-oldTips[j]).len());oldTips=tips;
            if(mode=="crossing" && i%5==0) {
                for(size_t j=0;j<h.fingers.size();++j)for(size_t k=j+1;k<h.fingers.size();++k) {
                    const auto& a=h.fingers[j];const auto& b=h.fingers[k];if(a.length<1||b.length<1)continue;
                    const std::array<std::vector<V2>,2> as{polygon(a.left,a.right),polygon(a.forkLeft,a.forkRight)},bs{polygon(b.left,b.right),polygon(b.forkLeft,b.forkRight)};
                    for(const auto& pa:as)for(const auto& pb:bs)
                    for(size_t x=1;x+1<pa.size();++x)for(size_t y=1;y+1<pb.size();++y)if(intersects(pa[x],pa[x+1],pb[y],pb[y+1])) {
                        const double overlap=std::min((pa[x]-a.root).len(),(pb[y]-b.root).len());maxCross=std::max(maxCross,overlap);
                        if(overlap>3){std::cerr<<"stage="<<s.stage<<" crossing="<<j<<","<<k<<" overlap="<<overlap<<'\n';require(false,"fingers cross beyond 3px root tolerance");}
                    }
                }
            }
        }
        require(maxStep<3,"finger tips pop across dense stage samples");
        if(mode=="detail-continuity") {
            WaveTrainPoseV2 s;s.stage=5;WaveTrainFoamMotionV2 motion;Score sc;Audio a;std::vector<V2> previous;double frameStep=0,contourStep=0;WaveTrainProfileV2::Crest oldCrest;
            for(int i=0;i<1800;++i) {
                const double now=(i+1)/60.;s.seconds=now;s.energy=.7;s.flow=.21*now;for(int b=0;b<6;++b)a.bands[b]=.22+.18*std::sin(now*(.3+b*.13)+b);
                if(i%40==0)a.bands[5]=.9;
                motion.advance(a,sc,s,now,1/60.);auto shape=WaveTrainV2::profile(s,p);const auto& h=shape.crests[shape.hero];fingerSafe(h);
                std::vector<V2> current;for(const auto& f:h.fingers)current.push_back(f.tip);
                if(!previous.empty())for(size_t j=0;j<current.size();++j)frameStep=std::max(frameStep,(current[j]-previous[j]).len());previous=current;
                if(!oldCrest.contours[0].empty())for(int row=0;row<16;++row)for(size_t k=0;k<h.contours[row].size();++k)
                    contourStep=std::max(contourStep,(h.contours[row][k]-oldCrest.contours[row][k]).len()*std::max(h.contourAlpha[row],oldCrest.contourAlpha[row]));
                oldCrest=h;
            }
            require(frameStep<6,"finger frame movement pops at 60Hz");require(contourStep<6,"visible contour movement pops at phase wrap");
            std::cout<<"max visible contour frame step px="<<contourStep<<'\n';std::cout<<"max finger frame step px="<<frameStep<<'\n';
        }
        std::cout<<"max dense stage tip step px="<<maxStep<<" max crossing depth px="<<maxCross<<'\n';
    }else if(mode=="bands") {
        std::array<double,6> previous{};
        for(double level:{0.,.02,.08,.18,.35,.65,2.}) {
            WaveTrainPoseV2 s;s.stage=5;WaveTrainFoamMotionV2 motion;Score sc;Audio a;a.bands.fill(level);
            for(int i=0;i<900;++i)motion.advance(a,sc,s,(i+1)/60.,1/60.);
            auto shape=WaveTrainV2::profile(s,p);const auto& h=shape.crests[shape.hero];std::array<double,6> lengths{};std::array<int,6> counts{};
            for(const auto& f:h.fingers){lengths[f.band]+=f.length;++counts[f.band];}
            for(int band=0;band<6;++band){lengths[band]/=counts[band];require(lengths[band]>=previous[band]-.05,"finger response is not monotonic by band");}
            if(level==.65)for(int b=0;b<6;++b)require(lengths[b]>previous[b]+.1,"band response is saturated too early");
            previous=lengths;
            std::cout<<"level="<<level<<" band means=";for(double x:lengths)std::cout<<x<<',';std::cout<<'\n';
        }
        // One band's onset must not flick another frequency's fingers.
        WaveTrainPoseV2 s;s.stage=5;WaveTrainFoamMotionV2 motion;Score sc;Audio a;
        a.bands[5]=.6;for(int i=0;i<12;++i)motion.advance(a,sc,s,(i+1)/60.,1/60.);
        double flick=0;for(int i=0;i<WaveTrainFingerCountV2;++i) {
            int band=std::min(5,int(6*(i+.5+.18*std::sin(i*2.41))/WaveTrainFingerCountV2));
            if(band==5)flick=std::max(flick,s.fingers[i].flick);else require(s.fingers[i].flick==0,"onset flick leaked to other bands");
        }require(flick>.02,"band onset produced no flick");
    }else if(mode=="spray") {
        WaveTrainPoseV2 a,b;a.stage=b.stage=5;WaveTrainFoamMotionV2 x,y;Score sc;Audio input;int maximum=0;unsigned total=0;
        for(int i=0;i<3600;++i) {
            const double now=(i+1)/60.;a.seconds=b.seconds=now;
            input.bands.fill(i%7==0?10000:.01);if(i%7==0)sc.bassHits.push_back({now,1,unsigned(i+1)});
            while(sc.bassHits.size()>64)sc.bassHits.pop_front();
            x.advance(input,sc,a,now,1/60.);y.advance(input,sc,b,now,1/60.);
            require(sameDetails(a,b),"spray is not seeded deterministically");int alive=0;
            for(const auto& d:a.droplets)if(d.life>0&&d.age<d.life) {
                ++alive;total=std::max(total,d.serial);require(d.life<=2.7&&std::isfinite(d.position.x)&&std::isfinite(d.position.y),"invalid ballistic particle");
            }
            maximum=std::max(maximum,alive);require(alive<=WaveTrainDropletLimitV2,"pool exceeds bound");
        }
        require(maximum==WaveTrainDropletLimitV2&&total>1000,"spray stress did not exercise reuse");
        input={};for(int i=0;i<240;++i)x.advance(input,sc,a,60+(i+1)/60.,1/60.);
        for(const auto& d:a.droplets)require(d.age>=d.life,"particles did not expire in silence");
        std::cout<<"pool limit="<<WaveTrainDropletLimitV2<<" max alive="<<maximum<<" emitted="<<total<<'\n';
    }else if(mode=="profile") {
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
                fingerSafe(crest);
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
