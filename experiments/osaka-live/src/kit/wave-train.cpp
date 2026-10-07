#include "wave-train.h"
#include "../world.h"
#include <sstream>
namespace Journey::Kit {
void CriticalSpringV2::advance(double target,double omega,double dt) {
    const double error=value-target,b=velocity+omega*error,e=std::exp(-omega*dt);
    value=target+(error+b*dt)*e;velocity=(velocity-omega*b*dt)*e;
}
WaveTrainMotionV2::WaveTrainMotionV2() {
    amplitude_.value=pose_.amplitude;speed_.value=pose_.phaseSpeed;heroScale_.value=.92;
}
void WaveTrainMotionV2::advance(const Audio& a,const Score& score,double seconds,double dt,const WaveTrainParametersV2& params,const WaveTrainCueV2& cue) {
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
    // The surge is the climax: the rising set stands taller than any other.
    const double climax=params.surgeEnabled?clamp01(a.surge):0;
    const bool set=params.setCycle;
    const double approach=set?cue.approach:0,breaking=set?cue.crash:0;
    amplitude_.advance(590+240*std::max(0.,a.bassLevel)/(.20+std::max(0.,a.bassLevel))+130*climax+90*approach+70*breaking
        +70*params.pulseGain*Score::envelope(score.bassHits,seconds-params.pulseDelay,3.5),1.05*beat,dt);
    double stage=std::max(5*sstep(.25,.95,energy),params.surgeEnabled?5*sstep(0,.65,a.surge):0);
    // A charging set builds the curl and the arriving set completes it; the
    // breaking crest holds its own curl, so the next one starts low.
    if(set)stage=std::max(stage*(.5+.5*cue.charge),4.6*approach);
    stage_.advance(stage,.8*beat,dt);
    speed_.advance(14+6*beat+4*clamp01(count/16.),.65*beat,dt);
    const double mid=std::max(0.,(a.bands[2]+a.bands[3])*.5);
    throw_.advance(62*mid/(.18+mid)+48*Score::envelope(score.bassHits,seconds-params.pulseDelay,5)+30*climax+50*approach,2.25*beat,dt);
    lean_.advance(.075*mid/(.18+mid),1.6*beat,dt);
    // Exact underdamped spring at analyzer hops. Lip lag and overshoot are
    // separate from body stage, with a bounded material displacement.
    const double omega=3.4*beat,zeta=.64,w=omega*std::sqrt(1-zeta*zeta);
    const double x=lip_-stage_.value,b=(lipVelocity_+zeta*omega*x)/w;
    const double e=std::exp(-zeta*omega*dt),cs=std::cos(w*dt),sn=std::sin(w*dt);
    lip_=stage_.value+e*(x*cs+b*sn);
    lipVelocity_=e*((-zeta*omega*x+w*b)*cs+(-zeta*omega*b-w*x)*sn);
    pose_.amplitude=amplitude_.value*params.heightScale;pose_.baseWidth=params.width;pose_.stage=stage_.value;pose_.phaseSpeed=speed_.value;
    pose_.sway+=beat*.5*dt;
    pose_.lean=lean_.value+params.swayGain*(.45+.55*energy)*std::sin(Tau*pose_.sway);pose_.lipThrow=throw_.value;pose_.lipStage=lip_;
    pose_.seconds=seconds;pose_.energy=energy;pose_.tempo=tempo;
    pose_.distance+=(oldSpeed+speed_.value)*.5*dt*params.travelScale;pose_.flow+=(.065+.16*energy)*dt;
    if(set) {
        // One group period per set: the crest approaches its break point as
        // the music charges the set, then the break carries it through the
        // sink and the next crest takes its place without a seam.
        const double P=params.groupPeriod,start=params.sinkTo-P,ready=(params.sinkFrom-start)/P;
        if(setU_<0)setU_=.4*ready; // the first crest starts on screen
        if(cue.crashStart>lastCrash_ && cue.crashAge>=0){lastCrash_=cue.crashStart;crashFrom_=setU_;crashing_=true;pose_.crashStage=stage_.value;}
        if(crashing_) {
            const double k=std::clamp(cue.crashAge/5.,0.,1.);
            setU_=std::max(setU_,lerp(crashFrom_,1.,1-std::pow(1-k,2.2)));
            if(k>=1){crashing_=false;setU_=0;setBase_+=P;}
        } else {
            // The next crest glides on screen at once, then waits for the charge.
            const double target=ready*(approach>0?1:.4+.6*std::pow(clamp01(cue.charge),.8));
            if(target>setU_)setU_+=(target-setU_)*-std::expm1(-dt/(approach>0?.8:2.));
        }
        pose_.anchor=start-380+setBase_+setU_*P;
        quiet_.advance(1-sstep(.30,.60,energy),.6,dt);pose_.quiet=clamp01(quiet_.value);
        pose_.crashStart=cue.crashStart;pose_.crashAge=cue.crashAge;pose_.crashStrength=cue.strength;
    }
    if(params.authored){advanceHero(a,score,seconds,dt,energy,beat,params,cue);return;}
    foam_.advance(a,score,pose_,seconds,dt,params);
}
void WaveTrainMotionV2::advanceHero(const Audio& a,const Score& score,double t,double dt,double energy,double beat,
                                    const WaveTrainParametersV2& p,const WaveTrainCueV2& cue) {
    auto& h=pose_.hero;auto& old=pose_.trailing;auto& im=pose_.impact;
    for(auto* s:{&h,&old}) {
        s->seconds=t;s->energy=energy;s->sway+=beat*.5*dt;s->flow+=dt*(.25+1.1*energy)*std::max(.2,p.faceFlow);
        s->pulse=p.pulseGain*Score::envelope(score.bassHits,t-p.pulseDelay,3.5);
        s->flick=Score::envelope(score.onsets,t,6);
    }
    // A new landing: remember the pose it falls from and where the lip lands.
    if(cue.crashStart>heroCrash_ && cue.crashAge>=0) {
        heroCrash_=cue.crashStart;heroFrom_=std::min(heroPhase_.value,4.9);heroSpawned_=false;heroRollFrom_=-1;
        HeroWaveStateV1 landing=h;landing.phase=5;landing.sink=0;landing.pulse=0;landing.sway=0;landing.energy=0;
        im.at=HeroWaveV1::shape(landing).tip();im.strength=cue.strength*h.scale;im.seed=++heroCycle_;
    }
    const double age=t-heroCrash_;
    im.age=age-.25;
    if(!heroSpawned_) {
        // The lip lands a quarter second after the set arrives, the crest
        // spends itself into whitewater, then sinks while the next one forms.
        h.phase=age<.25?lerp(heroFrom_,5.,sstep(0,.25,age)):lerp(5.,6.,easeInOut((age-.25)/1.7));
        h.sink=.95*sstep(.8,3.2,age);h.shift=heroShift_.value+70*sstep(1,5,age);
        heroPhase_.value=h.phase;heroPhase_.velocity=0;
        if(age>=3.0) {
            old=h;pose_.trailingActive=true;heroSpawned_=true;
            heroPhase_.value=0;heroPhase_.velocity=0;heroSink_.value=1;heroSink_.velocity=0;
            heroShift_.value=-320;heroShift_.velocity=0;h.seed=heroCycle_*3+1;
            heroScale_.value=.8;heroScale_.velocity=0;
        }
        return;
    }
    if(pose_.trailingActive) {
        old.phase=6;old.sink=.95*sstep(.8,3.2,age)+.4*sstep(3.2,4.5,age);old.shift+=dt*18;
        if(age>4.6)pose_.trailingActive=false;
    }
    // A rolling set carries the crest through its curl to the plunge; it lands
    // when the set reaches the wave's row.
    double target;
    if(cue.approach>0) {
        if(heroRollFrom_<0)heroRollFrom_=heroPhase_.value;
        const double u=cue.approach;
        target=u<.55?lerp(heroRollFrom_,4.,easeOut(u/.55)):lerp(4.,4.85,easeIn((u-.55)/.45));
        heroPhase_.value=target;heroPhase_.velocity=0;
    } else {
        // Charging music raises it from a swell to a clawed crest; loud
        // passages lift it further, quiet water lets it settle.
        // Each bass hit makes the waiting crest lunge forward and recoil.
        const double quiet=pose_.quiet;
        target=std::clamp(lerp(.25,1.85,std::pow(clamp01(cue.charge),.8))+.5*(energy-.4)-.9*quiet,0.,2.2);
        heroPhase_.advance(target,1.1,dt);
    }
    // Every set has its own size and place: chosen as it forms, and a strong
    // launch makes it stand taller.
    const double base=lerp(.84,1.,hash2(heroCycle_,7)),spot=lerp(-150,60,hash2(heroCycle_,9));
    const double strong=cue.approach>0?std::clamp((cue.setStrength-.6)/.7,0.,1.):0;
    heroScale_.advance(base+.12*strong*(1-pose_.quiet),.8,dt);
    heroSink_.advance(.22*pose_.quiet,.9,dt);heroShift_.advance(spot,.7,dt);
    const double lunge=cue.approach>0?0:.35*std::min(1.,h.pulse)*(1-pose_.quiet);
    h.phase=std::clamp(heroPhase_.value+lunge,0.,6.);h.sink=std::clamp(heroSink_.value,0.,1.);h.shift=heroShift_.value;
    h.scale=heroScale_.value;
    (void)a;
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
double bandLevel(double x) { x=std::isfinite(x)?std::max(0.,x):0.;return x/(.18+x); }
V2 sample(const std::vector<V2>& a,double index) {
    // Extend the final lip tangent for the hooked free ends, never clamp
    // multiple material cells onto the same endpoint.
    if(index>a.size()-1)return a.back()+(a.back()-a[a.size()-2])*(index-(a.size()-1));
    index=std::max(0.,index);int i=std::min(int(index),int(a.size()-2));
    return lerp(a[i],a[i+1],index-i);
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
// Tapered stroke whose heading turns by `turn` toward the end (pow > 1 hooks late).
WaveTrainProfileV2::Strand strand(V2 root,double angle,double length,double width,double turn,double power,int n,double alpha=1) {
    WaveTrainProfileV2::Strand out;out.alpha=alpha;V2 p=root;
    out.centre.reserve(n+1);out.left.reserve(n+1);out.right.reserve(n+1);
    for(int j=0;j<=n;++j) {
        const double u=j/double(n),a=angle+turn*std::pow(u,power);
        const V2 normal{-std::sin(a),std::cos(a)};
        const double w=width>0?width*std::pow(1-u,.95)+std::min(.35,width):0;
        out.centre.push_back(p);out.left.push_back(p+normal*w);out.right.push_back(p-normal*w);
        p=p+V2(std::cos(a),std::sin(a))*(length/n);
    }return out;
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
    const double life=p.sinkTo>p.riseFrom?sstep(p.riseFrom,p.riseTo,a)*(p.setCycle?1:1-sstep(p.sinkFrom,p.sinkTo,a)):1;
    return std::max(p.groupFloor,.08+.92*group/peak)*(1-.62*sstep(1400,2200,a))*life;
}
WaveTrainProfileV2 WaveTrainV2::profile(const WaveTrainPoseV2& s,const WaveTrainParametersV2& p) {
    WaveTrainProfileV2 out;out.surface.reserve(161);
    for(int j=0;j<=160;++j)out.surface.push_back(sea(lerp(p.x0,p.x1,j/160.),s,p));
    if(p.authored) {
        // Dependents (boats, foam masks) read the authored body as a crest.
        auto add=[&](const HeroWaveStateV1& st){
            const auto shape=HeroWaveV1::shape(st);WaveTrainProfileV2::Crest crest;
            crest.boundary=HeroWaveV1::outline(shape,112);crest.outerLip=HeroWaveV1::outerLip(shape);
            crest.envelope=1-st.sink;crest.stage=std::clamp(st.phase*1.25,0.,5.);crest.sink=st.sink;crest.a=shape.tip().x;
            out.boundaries.push_back(crest.boundary);out.crests.push_back(std::move(crest));
        };
        if(s.trailingActive)add(s.trailing);
        out.hero=int(out.crests.size());add(s.hero);
        if(s.impact.active()){out.impact=s.impact.at;out.impactSink=sstep(0,2.5,s.impact.age);}
        return out;
    }
    // A set cycle moves the crests on its own clock; otherwise they travel.
    const double travel=p.setCycle?s.anchor:s.distance;
    const int first=int(std::floor((p.x0-travel-380)/p.groupPeriod))-1;
    const int last=int(std::ceil((p.x1-travel-380)/p.groupPeriod))+1;
    double best=-1;
    for(int n=first;n<=last;++n) {
        const double a=380+travel+n*p.groupPeriod,g=envelope(a,s,p);
        const double left=a-500,width=std::clamp(s.baseWidth,700.,1400.);
        if(left>p.x1||left+width+180<p.x0)continue;
        WaveTrainProfileV2::Crest crest;crest.a=a;crest.envelope=g;
        // A sinking set keeps its curl while it lowers and pitches its lip
        // forward, so it reads as a wave breaking, not a deflating hump.
        const double sink=p.sinkTo>p.sinkFrom?sstep(p.sinkFrom,p.sinkTo,a):0,held=sink>0 && !p.setCycle?g/std::max(.05,1-sink):g;
        // In a set cycle the breaking crest keeps its whole curl while it
        // sinks into the sea; its lip plunges forward and down first.
        crest.stage=std::clamp(p.setCycle?lerp(s.stage,std::max(s.stage,s.crashStage),sstep(0,.08,sink)):s.stage,0.,5.)*sstep(.12,.88,held)*(p.setCycle?1:sstep(0,.3,1-sink));
        crest.sink=p.setCycle?sink:0;
        const double plunge=p.setCycle?std::sin(Pi*std::min(1.,sink*1.6))*(.6+.4*std::min(1.,s.crashStrength)):0;
        const double crash=p.setCycle?150*plunge:sink>0?115*std::sin(Pi*std::min(1.,sink*1.25)):0;
        Samples v=blend(crest.stage);Controls delta{};const auto& st=stages();
        auto controlsAt=[&](double stage,int j) {
            stage=std::clamp(stage,0.,5.);const int i=std::min(4,int(stage));
            return cat(st[std::max(0,i-1)][j],st[i][j],st[i+1][j],st[std::min(5,i+2)][j],stage-i);
        };
        const double lip=std::clamp(crest.stage+(s.lipStage<0?0:std::clamp(s.lipStage-s.stage,-.32,.32)),0.,5.);
        for(int j=1;j<21;++j) {
            const double lipWeight=sstep(3,7,j)*(1-sstep(11,15,j));
            delta[j]=(controlsAt(lip,j)-controlsAt(crest.stage,j))*lipWeight;
            // Slow coherent movement: neighbours share phase, quiet water still breathes.
            const double breathe=.008+.009*s.energy;
            delta[j]=delta[j]+V2(breathe*noise1(s.seconds*.23+j*.17,91),
                breathe*.9*noise1(s.seconds*.19+j*.13,151));
        }
        // Below the curl the authored hump reads as a dark dome; a quiet set
        // settles low into the sea and stands tall again as its lip forms.
        const double settle=p.sinkTo>p.riseFrom?lerp(p.setCycle?.8:.42,1.,sstep(.9,2.9,crest.stage)):1;
        const double height=std::clamp(s.amplitude,450.*p.heightScale,980.*p.heightScale)*g*settle;
        // Lowered whole, never squashed: quiet water sinks the set halfway.
        crest.drop=p.setCycle?height*(1.08*std::pow(sink,1.5)+.5*s.quiet):0;
        const double fall=p.setCycle?.28*height*sstep(.05,.6,sink):0;
        auto map=[&](V2 q) {
            const double top=std::max(0.,q.y),tip=sstep(.52,.78,q.x)*sstep(.30,.65,top);
            return V2(left+width*q.x+s.lean*height*top+(s.lipThrow*g+crash)*tip*sstep(2,5,crest.stage),
                p.baseY-height*q.y+crest.drop+fall*tip);
        };
        for(int seg=0;seg<Segments;++seg)for(int j=0;j<Steps;++j) {
            const int k=seg*3,index=seg*Steps+j;
            v[index]=v[index]+bezier(delta[k],delta[k+1],delta[k+2],delta[k+3],j/double(Steps));
        }v[Count]=v[0];
        crest.boundary.reserve(Count);for(int j=0;j<Count;++j)crest.boundary.push_back(map(v[j]));
        // The retained W1 lip with its slow material wobble; lipNormal points into the water.
        std::vector<V2> lipNormal;
        for(int j=8;j<=3*Steps;++j) {
            const V2 q=map(v[j]),t=map(v[j+1])-map(v[j-1]);
            crest.outerLip.push_back(q);lipNormal.push_back(unit({-t.y,t.x}));
        }
        const int lipCount=int(crest.outerLip.size());
        std::vector<double> lipArc{0};
        for(int j=1;j<lipCount;++j)lipArc.push_back(lipArc.back()+(crest.outerLip[j]-crest.outerLip[j-1]).len());
        const double lipLength=std::max(1.,lipArc.back());
        auto indexAt=[&](double arc) {
            arc=std::clamp(arc,0.,lipLength);
            const int j=std::clamp(int(std::upper_bound(lipArc.begin(),lipArc.end(),arc)-lipArc.begin())-1,0,lipCount-2);
            return j+(arc-lipArc[j])/std::max(1e-9,lipArc[j+1]-lipArc[j]);
        };
        auto tangentAt=[&](double index) {return unit(sample(crest.outerLip,index+.3)-sample(crest.outerLip,index-.3));};
        const double growth=sstep(1.15,4.,crest.stage)*g,highs=bandLevel((s.bands[4]+s.bands[5])*.5);
        // Hokusai claws after Journey kanagawa.cpp and direction_study.cpp:
        // unequal clusters of hooked, forked fingers rise out of the lip and
        // curl over forward. Each finger slot owns a spring; bands run from
        // the shoulder (lows) to the curl tip (highs).
        constexpr int Clusters=11;
        constexpr int counts[Clusters]={3,4,3,3,4,3,4,3,4,3,4};
        constexpr double along[Clusters]={.22,.29,.36,.43,.50,.565,.63,.69,.75,.81,.87};
        constexpr double size[Clusters]={.55,.72,.90,1.05,1.20,1.28,1.18,1.0,.84,.68,.54};
        std::array<double,Clusters> centreArc{},gate{};
        for(int c=0;c<Clusters;++c) {
            const double u=along[c]+.012*std::sin(c*2.7+1.3);centreArc[c]=u*lipLength;
            // The crest top foams first; shoulder and curl tip follow with stage.
            const double d=std::min(1.,std::abs(u-.33)/.6);
            // A set cycle keeps low and quiet water clean: claws need a real face.
            gate[c]=(p.setCycle?sstep(1.9+1.3*d,3.1+1.7*d,crest.stage)*(1-s.quiet):sstep(1.15+1.3*d,2.5+1.7*d,crest.stage))*g;
        }
        double covered=0;int slot=0;
        for(int c=0;c<Clusters;++c) {
            const int n=counts[c];const double sz=size[c];
            const double spacing=(28+16*hash2(c,3))*sz*(.55+.45*gate[c]);
            WaveTrainProfileV2::Clump clump;clump.count=n;
            clump.begin=indexAt(centreArc[c]-spacing*(n-1)*.5-6*sz);clump.end=indexAt(centreArc[c]+spacing*(n-1)*.5+6*sz);
            covered+=spacing*(n-1)+12*sz;crest.clumps.push_back(clump);
            for(int b=0;b<n;++b,++slot) {
                const auto& state=s.fingers[slot];
                const double ext=std::clamp(state.extension,0.,1.15),flick=std::clamp(state.flick,-.35,.35);
                const double h=hash2(slot,11),k=hash2(slot,29),order=n>1?b/double(n-1):.5;
                const double index=indexAt(centreArc[c]+(b-(n-1)*.5+.42*(hash2(slot,7)-.5))*spacing);
                const V2 root=sample(crest.outerLip,index),t=tangentAt(index);
                // Slow per-finger sway keeps quiet water breathing.
                const double sway=.10*noise1(s.seconds*.31+slot*.71,401)+.05*std::sin(s.seconds*.47+slot*1.9);
                // Sickle claws as in the K3 prototype frame: a broad root on the
                // lip, heading forward and down over the face, hooking to a point.
                // Loud reaches further forward; an onset flicks out, then settles.
                const double drop=.38+.28*order+.30*(k-.5)-.18*ext-.9*flick+sway;
                const double turn=(.55+.95*h)*(.80+.30*ext)+1.5*flick+.5*sway;
                const double thin=hash2(slot,53)<.36?.45:1;
                const double length=(45+110*h*h)*(thin<1?1.15:1)*sz*(.50+.60*ext)*gate[c]*(1+.06*noise1(s.seconds*.27+slot,433));
                // Width follows reach so short claws never fold back over their root.
                const double width=std::min((5.5+4*k+5*h)*thin*sz*(.75+.35*ext)*gate[c],.26*length);
                WaveTrainProfileV2::Finger finger;finger.root=root;finger.lipIndex=index;finger.clump=c;
                finger.band=std::min(5,int(6*(slot+.5+.18*std::sin(slot*2.41))/WaveTrainFingerCountV2));
                const double theta=std::atan2(t.y,t.x);
                finger.shadow=strand(root-t*(.5*width),theta+drop-.95-.35*hash2(slot,47),std::min(length*(.28+.30*k),14+30*hash2(slot,49)*gate[c]),std::max(width,4.*gate[c])*(1.1+.5*hash2(slot,49)),turn*.3,1.2,12);
                auto main=strand(root,theta+drop,length,width,turn,1.8,24);
                // Some claws carry a thin side hook from the middle.
                if(hash2(slot,41)>.62) {
                    const int j=int(24*(.40+.15*hash2(slot,43)));
                    const V2 dir=unit(main.centre[j+1]-main.centre[j]);
                    const double half=.5*(main.left[j]-main.right[j]).len();
                    finger.twigs.push_back(strand(main.centre[j],std::atan2(dir.y,dir.x)-.55+.15*sway,length*.36,half*.38,1.9,1.3,12));
                }
                finger.tip=main.centre.back();
                const V2 direction=unit(main.centre.back()-main.centre[main.centre.size()-2]);
                // Signed clockwise angle from the forward ROOT lip tangent.
                finger.angle=length>.5?std::atan2(t.x*direction.y-t.y*direction.x,t.x*direction.x+t.y*direction.y):0;
                for(size_t q=1;q<main.centre.size();++q)finger.length+=(main.centre[q]-main.centre[q-1]).len();
                finger.centre=std::move(main.centre);finger.left=std::move(main.left);finger.right=std::move(main.right);
                crest.fingers.push_back(std::move(finger));
            }
        }
        crest.gapFraction=std::clamp(1-covered/lipLength,0.,1.);
        // Ragged cream sheet: deep under clusters, a thin broken rim between them.
        for(int j=0;j<lipCount;++j) {
            const double u=lipArc[j]/lipLength;double near=0;
            for(int c=0;c<Clusters;++c)near=std::max(near,gate[c]*std::exp(-std::pow((lipArc[j]-centreArc[c])/(60*size[c]),2)));
            const double band=bandLevel(s.bands[std::min(5,int(u*6))]);
            const double back=sstep(.10,.24,u);
            const double depth=back*(2+12*near)*(.70+.65*band)*(1+.40*fbm1(u*6+s.seconds*.05,10))*growth;
            const double rim=back*(2+4*(.5+.5*noise1(u*20+s.seconds*.11,14)))*growth;
            crest.foamRim.push_back(crest.outerLip[j]-lipNormal[j]*rim);
            crest.foamInside.push_back(crest.outerLip[j]+lipNormal[j]*depth);
            crest.foamThickness=std::max(crest.foamThickness,depth+rim);
        }
        // Blue cavities read as lace inside the sheet, not outlined beads.
        for(int k=0;k<26;++k) {
            const double index=indexAt((k+.5+.4*(hash2(k,90)-.5))/26*lipLength);
            const int j=std::min(lipCount-1,int(index+.5));
            const double depth=(crest.foamInside[j]-crest.outerLip[j]).len();
            const V2 n=lipNormal[j],at=sample(crest.outerLip,index)+n*(depth*(.30+.45*hash2(k,91)));
            crest.lace.push_back(strand(at,std::atan2(n.y,n.x)+.7,(5+13*hash2(k,92))*growth,(1+2.2*hash2(k,93))*growth,2.9,1,10,sstep(7,14,depth)));
        }
        // The curl tip unravels into thin tangled tendrils and sheds loose fingers into the barrel.
        const double tipGate=sstep(3.2,4.6,crest.stage)*g;
        for(int k=0;k<5;++k) {
            const double index=indexAt((.86+.028*k+.006*std::sin(k*3.1))*lipLength);
            const V2 root=sample(crest.outerLip,index),t=tangentAt(index);
            const double sway=.14*noise1(s.seconds*.45+k*1.3,451);
            crest.tangle.push_back(strand(root,std::atan2(t.y,t.x)+.25+.35*hash2(k,62)+sway,(30+34*hash2(k,63))*(.55+.75*highs)*tipGate,
                (3.2+2.2*hash2(k,64))*tipGate,1.9+1.1*hash2(k,65),1.5,16));
        }
        for(int k=0;k<6;++k) {
            const double phase=wrap(s.flow*2.4+hash2(k,71),1.);
            const V2 start=sample(crest.outerLip,indexAt((.70+.28*hash2(k,72))*lipLength))+V2(-6-14*hash2(k,73),4);
            const V2 at=start+V2(-(14+20*hash2(k,74))*phase,(40+120*hash2(k,75))*phase*phase+20*phase)*g;
            crest.falling.push_back(strand(at,Pi*.5+.9*(hash2(k,76)-.5)+.5*std::sin(phase*4+k),(10+14*hash2(k,77))*tipGate*(.6+.6*highs),
                (1.6+2*hash2(k,78))*tipGate,(k%2?1:-1)*1.8,1.2,8,sstep(0.,.10,phase)*(1-sstep(.65,1.,phase))));
        }
        // Foam snow: dots shed from the front claws drift down and forward in
        // front of the curl on the flow clock, more of them when loud.
        for(int k=0;k<36;++k) {
            const double phase=wrap(s.flow*1.6+hash2(k,81),1.);
            const int slot=26+int(hash2(k,82)*11.99);
            const auto& claw=crest.fingers[slot];
            if(claw.length<4)continue;
            const double show=sstep(0.,.08,phase)*(1-sstep(.7,1.,phase))*sstep(.25,.75,highs+.3*hash2(k,83))*tipGate;
            if(show<=0)continue;
            const V2 drift=V2(55+70*hash2(k,84),-14-30*hash2(k,85))*phase+V2(0,150*phase*phase);
            crest.snow.push_back(claw.tip+drift*g);crest.snowSize.push_back((1.2+2.2*hash2(k,86))*show*g);
        }
        // Material lines advect from the lower face into the barrel. Wrapping
        // ribbons enter/leave invisibly at endpoints instead of phase popping.
        for(int row=0;row<16;++row) {
            const double phase=wrap(row/16.-s.flow*p.faceFlow,1.);
            const double f=.035+.83*phase;
            crest.contourAlpha[row]=sstep(0.,.06,phase)*(1-sstep(.94,1.,phase));
            auto& line=crest.contours[row];line.reserve(49);
            for(int j=0;j<=48;++j) {
                const V2 outside=v[j],inside=v[6*Steps-j];
                line.push_back(map(lerp(outside,inside,f)));
            }
        }
        // Fixed material bands carry the flat woodblock tones, outer skin to face.
        constexpr double bandAt[6]={0,.10,.30,.52,.74,1};
        for(int b=0;b<6;++b) {
            auto& edge=crest.bandEdges[b];edge.reserve(49);
            for(int j=0;j<=48;++j)edge.push_back(map(lerp(v[j],v[6*Steps-j],bandAt[b])));
        }
        if(p.faceFlow>.14) {
            // The face rolls: four tone stripes travel with the material lines
            // and re-enter beside the fixed outer skin, so the water climbs the
            // wave continuously instead of standing as a still picture.
            const double skin=bandAt[1],offset=wrap(-s.flow*p.faceFlow,1.);
            auto edgeAt=[&](double f){std::vector<V2> e;e.reserve(49);for(int j=0;j<=48;++j)e.push_back(map(lerp(v[j],v[6*Steps-j],f)));return e;};
            auto piece=[&](double a,double b,int tone,bool keyed) {
                WaveTrainProfileV2::Crest::FlowBand band;band.tone=tone;
                auto outer=edgeAt(skin+(1-skin)*a),inner=edgeAt(skin+(1-skin)*b);
                if(keyed)band.lead=outer;
                band.outline=std::move(outer);band.outline.insert(band.outline.end(),inner.rbegin(),inner.rend());
                crest.flowBands.push_back(std::move(band));
            };
            for(int k=0;k<4;++k) {
                const double a=wrap(k/4.+offset,1.),b=a+.25;
                if(b<=1)piece(a,b,k,a>.002);
                else {piece(a,1,k,true);if(b-1>.002)piece(0,b-1,k,false);}
            }
        }
        // Small broken whitecaps ride the face; their roots share the contour
        // transform. High bands alter size continuously, never a spawn switch.
        const double high=bandLevel((s.bands[4]+s.bands[5])*.5);
        for(int i=0;i<11;++i) {
            WaveTrainProfileV2::Whitecap cap;
            const double f=.13+.055*i,index=12+(i*17%31),size=(3+13*high)*g*sstep(.8,3.,crest.stage);
            for(int k=0;k<=8;++k) {
                const double j=index+(k-4)*.20;int lo=int(j);double frac=j-lo;
                const V2 q=map(lerp(lerp(v[lo],v[lo+1],frac),lerp(v[96-lo],v[95-lo],frac),f));
                cap.edge.push_back(q);
                cap.inside.push_back(q+V2(.3*size,size*std::sin(Pi*k/8.)*(.55+.3*std::sin(k*2.1+i+s.seconds*.37))));
            }crest.whitecaps.push_back(std::move(cap));
        }
        double selection=g*(1-crest.sink);
        if(crest.sink>0 && out.impactSink==0) {
            out.impactSink=crest.sink;out.impact=crest.outerLip.front();
            for(auto q:crest.outerLip)if(q.x>out.impact.x)out.impact=q;
        }
        if(p.surgeEnabled){double l=1e9,r=-1e9;for(auto q:crest.boundary){l=std::min(l,q.x);r=std::max(r,q.x);}
            selection*=std::max(0.,std::min(1920.,r)-std::max(0.,l))/std::max(1.,r-l);}
        if(selection>best){best=selection;out.hero=int(out.crests.size());}
        out.boundaries.push_back(crest.boundary);out.crests.push_back(std::move(crest));
    }
    const double high=bandLevel((s.bands[4]+s.bands[5])*.5);
    for(int i=0;i<14;++i) {
        WaveTrainProfileV2::Whitecap cap;const double a=lerp(p.x0,p.x1,(i+.5)/14.);
        for(int k=0;k<=10;++k) {
            const double z=k/10.;const V2 q=sea(a+(z-.5)*(14+35*high),s,p);
            cap.edge.push_back(q);cap.inside.push_back(q+V2(0,(2+7*high)*std::sin(Pi*z)*(1+.22*std::sin(k*2.3+i+s.seconds*.3))));
        }out.swellCaps.push_back(std::move(cap));
    }return out;
}
void WaveTrainV2::paint(Canvas& cv,const WaveTrainProfileV2& f,const WaveTrainPoseV2& s,const WaveTrainParametersV2& p) {
    if(p.authored) {
        if(s.trailingActive)HeroWaveV1::paint(cv,HeroWaveV1::shape(s.trailing),s.trailing);
        HeroWaveV1::paint(cv,HeroWaveV1::shape(s.hero),s.hero);
        HeroWaveV1::paintImpact(cv,s.impact,s.seconds);
    }
    for(const auto& crest:f.crests) {
        if(p.authored)break;
        const double top=p.baseY-s.amplitude*crest.envelope+crest.drop;
        // Woodblock body: flat tone bands with a printed top-to-base gradation
        // (bokashi), thin light key lines between bands, sparse flowing veins.
        static const Col tones[5]={hex(0x1a3a66),hex(0x2a5a88),hex(0x3a6d98),hex(0x2d6090),hex(0x1e4775)};
        const Col deep=hex(0x0f2347);
        const bool rolling=!crest.flowBands.empty();
        for(int b=0;b<(rolling?1:5);++b) {
            auto band=crest.bandEdges[b];band.insert(band.end(),crest.bandEdges[b+1].rbegin(),crest.bandEdges[b+1].rend());
            cv.linear(0,top,0,p.baseY+crest.drop,{{0,tones[b],1},{.55f,mix(tones[b],deep,.25),1},{1,mix(tones[b],deep,.6),1}});
            fill(cv,band);
        }
        if(rolling) {
            static const Col rolled[4]={hex(0x27578a),hex(0x4178a6),hex(0x2b5d8e),hex(0x3a6f9e)};
            for(const auto& band:crest.flowBands) {
                const Col tone=rolled[band.tone];
                cv.linear(0,top,0,p.baseY+crest.drop,{{0,tone,1},{.55f,mix(tone,deep,.25),1},{1,mix(tone,deep,.6),1}});
                fill(cv,band.outline);
            }
            stripe(cv,crest.bandEdges[1],1.3*crest.envelope,p.lines,.45);
            for(const auto& band:crest.flowBands)if(!band.lead.empty())stripe(cv,band.lead,1.5*crest.envelope,p.lines,.6);
        } else for(int b=1;b<5;++b)stripe(cv,crest.bandEdges[b],1.3*crest.envelope,p.lines,.45);
        for(int row=0;row<16;++row) {
            const double alpha=crest.contourAlpha[row];if(alpha<=0)continue;
            const auto& line=crest.contours[row];
            if(row%4==1 && rolling) {
                // Foam streaks climb the face into the lip on the flow clock,
                // two per vein, faster when the music is loud.
                const int from=10+row%3*3;
                for(int k=0;k<2;++k) {
                    const double u=wrap(s.flow*p.faceFlow*1.6+.5*k+.37*row,1.);
                    const double span=13,head=from-span+(47-from+span)*u;
                    std::vector<V2> l,r;
                    for(int step=0;step<=12;++step) {
                        const double jj=head+span*step/12.;
                        if(jj<from || jj>47)continue;
                        const int j=int(jj);const double fr=jj-j;
                        const V2 q=lerp(line[j],line[std::min(48,j+1)],fr);
                        const V2 t=unit(line[std::min(48,j+1)]-line[std::max(0,j-1)]),n{-t.y,t.x};
                        const double w=2.6*crest.envelope*std::pow(std::sin(Pi*step/12.),.8)*sstep(from,from+6,jj);
                        l.push_back(q+n*w);r.push_back(q-n*w);
                    }
                    if(l.size()>2)ribbon(cv,l,r,p.foam,.8*alpha);
                }
            } else if(row%4==1) {
                // Cream vein: tapered, only on the upper face and into the curl.
                const int from=14+row%3*3;std::vector<V2> l,r;
                for(int j=from;j<=47;++j) {
                    const double z=(j-from)/double(47-from),w=2.4*crest.envelope*std::pow(std::sin(Pi*z),.7);
                    const V2 t=unit(line[j+1]-line[j-1]),n{-t.y,t.x};l.push_back(line[j]+n*w);r.push_back(line[j]-n*w);
                }
                ribbon(cv,l,r,p.foam,.75*alpha);
            } else if(row%2==0) {
                std::vector<V2> part(line.begin()+8,line.end());
                stripe(cv,part,1.0*crest.envelope,p.lines,.30*alpha);
            }
        }
        {
            auto loop=crest.boundary;loop.push_back(loop.front());
            stripe(cv,loop,2.2*crest.envelope,hex(0x0e2140),.85);
        }
        // Sheet and lace first, then each stroke's offset blue underprint and
        // cream in lip order, so later claws overlap earlier ones like a print.
        if(crest.foamRim.size()>1) {
            stripe(cv,crest.foamRim,1.6*crest.envelope*sstep(.4,1.5,crest.foamThickness),hex(0x0e2140),.85);
            ribbon(cv,crest.foamRim,crest.foamInside,p.foam);
        }
        for(const auto& l:crest.lace)if(l.alpha>0)ribbon(cv,l.left,l.right,p.bottom,.95*l.alpha);
        // Key-block outline: each cream stroke sits on a slightly wider dark
        // copy, so overlapping claws stay legible against sky and face alike.
        const Col key=hex(0x0e2140);
        auto print=[&](const WaveTrainProfileV2::Strand& st,double alpha) {
            const double root=.5*(st.left[0]-st.right[0]).len();
            if(root<=0||alpha<=0)return;
            const double edge=std::clamp(.35*root,.8,1.8);
            std::vector<V2> l,r;l.reserve(st.centre.size());r.reserve(st.centre.size());
            for(size_t j=0;j<st.centre.size();++j) {
                const V2 half=(st.left[j]-st.right[j])*.5,n=unit(half)*(half.len()+edge);
                l.push_back(st.centre[j]+n);r.push_back(st.centre[j]-n);
            }
            ribbon(cv,l,r,key,.9*alpha);ribbon(cv,st.left,st.right,p.foam,alpha);
        };
        for(const auto& f:crest.fingers)if(f.length>.5) {
            print({f.centre,f.left,f.right,1},f.opacity);
            for(const auto& twig:f.twigs)print(twig,f.opacity);
        }
        for(const auto& st:crest.tangle)print(st,st.alpha);
        for(size_t k=0;k<crest.snow.size();++k) {
            const V2 q=crest.snow[k];const double r=crest.snowSize[k];
            cv.disc(q.x,q.y,r,p.foam,.95);
        }
        for(const auto& st:crest.falling)print(st,st.alpha);
    }
    // The near water passes in front of every crest, so the wave rises out of
    // the sea instead of standing on it.
    // A smaller swell rises in front of the hero's foot, as in the print's
    // foreground, so the wave's base sinks into the trough instead of
    // ending in a hard wedge.
    double humpX=0,humpWidth=1,humpHeight=0;
    if(f.hero>=0) {
        const auto& b=f.crests[f.hero].boundary;double lo=1e9,hi=-1e9,top=1e9;
        for(int i=78;i<=96;++i){lo=std::min(lo,b[i].x);hi=std::max(hi,b[i].x);top=std::min(top,b[i].y);}
        humpX=.5*(lo+hi)+.10*(hi-lo);humpWidth=std::max(60.,.55*(hi-lo));
        humpHeight=p.footSwell*std::max(0.,p.waterline-top+14);
    }
    auto surface=f.surface;std::vector<V2> lip;
    for(auto& q:surface) {
        const double bump=std::exp(-.5*std::pow((q.x-humpX)/humpWidth,2));
        q.y-=humpHeight*bump;if(bump>.18)lip.push_back(q);
    }
    auto body=surface;body.push_back({p.x1,p.waterline+p.depth});body.push_back({p.x0,p.waterline+p.depth});
    {
        // Printed like the swells behind it: light at the surface, dark below.
        double hi=1e9;for(auto q:surface)hi=std::min(hi,q.y);
        const Col light=mix(p.body,hex(0x7fb4ca),.42);
        cv.linear(0,hi,0,hi+260,{{0,light,1},{.4f,mix(light,p.bottom,.5),1},{1,p.bottom,1}});fill(cv,body);
        stripe(cv,surface,1.6,hex(0x143154),.6);
    }
    for(int row=0;row<6;++row) {
        std::vector<V2> line;for(auto v:surface)line.push_back(v+V2(0,35+row*35));stripe(cv,line,1.1,p.lines,.24);
    }
    if(lip.size()>2) {
        // Cream crest on the foreground swell: thick and broken at the peak,
        // a thin rim down its shoulders, with small sickle claws spilling
        // down its front like the hero's.
        std::vector<V2> l,r;const double scale=std::min(1.,humpHeight/60);
        size_t peak=0;for(size_t j=1;j<lip.size();++j)if(lip[j].y<lip[peak].y)peak=j;
        const double spread=std::max(3.,.16*lip.size());
        for(size_t j=0;j<lip.size();++j) {
            const double near=std::exp(-std::pow((double(j)-double(peak)-.6*spread)/spread,2));
            const double w=(1.8+14*near*(.75+.25*noise1(j*.9+s.seconds*.2,57)))*scale;
            l.push_back(lip[j]+V2(0,-1));r.push_back(lip[j]+V2(0,w));
        }
        stripe(cv,l,1.4,hex(0x0e2140),.8);ribbon(cv,l,r,p.foam,.95);
        const double size=scale;
        for(int k=0;k<5 && size>.15;++k) {
            const size_t j=std::min(lip.size()-2,peak+size_t(std::round((.2+.45*k)*spread)));
            const V2 t=unit(lip[j+1]-lip[j]);
            auto claw=strand(lip[j]+V2(0,4),std::atan2(t.y,t.x)+.30+.10*hash2(k,5),(46-5*k)*size*(.8+.4*hash2(k,6)),(7-.7*k)*size,.9+.3*hash2(k,7),1.6,16);
            const Col key=hex(0x0e2140);std::vector<V2> kl,kr;
            for(size_t q=0;q<claw.centre.size();++q){const V2 h=(claw.left[q]-claw.right[q])*.5,n=unit(h)*(h.len()+1.1);kl.push_back(claw.centre[q]+n);kr.push_back(claw.centre[q]-n);}
            ribbon(cv,kl,kr,key,.85);ribbon(cv,claw.left,claw.right,p.foam);
        }
    }
    if(p.setCycle && f.impactSink>0) {
        // Whitewater boils up along the near sea where the plunging lip
        // lands, spreads both ways and falls back.
        const double age=s.crashAge,amount=std::min(1.,s.crashStrength)*sstep(.15,.45,f.impactSink)*(1-sstep(2.6,6.5,age));
        auto surfaceAt=[&](double x){const double u=std::clamp((x-p.x0)/(p.x1-p.x0)*160,0.,159.999);const int j=int(u);
            return lerp(surface[j].y,surface[j+1].y,u-j);};
        if(amount>.01) {
            const double spread=sstep(.6,4.,age),span=160+680*spread,x0=f.impact.x-.35*span;
            constexpr int Lumps=22;const Col key=hex(0x0e2140);
            for(int pass=0;pass<2;++pass)for(int k=0;k<Lumps;++k) {
                const double u=(k+.5+.6*(hash2(k,301)-.5))/Lumps,x=x0+span*u;
                const double boil=.75+.25*std::sin(s.seconds*(2.1+1.3*hash2(k,302))+k);
                const double h=(22+80*hash2(k,303))*amount*boil*(1-.55*std::abs(u-.38)/.62)*(1-.5*spread*hash2(k,304));
                const double r=(18+34*hash2(k,305))*(.6+.6*spread),edge=pass?0:1.6;
                if(h<1.5)continue;
                std::vector<V2> l,b;
                for(int j=0;j<=12;++j) {
                    const double v=j/12.,xx=x+(2*v-1)*(r+edge),y=surfaceAt(xx)+4;
                    l.push_back({xx,y-(h+edge)*std::pow(std::sin(Pi*v),.7)});b.push_back({xx,y+edge});
                }
                ribbon(cv,l,b,pass?p.foam:key,pass?.97:.8);
            }
        }
    }
    for(const auto& cap:f.swellCaps)ribbon(cv,cap.edge,cap.inside,p.foam,.85);
    for(const auto& d:s.droplets)if(d.life>0 && d.age<d.life) {
        const double size=d.size*(1-sstep(.30,1.,d.age/d.life));
        const V2 t=unit(d.velocity),n={-t.y,t.x},q=d.position;
        // Crisp teardrops and irregular flecks, no sprites/glow/blur.
        if(d.serial%4==0) {
            std::vector<V2> l,r;
            for(int j=0;j<=10;++j) {
                const double z=j/10.;const V2 point=q+t*(size*6*(z-.5))+n*(size*2.3*std::sin(Pi*z));
                const double w=size*.45*std::pow(std::sin(Pi*z),.8);
                l.push_back(point+n*w);r.push_back(point-n*w);
            }
            ribbon(cv,l,r,p.bottom,.8);stripe(cv,l,size*.35,p.foam,1);ribbon(cv,l,r,p.foam);
        } else {
            cv.tri(q-t*size*1.8,q+n*size*.55,q+t*size*.55,p.foam);
            cv.tri(q-t*size*1.8,q+t*size*.55,q-n*size*.65,p.foam);
        }
    }
}
void WaveTrainFoamMotionV2::advance(const Audio& a,const Score& score,WaveTrainPoseV2& pose_,double seconds,double dt,const WaveTrainParametersV2& params) {
    if(!std::isfinite(dt)||dt<=0||dt>.25||!std::isfinite(seconds))return;
    // A new break flicks every claw and throws spray off the lip.
    const bool burst=params.setCycle && pose_.crashStart>burstCrash_ && pose_.crashAge>=0;
    if(burst) {
        burstCrash_=pose_.crashStart;impactThrown_=false;
        for(int i=0;i<WaveTrainFingerCountV2;++i)flickVelocity_[i]+=2.4*(.8+.2*std::sin(i*1.7));
    }
    std::array<bool,6> onset{};
    for(int band=0;band<6;++band) {
        const double raw=std::isfinite(a.bands[band])?std::max(0.,a.bands[band]):0.;
        onset[band]=raw-previousBand_[band]>.035 && raw>.065 && seconds-lastBandOnset_[band]>.18;
        if(onset[band])lastBandOnset_[band]=seconds;
        previousBand_[band]=raw;
        pose_.bands[band]+=(raw-pose_.bands[band])*-std::expm1(-dt/.23);
    }
    for(int i=0;i<WaveTrainFingerCountV2;++i) {
        const int band=std::min(5,int(6*(i+.5+.18*std::sin(i*2.41))/WaveTrainFingerCountV2));
        fingerLength_[i].advance(bandLevel(pose_.bands[band]),3.2+(i%4)*.29,dt);
        if(onset[band])flickVelocity_[i]+=2.7*bandLevel(a.bands[band])*(.75+.25*std::sin(i*1.4));
        const double omega=4.7+(i%5)*.18,zeta=.42,w=omega*std::sqrt(1-zeta*zeta);
        const double x=flick_[i],b=(flickVelocity_[i]+zeta*omega*x)/w,e=std::exp(-zeta*omega*dt),cs=std::cos(w*dt),sn=std::sin(w*dt);
        flick_[i]=e*(x*cs+b*sn);flickVelocity_[i]=e*((-zeta*omega*x+w*b)*cs+(-zeta*omega*b-w*x)*sn);
        pose_.fingers[i]={fingerLength_[i].value,std::clamp(flick_[i],-.35,.35)};
    }
    for(auto& d:pose_.droplets)if(d.life>0 && d.age<d.life) {
        d.age+=dt;const double drag=std::exp(-.28*dt);
        d.velocity.x=(d.velocity.x+10*dt)*drag;d.velocity.y=(d.velocity.y+86*dt)*drag;
        d.position=d.position+d.velocity*dt;
    }
    const Event* kick=Score::last(score.bassHits,seconds);const bool kicked=kick && kick->serial!=lastKick_;
    if(kicked)lastKick_=kick->serial;
    bool triggered=false;for(bool b:onset)triggered=triggered||b;
    // Bounded at 28 particles per event and 10 event groups/second. Work cannot
    // scale with amplitude or arbitrary event queue lengths.
    sprayClock_=std::max(0.,sprayClock_-dt);
    const auto shape=WaveTrainV2::profile(pose_,params);
    if(shape.hero<0)return;const auto& hero=shape.crests[shape.hero];
    auto random=[&](){rng_^=rng_<<13;rng_^=rng_>>17;rng_^=rng_<<5;return (rng_&0xffffff)/double(0x1000000);};
    if((triggered||kicked) && sprayClock_<=0 && hero.stage>1.15) {
        sprayClock_=.10;
        for(int j=0;j<(kicked?28:12);++j) {
            const int i=std::min(WaveTrainFingerCountV2-1,int(random()*WaveTrainFingerCountV2));
            if(!kicked && !onset[hero.fingers[i].band])continue;
            auto slot=std::find_if(pose_.droplets.begin(),pose_.droplets.end(),[](const auto& d){return d.life==0||d.age>=d.life;});
            if(slot==pose_.droplets.end())break;
            const auto& f=hero.fingers[i];const V2 motion=tipsReady_?(f.tip-previousTip_[i])*(1/dt):V2{};
            const V2 drift=unit(motion)*std::min(110.,motion.len());
            const V2 tangent=unit(f.tip-f.centre[f.centre.size()-3]);
            const bool crestFleck=j%4==0;
            *slot={crestFleck?f.root:f.tip,drift+tangent*(40+65*random())+V2(12+22*random(),-24-32*random()),
                0,1.4+1.3*random(),2.0+4.2*random(),++serial_};
        }
    }
    auto toss=[&](V2 at,V2 velocity,double life,double size) {
        auto slot=std::find_if(pose_.droplets.begin(),pose_.droplets.end(),[](const auto& d){return d.life==0||d.age>=d.life;});
        if(slot!=pose_.droplets.end())*slot={at,velocity,0,life,size,++serial_};
    };
    if(burst)for(int j=0;j<64;++j) {
        const auto& f=hero.fingers[std::min(WaveTrainFingerCountV2-1,int(random()*WaveTrainFingerCountV2))];
        if(f.length<4)continue;
        const V2 tangent=unit(f.tip-f.centre[f.centre.size()-3]);
        toss(f.tip,tangent*(60+100*random())+V2(20+60*random(),-90-150*random()),1.8+1.2*random(),2.4+4.2*random());
    }
    // The landing lip throws a second burst up from the whitewater.
    if(params.setCycle && !impactThrown_ && shape.impactSink>.28 && pose_.crashAge<4) {
        impactThrown_=true;
        for(int j=0;j<90;++j)toss(shape.impact+V2(-80+260*random(),-4),V2(-60+200*random(),-110-210*random()),1.5+1.2*random(),2.6+5.4*random());
    }
    for(int i=0;i<WaveTrainFingerCountV2;++i)previousTip_[i]=hero.fingers[i].tip;
    tipsReady_=true;
}
WaveTrainPoseV2 WaveTrainV2::worldPose(const Ctx& c) {
    return c.schedule?c.schedule->waveTrain.pose():WaveTrainPoseV2{};
}
const WaveTrainProfileV2& WaveTrainV2::worldProfile(const Ctx& c,const WaveTrainParametersV2& p) {
    struct Cached {double t=-1;WaveTrainPoseV2 pose;WaveTrainProfileV2 field;};
    std::ostringstream key;key.precision(17);key<<name;
    for(double v:{p.x0,p.x1,p.waterline,p.wavelength,p.groupPeriod,p.groupWidth,p.groupOrigin,p.groupFloor,p.baseY,p.width,p.heightScale})key<<':'<<v;
    // Non-rendering callers (including CTests) own their fallback, not shared global state.
    if(!c.staticGeometry){thread_local WaveTrainProfileV2 field;field=profile(worldPose(c),p);return field;}
    auto& any=c.staticGeometry->layouts[key.str()];
    if(!any.has_value())any=Cached{};auto& cache=std::any_cast<Cached&>(any);
    const auto pose=worldPose(c);const auto& old=cache.pose;
    bool same=cache.t==c.t && old.amplitude==pose.amplitude && old.stage==pose.stage && old.lipStage==pose.lipStage
        && old.distance==pose.distance && old.anchor==pose.anchor && old.quiet==pose.quiet && old.crashAge==pose.crashAge && old.flow==pose.flow && old.lean==pose.lean && old.lipThrow==pose.lipThrow && old.energy==pose.energy && old.bands==pose.bands;
    for(int i=0;i<WaveTrainFingerCountV2 && same;++i)same=old.fingers[i].extension==pose.fingers[i].extension && old.fingers[i].flick==pose.fingers[i].flick;
    if(!same){cache.field=profile(pose,p);cache.t=c.t;cache.pose=pose;}
    return cache.field;
}
double WaveTrainV2::exclusionRight(const WaveTrainProfileV2::Crest& crest) {
    double right=-1e9;
    auto add=[&](const auto& points){for(auto q:points)right=std::max(right,q.x);};
    add(crest.outerLip);add(crest.foamRim);add(crest.foamInside);
    for(const auto& f:crest.fingers){add(f.left);add(f.right);add(f.shadow.left);add(f.shadow.right);for(const auto& st:f.twigs){add(st.left);add(st.right);}}
    for(const auto& st:crest.tangle){add(st.left);add(st.right);}
    for(const auto& st:crest.falling){add(st.left);add(st.right);}
    return right;
}
double WaveTrainV2::surfaceY(const WaveTrainProfileV2& field,double x,double sea) {
    auto crossing=[&](V2 a,V2 b,double& y){if(x<std::min(a.x,b.x)||x>std::max(a.x,b.x)||std::abs(b.x-a.x)<1e-9)return;
        y=std::min(y,lerp(a.y,b.y,(x-a.x)/(b.x-a.x)));};
    double y=1e9;
    for(size_t i=1;i<field.surface.size();++i)crossing(field.surface[i-1],field.surface[i],y);
    if(y==1e9)y=sea;
    // Top water boundary. Boats exclude the overhang separately before sampling.
    for(const auto& crest:field.crests)for(int i=1;i<=96;++i)crossing(crest.boundary[i-1],crest.boundary[i],y);
    return y;
}
void WaveTrainV2::drawWorld(Ctx& c,const WaveTrainParametersV2& p) {
    GpuProfile::Group group(c.gpu.profile,name);const auto& f=worldProfile(c,p);
    Canvas& cv=c.canvas();paint(cv,f,worldPose(c),p);c.gpu.over(cv);
}
void WaveTrainV2::draw(Ctx& c,const WaveTrainPoseV2& s,const WaveTrainParametersV2& p) {
    GpuProfile::Group group(c.gpu.profile,name);auto f=profile(s,p);
    Canvas& cv=c.canvas();paint(cv,f,s,p);c.gpu.over(cv);
}
}
