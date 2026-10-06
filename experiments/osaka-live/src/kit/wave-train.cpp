#include "wave-train.h"
namespace Journey::Kit {
void CriticalSpringV2::advance(double target,double omega,double dt) {
    const double error=value-target,b=velocity+omega*error,e=std::exp(-omega*dt);
    value=target+(error+b*dt)*e;velocity=(velocity-omega*b*dt)*e;
}
WaveTrainMotionV2::WaveTrainMotionV2() {
    amplitude_.value=pose_.amplitude;speed_.value=pose_.phaseSpeed;
}
void WaveTrainMotionV2::advance(const Audio& a,const Score& score,double seconds,double dt) {
    if(!std::isfinite(dt)||dt<=0||dt>.25||!std::isfinite(seconds))return;
    double power=0;for(double b:a.bands)power+=b*b;
    const double energy=clamp01(3.1*std::sqrt(power/6)+.35*clamp01(a.surge));
    std::vector<double> intervals;int count=0;double previous=-1;
    for(const auto& e:score.onsets)if(e.t<=seconds && e.t>seconds-8) {
        ++count;if(previous>=0 && e.t-previous>.12)intervals.push_back(e.t-previous);previous=e.t;
    }
    double tempo=90;
    if(!intervals.empty()) {
        std::sort(intervals.begin(),intervals.end());tempo=60/intervals[intervals.size()/2];
        while(tempo<60)tempo*=2;while(tempo>180)tempo*=.5;
    }
    const double beat=tempo/60,oldSpeed=speed_.value;
    amplitude_.advance(590+240*std::max(0.,a.bassLevel)/(.20+std::max(0.,a.bassLevel)),1.05*beat,dt);
    stage_.advance(5*sstep(.25,.95,energy),.8*beat,dt);
    speed_.advance(14+6*beat+4*clamp01(count/16.),.65*beat,dt);
    const double mid=std::max(0.,(a.bands[2]+a.bands[3])*.5);
    throw_.advance(62*mid/(.18+mid),2.25*beat,dt);
    lean_.advance(.075*mid/(.18+mid),1.6*beat,dt);
    // Exact underdamped spring at analyzer hops. Lip lag and overshoot are
    // separate from body stage, with a bounded material displacement.
    const double omega=3.4*beat,zeta=.64,w=omega*std::sqrt(1-zeta*zeta);
    const double x=lip_-stage_.value,b=(lipVelocity_+zeta*omega*x)/w;
    const double e=std::exp(-zeta*omega*dt),cs=std::cos(w*dt),sn=std::sin(w*dt);
    lip_=stage_.value+e*(x*cs+b*sn);
    lipVelocity_=e*((-zeta*omega*x+w*b)*cs+(-zeta*omega*b-w*x)*sn);
    pose_.amplitude=amplitude_.value;pose_.stage=stage_.value;pose_.phaseSpeed=speed_.value;
    pose_.lean=lean_.value;pose_.lipThrow=throw_.value;pose_.lipStage=lip_;
    pose_.seconds=seconds;pose_.energy=energy;pose_.tempo=tempo;
    pose_.distance+=(oldSpeed+speed_.value)*.5*dt;pose_.flow+=(.065+.16*energy)*dt;
}
namespace {
constexpr int Segments=7,Steps=16,Count=Segments*Steps;
using Controls=std::array<V2,22>;
using Samples=std::array<V2,Count+1>;
V2 bezier(V2 a,V2 b,V2 c,V2 d,double t) {
    const double v=1-t;return a*(v*v*v)+b*(3*v*v*t)+c*(3*v*t*t)+d*(t*t*t);
}
V2 cat(V2 a,V2 b,V2 c,V2 d,double t) {
    return b+(c-a)*(.5*t)+(a*2-b*5+c*4-d)*(.5*t*t)+(b*3-a-c*3+d)*(.5*t*t*t);
}
// S0 swell; S1 building; S2 vertical face; S3 pitching; S4 hollow; S5 curl.
const std::array<Controls,6>& stages() {
    static const std::array<Controls,6> profiles=[] {
        std::array<Controls,6> p{{
            {{{0,0},{.02,.05},{.13,.14},{.28,.14},{.36,.14},{.43,.13},{.49,.11},
              {.52,.10},{.55,.085},{.57,.075},{.58,.07},{.595,.065},{.61,.06},
              {.64,.052},{.67,.043},{.72,.03},{.80,.015},{.91,.005},{1,0},
              {.74,-.035},{.22,-.04},{0,0}}},
            {{{0,0},{.04,.08},{.18,.33},{.35,.36},{.44,.38},{.55,.35},{.61,.29},
              {.64,.27},{.665,.235},{.66,.21},{.675,.19},{.69,.18},{.705,.15},
              {.72,.115},{.75,.075},{.79,.055},{.83,.025},{.94,.02},{1,0},
              {.74,-.035},{.22,-.04},{0,0}}},
            {{{0,0},{.04,.14},{.19,.56},{.40,.61},{.49,.63},{.59,.60},{.64,.53},
              {.67,.49},{.685,.445},{.665,.405},{.67,.36},{.685,.31},{.69,.26},
              {.69,.17},{.70,.12},{.76,.08},{.84,.015},{.94,.025},{1,0},
              {.74,-.035},{.22,-.04},{0,0}}},
            {{{0,0},{-.005,.24},{.13,.69},{.35,.80},{.48,.88},{.65,.81},{.73,.69},
              {.79,.61},{.80,.51},{.745,.455},{.70,.46},{.69,.595},{.635,.535},
              {.56,.44},{.595,.24},{.73,.12},{.84,.02},{.94,.04},{1,0},
              {.74,-.035},{.22,-.04},{0,0}}},
            {{{0,0},{-.025,.32},{.08,.79},{.28,.91},{.44,1.025},{.64,.94},{.74,.78},
              {.835,.64},{.835,.535},{.752,.49},{.74,.52},{.727,.67},{.635,.64},
              {.52,.61},{.52,.40},{.67,.235},{.80,.095},{.88,.135},{1,.04},
              {.74,-.06},{.22,-.065},{0,0}}},
            Controls{}
        }};
        // Journey ending_water.cpp::studyWave, exact cubic water controls.
        // Normalize the 1620 x 1040 authored design before independent scales.
        const Controls source{{{-180,1100},{-240,720},{-100,275},{220,130},
            {475,-25},{825,65},{990,245},{1145,395},{1120,515},{1000,560},
            {1090,465},{985,360},{850,390},{660,410},{655,590},{850,735},
            {1040,880},{1230,875},{1440,1040},{1040,1170},{250,1190},{-180,1100}}};
        for(int i=0;i<22;++i)p[5][i]={(source[i].x+180)/1620,(1100-source[i].y)/1040};
        return p;
    }();return profiles;
}
const std::array<Samples,6>& sampledStages() {
    static const std::array<Samples,6> data=[] {
        std::array<Samples,6> v{};
        for(int s=0;s<6;++s)for(int seg=0;seg<Segments;++seg)for(int j=0;j<Steps;++j) {
            const auto& c=stages()[s];int k=seg*3;
            v[s][seg*Steps+j]=bezier(c[k],c[k+1],c[k+2],c[k+3],j/double(Steps));
        }
        for(int s=0;s<6;++s)v[s][Count]=v[s][0];return v;
    }();return data;
}
Samples blend(double stage) {
    stage=std::clamp(stage,0.,5.);const int i=std::min(4,int(stage));const double t=stage-i;
    const auto& d=sampledStages();Samples v{};
    for(int j=0;j<=Count;++j)v[j]=cat(d[std::max(0,i-1)][j],d[i][j],d[i+1][j],d[std::min(5,i+2)][j],t);
    return v;
}
V2 unit(V2 p) {return p*(1/std::max(.00001,p.len()));}
void fill(Canvas& cv,const std::vector<V2>& v) {
    if(v.empty())return;cv.moveTo(v[0].x,v[0].y);
    for(size_t i=1;i<v.size();++i)cv.lineTo(v[i].x,v[i].y);cv.closePath();cv.fill();
}
// Fixed connected quads avoid costly round joins; connectivity is reusable.
void ribbon(Canvas& cv,const std::vector<V2>& a,const std::vector<V2>& b,Col col,double alpha=1) {
    for(size_t j=0;j+1<a.size();++j) {
        cv.tri(a[j],a[j+1],b[j+1],col,alpha);
        cv.tri(a[j],b[j+1],b[j],col,alpha);
    }
}
void stripe(Canvas& cv,const std::vector<V2>& a,double width,Col col,double alpha) {
    std::vector<V2> l,r;l.reserve(a.size());r.reserve(a.size());
    for(size_t j=0;j<a.size();++j) {
        V2 t=a[std::min(j+1,a.size()-1)]-a[j?j-1:0],n=unit({-t.y,t.x})*(width*.5);
        l.push_back(a[j]+n);r.push_back(a[j]-n);
    }ribbon(cv,l,r,col,alpha);
}
V2 sea(double a,const WaveTrainPoseV2& s,const WaveTrainParametersV2& p) {
    V2 v{a,p.waterline};const double k=Tau/p.wavelength,A=28+48*s.energy;
    const double ratio[]={1,2.37,3.91,6.13},weight[]={1,.25,.12,.06};
    for(int j=0;j<4;++j) {
        const double phase=ratio[j]*k*(a-380)-std::sqrt(ratio[j])*k*s.distance+j*1.31;
        v.x-=.18*weight[j]/(k*ratio[j])*std::sin(phase);
        v.y-=A*weight[j]*std::cos(phase);
    }return v;
}
}
double WaveTrainV2::envelope(double a,const WaveTrainPoseV2& s,const WaveTrainParametersV2& p) {
    const double d=wrap(a-p.groupOrigin-.5*s.distance+p.groupPeriod*.5,p.groupPeriod)-p.groupPeriod*.5;
    double group=0,peak=0;for(int j=-2;j<=2;++j) {
        group+=std::exp(-.5*std::pow((d+j*p.groupPeriod)/p.groupWidth,2));
        peak+=std::exp(-.5*std::pow(j*p.groupPeriod/p.groupWidth,2));
    }
    return (.08+.92*group/peak)*(1-.62*sstep(1400,2200,a));
}
WaveTrainProfileV2 WaveTrainV2::profile(const WaveTrainPoseV2& s,const WaveTrainParametersV2& p) {
    WaveTrainProfileV2 out;out.surface.reserve(161);
    for(int j=0;j<=160;++j)out.surface.push_back(sea(lerp(p.x0,p.x1,j/160.),s,p));
    const int first=int(std::floor((p.x0-s.distance-380)/p.groupPeriod))-1;
    const int last=int(std::ceil((p.x1-s.distance-380)/p.groupPeriod))+1;
    double best=-1;
    for(int n=first;n<=last;++n) {
        const double a=380+s.distance+n*p.groupPeriod,g=envelope(a,s,p);
        const double left=a-500,width=std::clamp(s.baseWidth,700.,1400.);
        if(left>p.x1||left+width+180<p.x0)continue;
        WaveTrainProfileV2::Crest crest;crest.a=a;crest.envelope=g;
        crest.stage=std::clamp(s.stage,0.,5.)*sstep(.12,.88,g);
        Samples v=blend(crest.stage);Controls delta{};const auto& st=stages();
        auto controlsAt=[&](double stage,int j) {
            stage=std::clamp(stage,0.,5.);const int i=std::min(4,int(stage));
            return cat(st[std::max(0,i-1)][j],st[i][j],st[i+1][j],st[std::min(5,i+2)][j],stage-i);
        };
        const double lip=std::clamp(crest.stage+(s.lipStage<0?0:std::clamp(s.lipStage-s.stage,-.32,.32)),0.,5.);
        for(int j=1;j<21;++j) {
            const double lipWeight=sstep(3,7,j)*(1-sstep(11,15,j));
            delta[j]=(controlsAt(lip,j)-controlsAt(crest.stage,j))*lipWeight;
            delta[j]=delta[j]+V2(.0025*noise1(s.seconds*.31+j*1.7,91+j),.0022*noise1(s.seconds*.27+j*1.3,151+j));
        }
        const double height=std::clamp(s.amplitude,450.,850.)*g;
        auto map=[&](V2 q) {
            const double top=std::max(0.,q.y),tip=sstep(.52,.78,q.x)*sstep(.30,.65,top);
            return V2(left+width*q.x+s.lean*height*top+s.lipThrow*tip*g*sstep(2,5,crest.stage),1080-height*q.y);
        };
        for(int seg=0;seg<Segments;++seg)for(int j=0;j<Steps;++j) {
            const int k=seg*3,index=seg*Steps+j;
            v[index]=v[index]+bezier(delta[k],delta[k+1],delta[k+2],delta[k+3],j/double(Steps));
        }v[Count]=v[0];
        crest.boundary.reserve(Count);for(int j=0;j<Count;++j)crest.boundary.push_back(map(v[j]));
        // Leading edge is retained for W2 claw roots. Foam stays inside water.
        for(int j=8;j<=3*Steps;++j) {
            const V2 q=map(v[j]),t=map(v[j+1])-map(v[j-1]),normal=unit({-t.y,t.x});
            const double w=(5+43*sstep(1,5,crest.stage))*g*std::sin(Pi*(.10+.8*(j-8)/40.));
            crest.outerLip.push_back(q);crest.foamInside.push_back(q+normal*w);
        }
        for(int row=0;row<16;++row) {
            const double f=.055+row*.052+.012*std::sin(s.flow*5+row*.7);
            auto& line=crest.contours[row];line.reserve(49);
            for(int j=0;j<=48;++j) {
                const V2 outside=v[j],inside=v[6*Steps-j];
                line.push_back(map(lerp(outside,inside,f)));
            }
        }
        if(g>best){best=g;out.hero=int(out.crests.size());}
        out.boundaries.push_back(crest.boundary);out.crests.push_back(std::move(crest));
    }return out;
}
void WaveTrainV2::paint(Canvas& cv,const WaveTrainProfileV2& f,const WaveTrainPoseV2& s,const WaveTrainParametersV2& p) {
    auto body=f.surface;body.push_back({p.x1,p.waterline+p.depth});body.push_back({p.x0,p.waterline+p.depth});
    cv.linear(0,p.waterline,0,1290,{{0,p.body,1},{1,p.bottom,1}});fill(cv,body);
    for(int row=0;row<6;++row) {
        std::vector<V2> line;for(auto v:f.surface)line.push_back(v+V2(0,35+row*35));stripe(cv,line,1.1,p.lines,.24);
    }
    for(const auto& crest:f.crests) {
        const double top=1080-s.amplitude*crest.envelope;
        cv.linear(0,top,0,1100,{{0,p.body,1},{.38f,hex(0x274569),1},{.78f,p.bottom,1},{1,hex(0x121827),1}});
        fill(cv,crest.boundary);
        stripe(cv,crest.contours[13],22*crest.envelope,p.bottom,.42);
        for(int row=0;row<16;++row) {
            const double pulse=.5+.5*std::sin(s.flow*8-row*.6);
            if(row%4==0)stripe(cv,crest.contours[row],13*crest.envelope,p.lines,.12+.07*pulse);
            stripe(cv,crest.contours[row],(row%4==0?2:1)*crest.envelope,p.lines,.30+.16*pulse);
        }
        ribbon(cv,crest.outerLip,crest.foamInside,p.underprint);
        std::vector<V2> inner;for(size_t j=0;j<crest.outerLip.size();++j)inner.push_back(lerp(crest.outerLip[j],crest.foamInside[j],.87));
        ribbon(cv,crest.outerLip,inner,p.foam);stripe(cv,crest.outerLip,1.1*crest.envelope,p.lines,.5);
    }
}
void WaveTrainV2::draw(Ctx& c,const WaveTrainPoseV2& s,const WaveTrainParametersV2& p) {
    GpuProfile::Group group(c.gpu.profile,name);auto f=profile(s,p);
    Canvas& cv=c.canvas();paint(cv,f,s,p);c.gpu.over(cv);
}
}
