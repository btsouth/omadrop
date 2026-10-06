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
    foam_.advance(a,score,pose_,seconds,dt);
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
            // Slow coherent movement: neighbours share phase, quiet water still breathes.
            const double breathe=.008+.009*s.energy;
            delta[j]=delta[j]+V2(breathe*noise1(s.seconds*.23+j*.17,91),
                breathe*.9*noise1(s.seconds*.19+j*.13,151));
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
        // Follow the retained W1 profile with its slow material wobble. Foam is on the water side;
        // talons extrude into air/barrel along the opposite normal.
        for(int j=8;j<=3*Steps;++j) {
            const V2 q=map(v[j]),t=map(v[j+1])-map(v[j-1]),normal=unit({-t.y,t.x});
            const double u=(j-8)/40.,band=s.bands[std::min(5,int(u*6))];
            const double breakup=1+.13*std::sin(j*2.13+s.seconds*.41)+.10*noise1(j*.8+s.seconds*.18,813);
            const double w=(4+27*sstep(1,5,crest.stage))*(.72+1.10*bandLevel(band))*g
                *std::sin(Pi*(.10+.8*u))*breakup;
            crest.outerLip.push_back(q);crest.foamInside.push_back(q+normal*w);
        }
        std::vector<double> lipArc{0};
        for(size_t j=1;j<crest.outerLip.size();++j)lipArc.push_back(lipArc.back()+(crest.outerLip[j]-crest.outerLip[j-1]).len());
        auto arcSample=[&](double arc) {
            if(arc>=lipArc.back())return crest.outerLip.back()+unit(crest.outerLip.back()-crest.outerLip[crest.outerLip.size()-2])*(arc-lipArc.back());
            const int j=std::max(0,int(std::upper_bound(lipArc.begin(),lipArc.end(),arc)-lipArc.begin())-1);
            return lerp(crest.outerLip[j],crest.outerLip[j+1],(arc-lipArc[j])/(lipArc[j+1]-lipArc[j]));
        };
        const double growth=sstep(1.15,4.,crest.stage);
        // Nine seeded hands, not a cream rim with regularly spaced spikes.
        // Fixed slots keep springs and the spray pool deterministic; the last
        // four slots are thin tip lace whose opacity opens with high bands.
        constexpr int counts[]={3,4,2,5,3,4,5,5,7};
        constexpr double centres[]={3.2,8.4,14.0,19.6,24.1,28.7,33.0,36.7,39.3};
        constexpr double spans[]={3.2,3.8,2.8,3.9,2.8,3.0,2.5,1.9,.9};
        int slot=0;double covered=0;
        auto arcAt=[&](double index) {int j=int(index);return lerp(lipArc[j],lipArc[j+1],index-j);};
        // Integrate one scalar streamline potential, then transport each
        // root through it analytically. A shared chart and width cells prevent
        // crossings at different lengths without per-frame path unions.
        std::array<double,38> rootArcs{};
        int rootSlot=0;
        std::array<double,9> begins{},ends{};
        for(int hand=0;hand<9;++hand) {
            const int firstSlot=rootSlot;
            const double ex=std::clamp(s.fingers[firstSlot].extension,0.,1.15);
            const double half=spans[hand]*(.75+.25*ex)*.5;
            begins[hand]=arcAt(centres[hand]-half);ends[hand]=arcAt(centres[hand]+half);
            for(int j=0;j<counts[hand];++j)
                rootArcs[rootSlot++]=arcAt(lerp(centres[hand]-half,centres[hand]+half,(j+.5+.19*std::sin(hand*2.13+j*4.7))/counts[hand]));
        }
        std::vector<double> potential(int(lipArc.back()+200)+2,0);
        const double decay=std::exp(-1./28);
        for(size_t j=1;j<potential.size();++j) {
            const double x=j-.5;double force=.035;
            for(int hand=0;hand<9;++hand)
                force+=(1.2+.12*std::sin(hand*1.91))*(1-sstep(ends[hand]+8*growth,ends[hand]+17*growth,x))
                    *sstep(begins[hand]-4*growth,begins[hand]+4*growth,x);
            potential[j]=potential[j-1]*decay+force*28*(1-decay);
        }
        auto transported=[&](double root,double arc) {
            auto value=[&](double x) {int j=std::clamp(int(x),0,int(potential.size()-2));return lerp(potential[j],potential[j+1],x-j);};
            return std::max(0.,value(arc)-value(root)*std::exp(-(arc-root)/28));
        };
        for(int hand=0;hand<9;++hand) {
            const double centre=centres[hand],u=centre/40.;
            const int mainCount=hand==8?3:counts[hand];
            const double extension=std::clamp(s.fingers[slot].extension,0.,1.15);
            const double spread=.75+.25*extension;
            const double begin=centre-spans[hand]*spread*.5,end=centre+spans[hand]*spread*.5;
            WaveTrainProfileV2::Clump palm;palm.count=mainCount;palm.begin=begin;palm.end=end;
            for(int k=0;k<=12;++k) {
                const double z=k/12.,index=lerp(begin,end,z);const V2 q=sample(crest.outerLip,index);
                const V2 t=unit(sample(crest.outerLip,index+.25)-sample(crest.outerLip,index-.25));
                const V2 inward={-t.y,t.x};
                const double w=(10+23*(1-u))*(.70+.60*extension)*growth*g
                    *std::pow(std::sin(Pi*z),.55)*(1+.13*std::sin(z*8+hand*2.3+s.seconds*.22));
                palm.edge.push_back(q);palm.inside.push_back(q+inward*w);
                crest.foamThickness=std::max(crest.foamThickness,w);
            }
            covered+=arcAt(end)-arcAt(begin);crest.clumps.push_back(std::move(palm));
            for(int local=0;local<counts[hand];++local,++slot) {
                const bool lace=hand==8 && local>=3;
                const double zroot=(local+.5+.19*std::sin(hand*2.13+local*4.7))/counts[hand];
                const double index=lerp(begin,end,zroot);
                const V2 root=sample(crest.outerLip,index),t=unit(sample(crest.outerLip,index+.3)-sample(crest.outerLip,index-.3));
                const double rootArc=arcAt(index),cell=(arcAt(end)-arcAt(begin))/counts[hand];
                const double ext=std::clamp(s.fingers[slot].extension,0.,1.15);
                const double seed=.5+.5*std::sin(hand*4.13+local*2.71);
                const double authored=(40+43*(1-u))*(.53+.75*ext+.10*std::clamp(s.fingers[slot].flick,-.35,.35))*(.78+.40*seed);
                const double cap=std::min(185.,std::max(1.,(lipArc.back()-rootArc)*.87+85*sstep(3.4,4.8,crest.stage)));
                const double reach=cap*-std::expm1(-(arcAt(end)-rootArc+authored)/cap)*growth*g;
                WaveTrainProfileV2::Finger finger;finger.root=root;finger.lipIndex=index;
                finger.band=std::min(5,int(6*(slot+.5+.18*std::sin(slot*2.41))/WaveTrainFingerCountV2));
                finger.clump=hand;finger.tendril=lace;
                finger.opacity=lace?(.18+.82*bandLevel(s.bands[5])):1;
                const double width=std::min(14.,cell*.29)*(lace?.22:1)*growth*(.75+.30*ext);
                std::array<double,25> cellWidths{};
                for(int k=0;k<=24;++k) {
                    const double z=k/24.,arc=rootArc+reach*z;
                    const V2 anchor=arcSample(arc),tangent=unit(arcSample(arc+10)-arcSample(std::max(0.,arc-10)));
                    const V2 normal={tangent.y,-tangent.x};
                    const V2 before=unit(arcSample(arc)-arcSample(std::max(0.,arc-10))),after=unit(arcSample(arc+10)-arcSample(arc));
                    const double radius=20/std::max(.01,(after-before).len());
                    const double limit=radius*.38;
                    auto chart=[&](double q) {return limit*std::tanh(transported(q,arc)/limit);};
                    double h=chart(rootArc),space=h;
                    if(slot && arc>=rootArcs[slot-1])space=std::min(space,std::abs(chart(rootArcs[slot-1])-h));
                    if(slot+1<38 && arc>=rootArcs[slot+1])space=std::min(space,std::abs(chart(rootArcs[slot+1])-h));
                    cellWidths[k]=.25*space;
                    // Thin lace is separately bounded and may cross at the tip.
                    if(lace)h+=growth*(8+7*ext)*std::sin(Pi*z)*(.55+.45*std::sin(9*z+local*1.7));
                    finger.centre.push_back(anchor+normal*h);
                }
                for(int k=0;k<=24;++k) {
                    const double z=k/24.;const V2 direction=unit(finger.centre[std::min(k+1,24)]-finger.centre[std::max(k-1,0)]);
                    const V2 normal={-direction.y,direction.x};
                    const double arc=rootArc+reach*z;
                    const double air=(finger.centre[k]-arcSample(arc)).len();
                    const V2 a=unit(finger.centre[k]-finger.centre[std::max(k-1,0)]),b=unit(finger.centre[std::min(k+1,24)]-finger.centre[k]);
                    const double radius=(finger.centre[std::min(k+1,24)]-finger.centre[std::max(k-1,0)]).len()/std::max(.01,(b-a).len());
                    const double w=std::min(lace?2.0*growth:cellWidths[k]+width*(1-sstep(0.,.12,z)),std::min(.22*radius,std::min(width*std::pow(1-z,1.05)*(1+.13*std::sin(z*9+hand+local)),
                        .45*air+width*(1-sstep(0.,.20,z)))));
                    finger.left.push_back(finger.centre[k]+normal*w);finger.right.push_back(finger.centre[k]-normal*w);
                }
                if(lace) {
                    const V2 origin=finger.centre[9],direction=unit(finger.centre[13]-origin);
                    const V2 side={-direction.y,direction.x};
                    const double length=(15+22*ext)*growth;
                    for(int k=0;k<=16;++k) {
                        const double z=k/16.,a=(local%2?1:-1)*(1.3*z+.45*std::sin(z*Pi));
                        const V2 q=origin+direction*(length*z)+side*(length*.38*std::sin(a));
                        const double w=(.6+1.0*ext)*growth*std::pow(1-z,1.2);
                        finger.forkLeft.push_back(q+side*w);finger.forkRight.push_back(q-side*w);
                    }
                }
                finger.tip=finger.centre.back();
                const V2 direction=unit(finger.tip-finger.centre[23]);
                // Signed clockwise angle from the forward ROOT lip tangent.
                finger.angle=std::atan2(t.x*direction.y-t.y*direction.x,t.x*direction.x+t.y*direction.y);
                for(size_t k=1;k<finger.centre.size();++k)finger.length+=(finger.centre[k]-finger.centre[k-1]).len();
                crest.fingers.push_back(std::move(finger));
            }
        }
        crest.gapFraction=1-covered/lipArc.back();
        // Material lines advect from the lower face into the barrel. Wrapping
        // ribbons enter/leave invisibly at endpoints instead of phase popping.
        for(int row=0;row<16;++row) {
            const double phase=wrap(row/16.-s.flow*.14,1.);
            const double f=.035+.83*phase;
            crest.contourAlpha[row]=sstep(0.,.06,phase)*(1-sstep(.94,1.,phase));
            auto& line=crest.contours[row];line.reserve(49);
            for(int j=0;j<=48;++j) {
                const V2 outside=v[j],inside=v[6*Steps-j];
                line.push_back(map(lerp(outside,inside,f)));
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
        if(g>best){best=g;out.hero=int(out.crests.size());}
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
    auto body=f.surface;body.push_back({p.x1,p.waterline+p.depth});body.push_back({p.x0,p.waterline+p.depth});
    cv.linear(0,p.waterline,0,1290,{{0,p.body,1},{1,p.bottom,1}});fill(cv,body);
    for(int row=0;row<6;++row) {
        std::vector<V2> line;for(auto v:f.surface)line.push_back(v+V2(0,35+row*35));stripe(cv,line,1.1,p.lines,.24);
    }
    for(const auto& crest:f.crests) {
        const double top=1080-s.amplitude*crest.envelope;
        cv.linear(0,top,0,1100,{{0,p.body,1},{.38f,hex(0x274569),1},{.78f,p.bottom,1},{1,hex(0x121827),1}});
        fill(cv,crest.boundary);
        stripe(cv,crest.contours[13],22*crest.envelope,p.bottom,.42*crest.contourAlpha[13]);
        for(int row=0;row<16;++row) {
            const double pulse=.5+.5*std::sin(s.flow*8-row*.6);
            if(row%4==0)stripe(cv,crest.contours[row],13*crest.envelope,p.lines,(.12+.07*pulse)*crest.contourAlpha[row]);
            stripe(cv,crest.contours[row],(row%4==0?2:1)*crest.envelope,p.lines,(.30+.16*pulse)*crest.contourAlpha[row]);
        }
        // Paired underprint belongs to each hand; blue reaches the lip
        // between hands. No continuous rim and no separate periodic teeth.
        auto paired=[&](const std::vector<V2>& left,const std::vector<V2>& right,double alpha,bool blue) {
            std::vector<V2> l,r;
            for(size_t j=0;j<left.size();++j) {
                const V2 middle=(left[j]+right[j])*.5;
                l.push_back(middle+(left[j]-middle)*1.14+V2(-1.1,1.5)*crest.envelope);
                r.push_back(middle+(right[j]-middle)*1.14+V2(-1.1,1.5)*crest.envelope);
            }
            if(blue)ribbon(cv,l,r,p.bottom,alpha);else ribbon(cv,left,right,p.foam,alpha);
        };
        for(bool blue:{true,false}) {
            for(const auto& finger:crest.fingers)if(finger.length>.01)
                {paired(finger.left,finger.right,finger.opacity,blue);
                 if(!finger.forkLeft.empty())paired(finger.forkLeft,finger.forkRight,finger.opacity,blue);}
            for(const auto& hand:crest.clumps)paired(hand.edge,hand.inside,1,blue);
        }
        for(const auto& cap:crest.whitecaps)ribbon(cv,cap.edge,cap.inside,p.foam,.85);
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
void WaveTrainFoamMotionV2::advance(const Audio& a,const Score& score,WaveTrainPoseV2& pose_,double seconds,double dt) {
    if(!std::isfinite(dt)||dt<=0||dt>.25||!std::isfinite(seconds))return;
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
    // Bounded at 12 particles per event and 10 event groups/second. Work cannot
    // scale with amplitude or arbitrary event queue lengths.
    sprayClock_=std::max(0.,sprayClock_-dt);
    const auto shape=WaveTrainV2::profile(pose_,WaveTrainParametersV2{});
    if(shape.hero<0)return;const auto& hero=shape.crests[shape.hero];
    auto random=[&](){rng_^=rng_<<13;rng_^=rng_>>17;rng_^=rng_<<5;return (rng_&0xffffff)/double(0x1000000);};
    if((triggered||kicked) && sprayClock_<=0 && hero.stage>1.15) {
        sprayClock_=.10;
        for(int j=0;j<12;++j) {
            const int i=std::min(WaveTrainFingerCountV2-1,int(random()*WaveTrainFingerCountV2));
            if(!kicked && !onset[hero.fingers[i].band])continue;
            auto slot=std::find_if(pose_.droplets.begin(),pose_.droplets.end(),[](const auto& d){return d.life==0||d.age>=d.life;});
            if(slot==pose_.droplets.end())break;
            const auto& f=hero.fingers[i];const V2 motion=tipsReady_?(f.tip-previousTip_[i])*(1/dt):V2{};
            const V2 drift=unit(motion)*std::min(110.,motion.len());
            const V2 tangent=unit(f.tip-f.centre[f.centre.size()-3]);
            const bool crestFleck=j%4==0;
            *slot={crestFleck?f.root:f.tip,drift+tangent*(40+65*random())+V2(12+22*random(),-24-32*random()),
                0,1.4+1.3*random(),1.2+3.0*random(),++serial_};
        }
    }
    for(int i=0;i<WaveTrainFingerCountV2;++i)previousTip_[i]=hero.fingers[i].tip;
    tipsReady_=true;
}
void WaveTrainV2::draw(Ctx& c,const WaveTrainPoseV2& s,const WaveTrainParametersV2& p) {
    GpuProfile::Group group(c.gpu.profile,name);auto f=profile(s,p);
    Canvas& cv=c.canvas();paint(cv,f,s,p);c.gpu.over(cv);
}
}
