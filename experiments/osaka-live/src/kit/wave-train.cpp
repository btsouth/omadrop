#include "wave-train.h"
#include "../world.h"
#include <sstream>
namespace Journey::Kit {
void CriticalSpringV2::advance(double target,double omega,double dt) {
    const double error=value-target,b=velocity+omega*error,e=std::exp(-omega*dt);
    value=target+(error+b*dt)*e;velocity=(velocity-omega*b*dt)*e;
}
WaveTrainMotionV2::WaveTrainMotionV2() {
    amplitude_.value=pose_.amplitude;speed_.value=pose_.phaseSpeed;
}
void WaveTrainMotionV2::advance(const Audio& a,const Score& score,double seconds,double dt,const WaveTrainParametersV2& params) {
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
    stage_.advance(std::max(5*sstep(.25,.95,energy),params.surgeEnabled?5*sstep(0,.65,a.surge):0),.8*beat,dt);
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
    pose_.amplitude=amplitude_.value*params.heightScale;pose_.baseWidth=params.width;pose_.stage=stage_.value;pose_.phaseSpeed=speed_.value;
    pose_.lean=lean_.value;pose_.lipThrow=throw_.value;pose_.lipStage=lip_;
    pose_.seconds=seconds;pose_.energy=energy;pose_.tempo=tempo;
    pose_.distance+=(oldSpeed+speed_.value)*.5*dt*params.travelScale;pose_.flow+=(.065+.16*energy)*dt;
    foam_.advance(a,score,pose_,seconds,dt,params);
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
    return std::max(p.groupFloor,.08+.92*group/peak)*(1-.62*sstep(1400,2200,a));
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
        const double height=std::clamp(s.amplitude,450.*p.heightScale,850.*p.heightScale)*g;
        auto map=[&](V2 q) {
            const double top=std::max(0.,q.y),tip=sstep(.52,.78,q.x)*sstep(.30,.65,top);
            return V2(left+width*q.x+s.lean*height*top+s.lipThrow*tip*g*sstep(2,5,crest.stage),p.baseY-height*q.y);
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
        constexpr double along[Clusters]={.05,.135,.215,.30,.385,.47,.555,.64,.725,.805,.88};
        constexpr double size[Clusters]={.62,.80,.95,1.05,1.15,.92,1.0,.86,.74,.62,.52};
        std::array<double,Clusters> centreArc{},gate{};
        for(int c=0;c<Clusters;++c) {
            const double u=along[c]+.012*std::sin(c*2.7+1.3);centreArc[c]=u*lipLength;
            // The crest top foams first; shoulder and curl tip follow with stage.
            const double d=std::min(1.,std::abs(u-.33)/.6);
            gate[c]=sstep(1.15+1.3*d,2.5+1.7*d,crest.stage)*g;
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
            const double depth=(2+12*near)*(.70+.65*band)*(1+.40*fbm1(u*6+s.seconds*.05,10))*growth;
            const double rim=(2+4*(.5+.5*noise1(u*20+s.seconds*.11,14)))*growth;
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
        for(int k=0;k<7;++k) {
            const double index=indexAt((.835+.022*k+.006*std::sin(k*3.1))*lipLength);
            const V2 root=sample(crest.outerLip,index),t=tangentAt(index);
            const double sway=.18*noise1(s.seconds*.5+k*1.3,451);
            crest.tangle.push_back(strand(root,std::atan2(t.y,t.x)-(.2+.9*hash2(k,62))+sway,(26+40*hash2(k,63))*(.55+.75*highs)*tipGate,
                (1.4+1.6*hash2(k,64))*tipGate,(k%2?1:-1)*(1.6+1.6*hash2(k,65)),1.2,16));
        }
        for(int k=0;k<6;++k) {
            const double phase=wrap(s.flow*2.4+hash2(k,71),1.);
            const V2 start=sample(crest.outerLip,indexAt((.70+.28*hash2(k,72))*lipLength))+V2(-6-14*hash2(k,73),4);
            const V2 at=start+V2(-(14+20*hash2(k,74))*phase,(40+120*hash2(k,75))*phase*phase+20*phase)*g;
            crest.falling.push_back(strand(at,Pi*.5+.9*(hash2(k,76)-.5)+.5*std::sin(phase*4+k),(10+14*hash2(k,77))*tipGate*(.6+.6*highs),
                (1.6+2*hash2(k,78))*tipGate,(k%2?1:-1)*1.8,1.2,8,sstep(0.,.10,phase)*(1-sstep(.65,1.,phase))));
        }
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
        double selection=g;
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
    auto body=f.surface;body.push_back({p.x1,p.waterline+p.depth});body.push_back({p.x0,p.waterline+p.depth});
    cv.linear(0,p.waterline,0,1290,{{0,p.body,1},{1,p.bottom,1}});fill(cv,body);
    for(int row=0;row<6;++row) {
        std::vector<V2> line;for(auto v:f.surface)line.push_back(v+V2(0,35+row*35));stripe(cv,line,1.1,p.lines,.24);
    }
    for(const auto& crest:f.crests) {
        const double top=p.baseY-s.amplitude*crest.envelope;
        cv.linear(0,top,0,1100,{{0,p.body,1},{.38f,hex(0x274569),1},{.78f,p.bottom,1},{1,hex(0x121827),1}});
        fill(cv,crest.boundary);
        stripe(cv,crest.contours[13],22*crest.envelope,p.bottom,.42*crest.contourAlpha[13]);
        for(int row=0;row<16;++row) {
            const double pulse=.5+.5*std::sin(s.flow*8-row*.6);
            if(row%4==0)stripe(cv,crest.contours[row],13*crest.envelope,p.lines,(.12+.07*pulse)*crest.contourAlpha[row]);
            stripe(cv,crest.contours[row],(row%4==0?2:1)*crest.envelope,p.lines,(.30+.16*pulse)*crest.contourAlpha[row]);
        }
        // Sheet and lace first, then each stroke's offset blue underprint and
        // cream in lip order, so later claws overlap earlier ones like a print.
        if(crest.foamRim.size()>1)ribbon(cv,crest.foamRim,crest.foamInside,p.foam);
        for(const auto& l:crest.lace)if(l.alpha>0)ribbon(cv,l.left,l.right,p.bottom,.95*l.alpha);
        auto print=[&](const WaveTrainProfileV2::Strand& st,double alpha) {
            const double root=.5*(st.left[0]-st.right[0]).len();
            if(root<=0||alpha<=0)return;
            const double k=std::clamp(root/6,.3,1.6);const V2 offset=V2(-3.5,-4.5)*k;
            std::vector<V2> l,r;l.reserve(st.centre.size());r.reserve(st.centre.size());
            for(size_t j=0;j<st.centre.size();++j) {
                const V2 half=(st.left[j]-st.right[j])*.5,n=unit(half)*(half.len()+2.0*k);
                l.push_back(st.centre[j]+offset+n);r.push_back(st.centre[j]+offset-n);
            }
            ribbon(cv,l,r,p.bottom,.9*alpha);ribbon(cv,st.left,st.right,p.foam,alpha);
        };
        for(const auto& f:crest.fingers)if(f.length>.5) {
            ribbon(cv,f.shadow.left,f.shadow.right,p.underprint,f.opacity);
            ribbon(cv,f.left,f.right,p.foam,f.opacity);
            for(const auto& twig:f.twigs)print(twig,f.opacity);
        }
        for(const auto& st:crest.tangle)print(st,st.alpha);
        for(const auto& st:crest.falling)print(st,st.alpha);
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
void WaveTrainFoamMotionV2::advance(const Audio& a,const Score& score,WaveTrainPoseV2& pose_,double seconds,double dt,const WaveTrainParametersV2& params) {
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
    const auto shape=WaveTrainV2::profile(pose_,params);
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
        && old.distance==pose.distance && old.flow==pose.flow && old.lean==pose.lean && old.lipThrow==pose.lipThrow && old.energy==pose.energy && old.bands==pose.bands;
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
