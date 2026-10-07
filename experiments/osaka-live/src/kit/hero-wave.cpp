#include "hero-wave.h"
#include "../canvas.h"
#include <cmath>
#include <algorithm>
#include <functional>
namespace Journey::Kit {
namespace {
const Col Ink=hex(0x121d3a),Deep=hex(0x111d48),Navy=hex(0x182b62),Mid=hex(0x2b5288),Blue=hex(0x3d6a9e),Finger=hex(0x3a64a2),
    Pale=hex(0x86a8c4),Light=hex(0xb3c8d4),Cream=hex(0xe6e0c4);
constexpr int Per=16;
struct Pose {
    std::array<V2,10> back,face;
    double grow[3],mantleFrom,mantleTo,mantleDepth,tongues,lipFrom,lipDepth,specks,stripeTo,lean;
};
// Authored on the 1920 x 1080 design page; foot first, lip tip last.
const std::array<Pose,7>& poses() {
    static const std::array<Pose,7> p{{
        {{{{-150,1095},{-60,960},{40,840},{150,745},{260,690},{360,668},{450,672},{520,692},{565,712},{585,726}}},
         {{{1000,1095},{860,1015},{760,935},{690,860},{640,800},{610,762},{595,742},{589,734},{586,729},{585,726}}},
         {.2,0,0},.3,.7,.12,0,.35,.05,.14,.8,0},
        {{{{-150,1095},{-50,900},{60,720},{180,585},{300,488},{420,438},{520,432},{600,452},{648,485},{668,522}}},
         {{{1010,1095},{840,1010},{740,905},{690,800},{668,712},{660,640},{662,590},{666,560},{668,538},{668,522}}},
         {.5,.3,0},.25,.7,.2,1,.42,.13,.43,.75,0},
        {{{{-150,1095},{-40,880},{80,650},{220,462},{380,322},{540,252},{680,240},{795,278},{860,335},{884,398}}},
         {{{1010,1095},{810,1015},{700,900},{650,770},{640,642},{660,532},{700,456},{755,412},{830,392},{884,398}}},
         {.85,.75,0},.22,.64,.3,1,.42,.13,.79,.6,0},
        {{{{-150,1095},{-40,880},{90,640},{230,450},{400,300},{580,212},{770,210},{915,300},{965,470},{862,600}}},
         {{{1020,1095},{800,1020},{680,900},{622,760},{612,630},{645,520},{720,452},{815,440},{880,500},{862,600}}},
         {1,1,1},.22,.64,.3,1,.42,.13,1,.6,0},
        {{{{-150,1095},{-30,880},{100,660},{260,480},{440,350},{640,280},{840,290},{1010,380},{1110,540},{1130,730}}},
         {{{1050,1095},{850,1030},{745,925},{700,800},{700,670},{740,565},{830,500},{940,510},{1030,600},{1130,730}}},
         {1.05,1,1.2},.22,.64,.3,1,.42,.13,1.2,.6,.15},
        {{{{-150,1095},{-20,890},{120,690},{290,530},{480,430},{680,390},{880,420},{1040,520},{1130,700},{1150,940}}},
         {{{1000,1095},{830,1015},{740,910},{710,800},{725,690},{785,600},{880,560},{990,605},{1075,740},{1150,940}}},
         {.9,.85,.5},.22,.64,.3,1,.42,.13,1.07,.6,.3},
        {{{{-150,1095},{-30,960},{130,860},{320,790},{520,750},{720,740},{900,760},{1060,810},{1170,880},{1240,960}}},
         {{{1180,1095},{1120,1065},{1090,1035},{1085,1005},{1105,985},{1140,972},{1175,966},{1205,963},{1225,961},{1240,960}}},
         {.55,.4,0},.3,.99,.14,0,.42,.13,.18,.85,.9},
    }};
    return p;
}
V2 unit(V2 v) { const double l=v.len();return l>1e-9?v*(1/l):V2(1,0); }
V2 catmull(V2 a,V2 b,V2 c,V2 d,double t) {
    const double t2=t*t,t3=t2*t;
    return (b*2+(c-a)*t+(a*2-b*5+c*4-d)*t2+(b*3-a-c*3+d)*t3)*.5;
}
// Centripetal Catmull-Rom through the points, `per` samples per span.
std::vector<V2> smooth(const V2* p,int n,int per) {
    std::vector<V2> out;out.reserve((n-1)*per+1);
    auto pt=[&](int i){ if(i<0)return p[0]*2-p[1]; if(i>=n)return p[n-1]*2-p[n-2]; return p[i]; };
    for(int i=0;i<n-1;++i) {
        const V2 p0=pt(i-1),p1=pt(i),p2=pt(i+1),p3=pt(i+2);
        auto tj=[](double ti,V2 a,V2 b){return ti+std::sqrt(std::max((b-a).len(),1e-3));};
        const double t0=0,t1=tj(t0,p0,p1),t2=tj(t1,p1,p2),t3=tj(t2,p2,p3);
        for(int k=0;k<per;++k) {
            const double t=t1+(t2-t1)*k/per;
            const V2 a1=p0*((t1-t)/(t1-t0))+p1*((t-t0)/(t1-t0));
            const V2 a2=p1*((t2-t)/(t2-t1))+p2*((t-t1)/(t2-t1));
            const V2 a3=p2*((t3-t)/(t3-t2))+p3*((t-t2)/(t3-t2));
            const V2 b1=a1*((t2-t)/(t2-t0))+a2*((t-t0)/(t2-t0));
            const V2 b2=a2*((t3-t)/(t3-t1))+a3*((t-t1)/(t3-t1));
            out.push_back(b1*((t2-t)/(t2-t1))+b2*((t-t1)/(t2-t1)));
        }
    }
    out.push_back(p[n-1]);
    return out;
}
void trace(Canvas& cv,const std::vector<V2>& v,bool close) {
    cv.newPath();if(v.size()<2)return;
    cv.moveTo(v[0].x,v[0].y);for(size_t i=1;i<v.size();++i)cv.lineTo(v[i].x,v[i].y);
    if(close)cv.closePath();
}
void fillPoly(Canvas& cv,const std::vector<V2>& v,Col c,double a=1) {
    if(v.size()<3||a<=.002)return;cv.color(c,a);trace(cv,v,true);cv.fill();
}
void strokeLine(Canvas& cv,const std::vector<V2>& v,Col c,double w,double a=1,bool close=false) {
    if(v.size()<2||a<=.002||w<=.05)return;cv.color(c,a);trace(cv,v,close);cv.stroke(w);
}
std::vector<V2> ribbon(const std::vector<V2>& centre,const std::vector<double>& widths) {
    std::vector<V2> l,r;const size_t n=centre.size();
    for(size_t i=0;i<n;++i) {
        const V2 t=unit(centre[std::min(i+1,n-1)]-centre[i>0?i-1:0]),nrm{-t.y,t.x};
        l.push_back(centre[i]+nrm*(widths[i]*.5));r.push_back(centre[i]-nrm*(widths[i]*.5));
    }
    l.insert(l.end(),r.rbegin(),r.rend());return l;
}
double rnd(unsigned seed,double key,double lo=0,double hi=1) { return lerp(lo,hi,hash2(seed*.731+key,seed*1.37+key*.113)); }

// Hokusai foam edge. One lobe authored in a local frame (u along the edge,
// v outward, in lobe units): over the crown, out to a hooked beak, back
// under the beak into the notch. Mapped along any curve, back to front.
const std::vector<V2>& lobe() {
    static const std::vector<V2> v=[]{const V2 p[]={{0,-.38},{0,0},{.06,.34},{.18,.6},{.38,.76},{.6,.78},{.82,.69},
        {1,.52},{1.14,.3},{1.22,.08},{1.21,-.1},{1.15,.04},{1.08,.2},{.98,.31},{.86,.37},{.77,.31},{.72,.16},{.72,-.38}};
        return smooth(p,18,5);}();
    return v;
}
const std::vector<V2>& shade() {
    static const std::vector<V2> v=[]{const V2 p[]={{.04,.04},{.09,.34},{.2,.56},{.34,.64},{.3,.48},{.22,.3},{.18,.04}};
        return smooth(p,7,5);}();
    return v;
}
constexpr double Advance=1.22;
struct Edge {
    std::vector<V2> p;std::vector<double> d;double length=0,side=1;
    Edge(std::vector<V2> pts,double s):p(std::move(pts)),side(s) {
        d.push_back(0);for(size_t i=1;i<p.size();++i)d.push_back(d.back()+(p[i]-p[i-1]).len());length=d.back();
    }
    void at(double x,V2& q,V2& n) const {
        x=std::clamp(x,0.,length);
        size_t i=std::upper_bound(d.begin(),d.end(),x)-d.begin();i=std::clamp<size_t>(i,1,p.size()-1)-1;
        const double f=(x-d[i])/std::max(d[i+1]-d[i],1e-9);
        q=lerp(p[i],p[i+1],f);const V2 t=unit(p[i+1]-p[i]);n=V2(t.y,-t.x)*side;
    }
};
struct Talons {
    double size=60,height=1,lean=.3,start=0,end=-1;Col fill=Cream,shadeCol=Pale;unsigned seed=1;bool rootLine=true;
    std::function<double(double)> scale;
};
void lobeShape(const Edge& e,const std::vector<V2>& local,double x0,double W,double H,double lean,std::vector<V2>& out) {
    out.clear();out.reserve(local.size());
    for(auto uv:local){V2 q,n;e.at(x0+(uv.x+lean*std::max(uv.y,0.))*W,q,n);out.push_back(q+n*(uv.y*H));}
}
void talons(Canvas& cv,const Edge& e,const Talons& t) {
    if(e.length<4)return;
    const double end=t.end<0?e.length:std::min(t.end,e.length);
    struct L {double x,W,H;};std::vector<L> lobes;double x=t.start;int k=0;
    while(x<end && k<200) {
        const double f=x/e.length,sc=t.scale?t.scale(f):1;
        const double W=std::max(6.,t.size*rnd(t.seed,k,.75,1.25)*sc),H=W*t.height*rnd(t.seed,k+500,.85,1.15);
        // The last lobe fades in as the edge grows, so nothing pops.
        lobes.push_back({x,W,H*sstep(0,.6,(end-x)/W)});x+=W*Advance;++k;
    }
    if(lobes.empty())return;
    {   // Smooth root band under the lobes so their bases never step.
        std::vector<V2> top,bottom;const int n=48;
        for(int j=0;j<=n;++j) {
            const double xx=lerp(t.start,end,j/double(n)),fr=j/double(n);
            double h=lobes.front().H;
            for(size_t i=0;i+1<lobes.size();++i){const double a=lobes[i].x+.5*lobes[i].W,b=lobes[i+1].x+.5*lobes[i+1].W;
                if(xx>=a&&xx<=b){h=lerp(lobes[i].H,lobes[i+1].H,(xx-a)/std::max(b-a,1e-6));break;}
                if(xx>b)h=lobes[i+1].H;}
            h*=std::min(1.,std::min(fr*5,(1-fr)*5));
            V2 q,nn;e.at(xx,q,nn);top.push_back(q+nn*(.2*h));bottom.push_back(q-nn*(.45*h));
        }
        auto band=top;band.insert(band.end(),bottom.rbegin(),bottom.rend());
        fillPoly(cv,band,t.fill);if(t.rootLine)strokeLine(cv,bottom,Ink,1.6,.8);
    }
    std::vector<V2> pts;
    for(auto i=lobes.rbegin();i!=lobes.rend();++i) {
        if(i->H<2)continue;
        lobeShape(e,lobe(),i->x,i->W,i->H,t.lean,pts);fillPoly(cv,pts,t.fill);
        std::vector<V2> sh;lobeShape(e,shade(),i->x,i->W,i->H,t.lean,sh);fillPoly(cv,sh,t.shadeCol,.95);
        std::vector<V2> vis(pts.begin()+5,pts.begin()+81);
        strokeLine(cv,vis,Ink,std::clamp(i->W*.04,1.4,2.6),.95);
    }
}
struct Body {
    const HeroWaveShapeV1& s;int M;
    explicit Body(const HeroWaveShapeV1& shape):s(shape),M(int(shape.face.size())) {}
    V2 at(double u,double t) const {
        const double x=std::clamp(u,0.,1.)*(M-1);const int i=std::min(int(x),M-2);const double f=x-i;
        return lerp(lerp(s.face[i],s.back[i],t),lerp(s.face[i+1],s.back[i+1],t),f);
    }
    template<class A,class B>
    std::vector<V2> band(A t0,B t1,double s0,double s1,int n) const {
        std::vector<V2> v;v.reserve(2*n+2);
        for(int j=0;j<=n;++j){const double u=lerp(s0,s1,j/double(n));v.push_back(at(u,t0(u)));}
        for(int j=n;j>=0;--j){const double u=lerp(s0,s1,j/double(n));v.push_back(at(u,t1(u)));}
        return v;
    }
};
auto constant(double v){return [v](double){return v;};}
}

HeroWaveShapeV1 HeroWaveV1::shape(const HeroWaveStateV1& st) {
    const auto& P=poses();const double ph=std::clamp(st.phase,0.,6.);
    const int i=std::min(5,int(ph));const double f=ph-i;
    const Pose &a=P[std::max(0,i-1)],&b=P[i],&c=P[i+1],&d=P[std::min(6,i+2)];
    HeroWaveShapeV1 s;
    std::array<V2,10> back,face;
    const double breathe=2+4*st.energy;
    for(int j=0;j<10;++j) {
        back[j]=catmull(a.back[j],b.back[j],c.back[j],d.back[j],f);
        face[j]=catmull(a.face[j],b.face[j],c.face[j],d.face[j],f);
        // Living water: a bass lift about the foot, a sway once every two
        // beats that leans the crest, and a slow breath through the body.
        const double w=j==0||j==9?0:1;
        for(V2* q:{&back[j],&face[j]}) {
            // Height above the sea leans forward on each (smoothed) bass push
            // and rocks gently on the beat sway; the size never changes here.
            const double up=std::max(0.,1095-q->y),reach=std::pow(up/700,1.5);
            q->x+=reach*(10*std::sin(Tau*st.sway)*(.4+.6*st.energy)+22*st.pulse);
            q->y+=reach*6*st.pulse;
            q->x+=w*breathe*noise1(st.seconds*.31+j*.53,17+(q==&face[j]));
            q->y+=w*breathe*.8*noise1(st.seconds*.27+j*.41,29+(q==&face[j]));
        }
        // Shared lip tip keeps the curl closed.
    }
    face[9]=back[9];
    for(auto* line:{&back,&face})for(auto& q:*line){q=V2(320+(q.x-320)*st.scale,1095-(1095-q.y)*st.scale);q.x+=st.shift;q.y+=st.sink*560;}
    s.back=smooth(back.data(),10,Per);s.face=smooth(face.data(),10,Per);
    for(int k=0;k<3;++k)s.grow[k]=std::max(0.,lerp(b.grow[k],c.grow[k],f));
    s.mantleFrom=lerp(b.mantleFrom,c.mantleFrom,f);s.mantleTo=lerp(b.mantleTo,c.mantleTo,f);
    s.mantleDepth=lerp(b.mantleDepth,c.mantleDepth,f);s.tongues=lerp(b.tongues,c.tongues,f);
    s.lipFrom=lerp(b.lipFrom,c.lipFrom,f);s.lipDepth=lerp(b.lipDepth,c.lipDepth,f);
    s.specks=lerp(b.specks,c.specks,f);s.stripeTo=lerp(b.stripeTo,c.stripeTo,f);s.lean=lerp(b.lean,c.lean,f);
    return s;
}
std::vector<V2> HeroWaveV1::outline(const HeroWaveShapeV1& s,int count) {
    std::vector<V2> loop=s.back;loop.insert(loop.end(),s.face.rbegin()+1,s.face.rend());
    std::vector<double> d{0};for(size_t i=1;i<loop.size();++i)d.push_back(d.back()+(loop[i]-loop[i-1]).len());
    d.push_back(d.back()+(loop.front()-loop.back()).len());loop.push_back(loop.front());
    std::vector<V2> out;out.reserve(count);
    for(int k=0;k<count;++k) {
        const double x=d.back()*k/count;
        size_t i=std::upper_bound(d.begin(),d.end(),x)-d.begin();i=std::clamp<size_t>(i,1,d.size()-1)-1;
        out.push_back(lerp(loop[i],loop[i+1],(x-d[i])/std::max(d[i+1]-d[i],1e-9)));
    }
    return out;
}
std::vector<V2> HeroWaveV1::outerLip(const HeroWaveShapeV1& s) {
    const int from=int(.42*(s.back.size()-1));return {s.back.begin()+from,s.back.end()};
}
double HeroWaveV1::right(const HeroWaveShapeV1& s) {
    double r=-1e9;for(auto q:s.back)r=std::max(r,q.x);for(auto q:s.face)r=std::max(r,q.x);
    return r+60*std::max(s.grow[0],s.grow[1]);
}
void HeroWaveV1::paint(Canvas& cv,const HeroWaveShapeV1& s,const HeroWaveStateV1& st) {
    if(s.back.size()<3)return;
    const Body w(s);const unsigned seed=st.seed;
    double top=1e9;for(auto q:s.back)top=std::min(top,q.y);
    std::vector<V2> body=s.back;body.insert(body.end(),s.face.rbegin(),s.face.rend());
    // Printed bokashi: the deep face darkens toward the lip, the back is lighter.
    cv.linear(0,top,0,top+std::max(300.,1080-top),{{0,Deep,1},{.45f,Navy,1},{1,Mid,1}});trace(cv,body,true);cv.fill();
    cv.linear(0,top,0,top+std::max(300.,1080-top),{{0,Mid,1},{1,Blue,1}});
    trace(cv,w.band(constant(s.backTone),constant(1.),0,1,90),true);cv.fill();
    // Hokusai's face: lighter blue fingers run up the dark face and over into
    // the curl, each keyed with an ink edge and a cream vein; bulges travel
    // up them with the music.
    for(int k=0;k<7;++k) {
        const double t=.035+.068*k,a=.22+.05*((k*3)%5),b=std::min(.985,.66+.1*std::clamp(st.phase,0.,3.)+.008*k);
        if(b<=a+.1)continue;
        const double weight=(16+8*rnd(seed,k+40))*(.85+.15*std::min(1.,s.specks));
        std::vector<V2> centre,edge;std::vector<double> width,vein;
        for(int j=0;j<=60;++j) {
            const double u=lerp(a,b,j/60.),f=j/60.;
            const double bulge=1+.22*std::sin(u*26-st.flow*3.2+k*1.3)*(.4+.6*st.energy);
            const double wd=weight*std::pow(std::sin(Pi*std::min(1.,f*1.15)),.55)*(1-.8*f)*(1-sstep(.85,1,f))*bulge;
            const V2 q=w.at(u,t+.012*std::sin(u*17+k));centre.push_back(q);width.push_back(std::max(0.,wd));vein.push_back(std::max(0.,wd*.16));
        }
        auto finger=ribbon(centre,width);fillPoly(cv,finger,k%3==1?Blue:Finger,.95);
        std::vector<V2> key(finger.begin(),finger.begin()+61);strokeLine(cv,key,Ink,1.5,.85);
        fillPoly(cv,ribbon(centre,vein),k%2?Light:Cream,.8);
    }
    // Fine veins between the fingers keep the face printed, not flat.
    static const double stripes[]={.07,.14,.21,.28,.35,.62,.72,.82};
    for(int k=0;k<8;++k) {
        const double t=stripes[k],s0=.1+.05*(k%3),s1=s.stripeTo-.025*k+.03*(k%2);
        if(s1<=s0+.05)continue;
        const double weight=3*(.6+.8*((k*37)%10)/10.),period=.34,phase=wrap(-st.flow*.22+k*.17,period);
        for(int dash=-1;dash<4;++dash) {
            const double a=std::max(s0,s0+phase+dash*period),b=std::min(s1,s0+phase+dash*period+.8*period);
            if(b-a<.03)continue;
            std::vector<V2> centre;std::vector<double> width;
            for(int j=0;j<=36;++j) {
                const double u=lerp(a,b,j/36.);
                centre.push_back(w.at(u,t+.015*std::sin(u*23+k)));
                width.push_back(weight*std::pow(std::max(0.,std::sin(Pi*(u-a)/(b-a))),.6)*std::min(1.,(b-a)/.12));
            }
            fillPoly(cv,ribbon(centre,width),k%2?Light:Pale,.7);
        }
    }
    // The lower back is printed with the sea's own swell lines.
    for(int k=0;k<6;++k) {
        const double t=.6+.065*k;std::vector<V2> line;
        for(int j=0;j<=50;++j)line.push_back(w.at(lerp(.02,.42-.03*k,j/50.),t));
        strokeLine(cv,line,Pale,1.3,.45);
    }
    // Cream foam draped over the back, blue talons lapping into it.
    const double m0=s.mantleFrom,m1=s.mantleTo,depth=s.mantleDepth;
    if(depth>.01) {
        auto inner=[&](double u){
            const double ramp=sstep(m0,m0+.1,u)*(1-sstep(m1-.12,m1,u));
            const double torn=.5+.5*std::sin(u*71+1.3)*std::sin(u*29);
            return 1-depth*ramp*(.75+.35*torn);
        };
        fillPoly(cv,w.band(inner,constant(1.),m0,m1,150),Cream);
        std::vector<V2> edge;for(int j=0;j<=150;++j){const double u=lerp(m0,m1,j/150.);edge.push_back(w.at(u,inner(u)));}
        strokeLine(cv,edge,Ink,1.6,.75);
        for(int k=0;k<5;++k) {
            const double fr=.18+.15*k;std::vector<V2> line;
            for(int j=0;j<=80;++j){const double u=lerp(m0+.06,m1-.08,j/80.);line.push_back(w.at(u,1-(1-inner(u))*fr));}
            strokeLine(cv,line,Pale,1.4,.6);
        }
        if(s.tongues>.02 && m1-.14>m0+.08) {
            std::vector<V2> line;for(int j=0;j<=100;++j){const double u=lerp(m0+.04,m1-.14,j/100.);line.push_back(w.at(u,inner(u)-.01));}
            Talons t;t.size=36;t.height=.72*s.tongues*(1+.1*st.pulse);t.lean=.35+.06*std::sin(Tau*st.sway);
            t.fill=Mid;t.seed=seed*7+3;t.scale=[](double f){return .6+.6*std::sin(Pi*f);};
            talons(cv,Edge(line,1),t);
        }
    }
    strokeLine(cv,s.back,Ink,2.6,.9);
    strokeLine(cv,std::vector<V2>(s.face.begin()+int(.08*s.face.size()),s.face.end()),Ink,2.2,.9);
    // Spray specks on the dark face drift up into the lip.
    const int specks=int(260*s.specks);
    for(int k=0;k<specks;++k) {
        const double u=std::pow(.35+wrap(rnd(seed,k*3+1)+st.flow*.06,.62),.6),t=rnd(seed,k*3+2,.02,.55);
        const double life=std::sin(Pi*clamp01((u-.53)/.46));
        cv.disc(w.at(u,t).x,w.at(u,t).y,rnd(seed,k*3+3,1.4,4.6)*(.5+.5*life),Cream,.92);
    }
    // Foam lip along the crest and curl, which the talons grow from.
    if(s.lipDepth>.01) {
        const double l0=s.lipFrom,lw=s.lipDepth;
        auto band=w.band([&](double u){return 1-lw*sstep(l0,l0+.08,u)*(1-.6*sstep(.9,1,u));},constant(1.),l0,1,110);
        fillPoly(cv,band,Cream);strokeLine(cv,band,Ink,2,1,true);
    }
    // Talons: the outer crest, a tier inside it, a fringe hanging into the hollow.
    // Talons cascade down the front of the curl in tiers, pale behind cream.
    struct Row {double t,s0,s1,size,height,lean,peak,width,side;int grow;bool pale;};
    static const Row rows[5]={{.99,.42,.995,60,1.25,.75,.78,.16,1,0,false},{.93,.5,.97,42,1.1,.7,.8,.14,1,1,false},
        {.85,.64,.97,34,1.05,.7,.86,.1,1,1,true},{.77,.74,.96,28,1,.65,.88,.08,1,1,false},{.035,.88,.99,30,.8,.3,-1,0,-1,2,false}};
    for(int r=0;r<5;++r) {
        const auto& row=rows[r];const double g=s.grow[row.grow];if(g<=.02)continue;
        std::vector<V2> line;for(int j=0;j<=120;++j)line.push_back(w.at(lerp(row.s0,row.s1,j/120.),row.t));
        Talons t;t.size=row.size;t.height=row.height*g*(1+.3*st.grasp*(r<4));if(row.pale)t.fill=Light;
        t.lean=row.lean+s.lean+.08*std::sin(Tau*st.sway+r)+.35*st.grasp*(row.side>0);t.seed=seed*13+r;
        if(row.peak>0)t.scale=[row](double f){const double u=lerp(row.s0,row.s1,f);return .55+.65*std::exp(-std::pow((u-row.peak)/row.width,2));};
        talons(cv,Edge(line,row.side),t);
    }
    // Loud hits throw sprays of drops up off the crest that arc forward
    // and fall back; each hit throws its own seeded spray.
    for(int h=0;h<4;++h) {
        const double age=st.hitAge[h];if(age>1.6||s.grow[0]<.15)continue;
        const unsigned key=unsigned(std::floor((st.seconds-age)*10));
        const int n=int(26*st.hitStrength[h]*std::min(1.,s.grow[0]));
        for(int k=0;k<n;++k) {
            const double u=rnd(key,k+40,.45,.98),a=age-rnd(key,k+41,0,.15);if(a<0)continue;
            const V2 q0=w.at(u,1.02),vel{rnd(key,k+42,40,260),rnd(key,k+43,-380,-140)};
            const V2 q=q0+vel*a+V2(0,420*a*a);
            cv.disc(q.x,q.y,rnd(key,k+44,1.6,4.4)*(1-.5*a/1.6),Cream,.95*(1-sstep(1.2,1.6,a)));
        }
    }
    // Spray: drops falling from the curling lip, snow thrown ahead of the crest.
    const double curl=sstep(2.4,3.4,st.phase)*(1-sstep(4.85,5.2,st.phase));
    if(curl>.01) {
        const V2 tip=s.tip();
        for(int k=0;k<int(70*curl);++k) {
            const double life=1.4,age=wrap(st.seconds+rnd(seed,k+900)*life,life),fade=1-age/life;
            const V2 q=tip+V2(rnd(seed,k+901,-60,40)*age,rnd(seed,k+902,0,90)*age+420*age*age)+V2(rnd(seed,k+903,-25,25),0);
            cv.disc(q.x,q.y,rnd(seed,k+904,1.4,3.6)*(.4+.6*fade),Cream,.95);
        }
        const V2 crest=w.at(.82,1);
        for(int k=0;k<int(80*curl*(.4+.6*st.energy));++k) {
            const double life=3.2,age=wrap(st.seconds+rnd(seed,k+700)*life,life),fade=std::sin(Pi*age/life);
            const V2 q=crest+V2(rnd(seed,k+701,60,240)*age,rnd(seed,k+702,-50,30)*age+40*age*age)+V2(rnd(seed,k+703,-40,40),rnd(seed,k+704,-30,30));
            cv.disc(q.x,q.y,rnd(seed,k+705,1.4,3.8)*fade,Cream,.95);
        }
    }
}
void HeroWaveV1::paintFoot(Canvas& cv,double seconds,double flow,double energy,double pulse) {
    // Two near swells run the width of the print in front of the wave's foot
    // and the landing, so both rise out of the sea. They roll with the flow
    // clock and jump together on every bass hit.
    static const Col top[2]={hex(0x355f8b),hex(0x2a5181)},deep[2]={hex(0x1f3d6b),hex(0x18305e)};
    for(int k=0;k<2;++k) {
        const double base=948+56*k-(6+10*k)*pulse,amp=(13+7*k)*(.7+.5*energy)*(1+.5*pulse);
        const double len=170+40*k,speed=flow*(.7+.25*k)+seconds*.12;
        std::vector<V2> edge;const int n=96;
        for(int j=0;j<=n;++j) {
            const double x=lerp(-40.,1960.,j/double(n));
            const double a=x/len-speed+k*1.9;
            edge.push_back({x,base-amp*(std::sin(a)+.25*std::sin(2.1*a+.7))});
        }
        auto body=edge;body.push_back({1960,1100});body.push_back({-40,1100});
        cv.linear(0,base-amp,0,base+120,{{0,top[k],1},{1,deep[k],1}});trace(cv,body,true);cv.fill();
        strokeLine(cv,edge,hex(0x143154),1.6,.75);
        // Cream rims on the highest crests, broken like the far rows.
        std::vector<V2> rim;
        auto flush=[&]{if(rim.size()>2){std::vector<double> wd;for(size_t i=0;i<rim.size();++i)wd.push_back((4+3*pulse)*std::pow(std::sin(Pi*i/(rim.size()-1)),.7));
            fillPoly(cv,ribbon(rim,wd),Cream,.92);}rim.clear();};
        for(int j=0;j<=n;++j){const double a=edge[j].x/len-speed+k*1.9;if(std::sin(a)>.7-.25*pulse)rim.push_back(edge[j]+V2(0,2));else flush();}
        flush();
        for(int r=1;r<=3;++r){std::vector<V2> line;for(auto q:edge)line.push_back(q+V2(0,20*r+5*k));strokeLine(cv,line,Pale,1.1,.28);}
    }
}
namespace {
void plume(Canvas& cv,V2 base,double ang,double length,double width,double curl,double size,unsigned seed) {
    if(length<8||width<2)return;
    const double side=curl>0?1:-1;V2 pos=base;double th=ang;std::vector<V2> spine;
    for(int k=0;k<=40;++k){const double u=k/40.;spine.push_back(pos);th+=curl/40*(.3+1.4*u);pos=pos+V2(std::cos(th),std::sin(th))*(length/40);}
    std::vector<V2> L,R;
    for(int k=0;k<=40;++k) {
        const V2 t=unit(spine[std::min(k+1,40)]-spine[std::max(k-1,0)]),n=V2(t.y,-t.x)*side;
        const double wd=width*std::pow(std::max(1-k/40.,0.),.6)*(1-.5*sstep(.8,1,k/40.));
        L.push_back(spine[k]+n*(wd*.5));R.push_back(spine[k]-n*(wd*.5));
    }
    auto loop=L;loop.insert(loop.end(),R.rbegin(),R.rend());
    fillPoly(cv,loop,Cream);strokeLine(cv,loop,Ink,2.2,1,true);
    strokeLine(cv,std::vector<V2>(R.begin()+4,R.begin()+30),Pale,std::max(3.,width*.12),.9);
    // Only the head of the sheet breaks into talons; drops peel off its tip.
    Talons t;t.size=size*1.15;t.height=1;t.lean=.4;t.seed=seed;t.start=length*.5;t.scale=[](double f){return .55+.6*sstep(.5,.75,f)*(1-.5*sstep(.9,1,f));};
    talons(cv,Edge(L,side),t);
    const V2 tip=spine.back(),dir=unit(spine[40]-spine[34]);
    for(int k=0;k<14;++k) {
        const double d=rnd(seed,k+800,10,90),off=rnd(seed,k+801,-30,30);
        const V2 q=tip+dir*d+V2(-dir.y,dir.x)*off;
        cv.disc(q.x,q.y,rnd(seed,k+802,1.6,4.2)*(1-d/120),Cream,.95);
    }
}
}
void HeroWaveV1::paintImpact(Canvas& cv,const HeroImpactV1& im,double) {
    if(!im.active())return;
    const double age=im.age,H=330*std::clamp(.7+.3*im.strength,.6,1.15),cx=im.at.x,cy=im.at.y;
    const unsigned seed=im.seed*31+5;
    // Rings spread across the sea and fade; lace lingers where the foam settles.
    const double ringFade=1-sstep(2.5,7,age);
    for(int k=0;k<3 && ringFade>.01;++k) {
        const double rx=H*(.45+1.7*easeOut(age/3.5))*(1-.2*k);
        for(int j=0;j<26;++j) {
            if(rnd(seed,j+40*k)<.3)continue;
            const double a0=Tau*j/26,a1=a0+Tau/26*rnd(seed,j+40*k+7,.5,.85);std::vector<V2> arc;
            for(int m=0;m<=7;++m){const double a=lerp(a0,a1,m/7.);arc.push_back({cx+std::cos(a)*rx,cy+18+std::sin(a)*rx*.17});}
            strokeLine(cv,arc,Cream,4.2-1.2*k,.8*ringFade*(1-.25*k)*sstep(0,.25,age));
        }
    }
    const double lace=sstep(.6,2,age)*(1-sstep(6,10,age));
    if(lace>.01) {
        static const double patches[][3]={{140,45,240},{370,-30,190},{-210,95,170},{550,-75,130},{210,-95,110}};
        int key=0;
        for(const auto& p:patches) {
            const double px=cx+p[0]+12*age,py=cy+p[1]+4*age,rx=p[2]*(.7+.3*sstep(1,6,age));
            for(int k=0;k<int(p[2]/9);++k,++key) {
                const double a=Tau*rnd(seed,key+300),r=rx*std::sqrt(rnd(seed,key+301));
                cv.color(Cream,lace*rnd(seed,key+302,.6,.95));cv.newPath();
                cv.ellipse(px+std::cos(a)*r,py+std::sin(a)*r*.18,rnd(seed,key+303,7,18),rnd(seed,key+304,2,4));cv.stroke(2.2);
            }
        }
    }
    // Whitewater heaves up in a dome, jets are thrown out of it, a crown
    // flares along the sea; then it spreads, slumps and sinks.
    const double grow=std::min(1.12,backOut(age/.55,1.3)),slump=sstep(.7,2.4,age),sink=H*1.3*easeIn(sstep(1.5,3.4,age));
    if(sink<H*1.25) {
        const double R=H*.95*(.55+.45*grow)*(1+.45*sstep(.5,2.4,age)),D=H*.62*grow*(1-.55*slump);
        const double rise=easeOut(age/.4),fall=sstep(.9,2.6,age);
        static const double jets[6][3]={{-.95,.75,-1},{-.45,1.05,-1},{-.08,1.35,-1},{.22,1.2,1},{.62,.95,1},{1.05,.7,1}};
        for(int k=0;k<6;++k) {
            const double off=jets[k][0],sign=jets[k][2];
            const double a=-Pi/2+off*.85*(1+.3*fall)+rnd(seed,k+60,-.1,.1);
            const double curl=sign*(.9+1.3*std::abs(off))*rnd(seed,k+61,.8,1.2)*(1+.9*fall);
            const double L=H*jets[k][1]*rnd(seed,k+62,.85,1.1)*rise*(1-.55*fall)*(.8+.2*im.strength);
            plume(cv,{cx+off*R*.45,cy-D*.55+sink},a,L,H*.16*(1.15-.35*std::abs(off))*(.5+.5*rise),curl,H*.12,seed+k);
        }
        if(D>4) {
            auto arc=[&](double u,double f){const double c=std::cos(u);return V2(cx+R*f*std::sin(u),cy+sink+14*(f<1)-D*f*std::pow(std::max(c,0.),.8));};
            std::vector<V2> right,left,body;
            for(int j=0;j<60;++j){const double u=Pi/2*j/59.;right.push_back(arc(u,1));left.push_back(arc(-u,1));}
            body.assign(left.rbegin(),left.rend());body.insert(body.end(),right.begin()+1,right.end());
            for(int j=0;j<=16;++j){const double u=Pi*j/16.;body.push_back({cx+R*std::cos(u),cy+sink+10+.16*R*std::sin(u)});}
            std::vector<V2> under;for(auto q:body)under.push_back(q+V2(0,6));
            fillPoly(cv,under,Pale);strokeLine(cv,under,Pale,10,1,true);
            fillPoly(cv,body,Cream);strokeLine(cv,body,Ink,2.2,1,true);
            Talons t;t.rootLine=false;t.size=H*.16;t.height=1.05*grow;t.lean=.4;t.scale=[](double f){return 1.2-.5*f;};
            t.seed=seed+20;talons(cv,Edge(right,1),t);t.seed=seed+21;talons(cv,Edge(left,-1),t);
            for(int k=0;k<2;++k) {
                const double f=k?.46:.72;std::vector<V2> rr,ll;
                for(int j=0;j<40;++j){const double u=1.25*j/39.;rr.push_back(arc(u,f));ll.push_back(arc(-u,f));}
                Talons ti;ti.rootLine=false;ti.size=H*(.13-.03*k);ti.height=.95*grow;ti.lean=.4;ti.scale=[](double f){return 1.1-.4*f;};
                ti.seed=seed+30+k;talons(cv,Edge(rr,1),ti);ti.seed=seed+40+k;talons(cv,Edge(ll,-1),ti);
            }
        }
        const double spread=1+.5*sstep(0,2.5,age),crown=sstep(0,.3,age)*(1-slump);
        for(int side=-1;side<=1;side+=2) {
            std::vector<V2> arc;
            for(int j=0;j<=59;++j){const double u=j/59.;arc.push_back({cx+side*H*(.75+.8*u)*spread,cy+8+sink*.6-H*.22*u*u*crown});}
            if(side<0)std::reverse(arc.begin(),arc.end());
            Talons t;t.size=H*.12;t.height=1.1*crown;t.lean=.45*side;t.seed=seed+100+side;
            t.scale=[](double f){return .35+.9*std::sin(Pi*f);};
            talons(cv,Edge(arc,1),t);
        }
    }
    // Spray thrown from the landing on ballistic arcs, gone into the sea.
    for(int k=0;k<300;++k) {
        const double born=rnd(seed,k+500,0,.5),a=age-born;if(a<0||a>2.6)continue;
        const double ang=-Pi/2+rnd(seed,k+501,-1.25,1.25),v=rnd(seed,k+502,300,950)*(.7+.3*im.strength);
        const V2 q{cx+std::cos(ang)*v*a+rnd(seed,k+503,-40,40),cy-30+std::sin(ang)*v*a+520*a*a};
        if(q.y>cy+30)continue;
        cv.disc(q.x,q.y,rnd(seed,k+504,1.4,4.2)*(1-.35*a/2.6),Cream,.95);
    }
}
}
