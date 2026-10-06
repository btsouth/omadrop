// Body, material curves and forked Hokusai talons ported from Journey's
// studyWave. The standing version never advances into the film's plunge.
#include "great-wave.h"
#include <array>
namespace Journey::Kit {
namespace {
struct Cubic {
    V2 a,b,c,d;
    V2 at(double t) const { const double v=1-t;return a*(v*v*v)+b*(3*v*v*t)+c*(3*v*t*t)+d*(t*t*t); }
    V2 tangent(double t) const { const double v=1-t;return (b-a)*(3*v*v)+(c-b)*(6*v*t)+(d-c)*(3*t*t); }
};
V2 unit(V2 p){return p*(1/std::max(.001,p.len()));}
void polygon(Canvas& cv,const std::vector<V2>& p,Col col){
    if(p.empty())return;
    cv.color(col);cv.moveTo(p[0].x,p[0].y);
    for(std::size_t i=1;i<p.size();++i)cv.lineTo(p[i].x,p[i].y);
    cv.closePath();cv.fill();
}
// Summing continuous attack/release pulses avoids replacing a still-live
// event envelope when the next beat arrives. Zero at discovery and expiry.
double pulse(const std::deque<Event>& events,double t,double attack,double life) {
    double sum=0;
    for(const auto& e:events){const double age=t-e.t;
        if(age>=0 && age<life)sum+=e.strength*sstep(0,attack,age)*std::exp(-age*3)* (1-sstep(life-.3,life,age));}
    return 1-std::exp(-sum);
}
void tendril(Canvas& cv,V2 root,double angle,double length,double width,double turn,Col color){
    std::vector<V2> left,right;V2 q=root;
    for(int j=0;j<=20;++j){const double u=j/20.,a=angle+turn*std::pow(u,1.7);
        const V2 n(-std::sin(a),std::cos(a));const double w=width*std::pow(1-u,1.1)+.22;
        left.push_back(q+n*w);right.push_back(q-n*w);q=q+V2(std::cos(a),std::sin(a))*(length/20);}
    for(auto i=right.rbegin();i!=right.rend();++i)left.push_back(*i);
    polygon(cv,left,color);
}
}
GreatWavePoseV1 GreatWaveV1::pose(const Ctx& c,const GreatWaveParametersV1& p){
    // Score's existing 1.6 s causal envelopes ease the entire body over
    // several beats. Never bind the mass to instantaneous kicks or onsets.
    double drive=0,sw=0;
    if(c.score){
        drive=p.lowGain*(.65*c.score->bandBody[2][0]+.35*c.score->bandBody[2][1]);
        sw=p.swellGain*pulse(c.score->surges,c.t,.75,3);
    }
    const double energy=clamp01(drive+sw);
    GreatWavePoseV1 out;out.rise=p.maxRise*sstep(0,1,energy);out.height=p.baseHeight+out.rise;
    out.curl=p.curlAmount*(.32+.68*sstep(0,1,energy));
    if(c.score)out.flick=p.kickGain*pulse(c.score->bassHits,c.t,.09,1.6)+p.onsetGain*pulse(c.score->onsets,c.t,.07,1.3);
    return out;
}
V2 GreatWaveV1::map(V2 q,const GreatWavePoseV1& s,const GreatWaveParametersV1& p){
    const double crest=1-sstep(580,1080,q.y),lip=sstep(540,1040,q.x)*crest;
    const double x=(q.x+75*s.curl*lip)*p.width/1440.;
    return {p.x+(p.anchorRight?-x:x),p.y+(q.y-1080)*s.height/1000.};
}
QRectF GreatWaveV1::responseArea(const GreatWaveParametersV1& p){
    return QRectF(p.anchorRight?p.x-p.width*.88:p.x,p.y-p.baseHeight-p.maxRise-100,p.width*.88,p.baseHeight+p.maxRise-20);
}
void GreatWaveV1::paint(Canvas& body,Canvas& flow,Canvas& foam,const Ctx& c,const GreatWaveParametersV1& p){
    const auto s=pose(c,p);
    auto mapped=[&](V2 q){return map(q,s,p);};
    Canvas* target=&body;
    auto move=[&](double x,double y){const V2 q=mapped({x,y});target->moveTo(q.x,q.y);};
    auto curve=[&](double ax,double ay,double bx,double by,double dx,double dy){
        const V2 a=mapped({ax,ay}),b=mapped({bx,by}),d=mapped({dx,dy});target->curveTo(a.x,a.y,b.x,b.y,d.x,d.y);};
    body.linear(p.x,p.y-s.height,p.x,p.y,{{0,p.body,1},{.55f,mix(p.body,p.bottom,.55),1},{1,p.bottom,1}});
    move(-180,1100);curve(-240,720,-100,275,220,130);curve(475,-25,825,65,990,245);
    curve(1145,395,1120,515,1000,560);curve(1090,465,985,360,850,390);
    curve(660,410,655,590,850,735);curve(1040,880,1230,875,1440,1040);
    curve(1040,1170,250,1190,-180,1100);body.closePath();body.fill();
    target=&flow;
    auto stream=[&](double f){
        move(-155+f*1540,1120);curve(-230+f*1170,825,-145+f*700,245+f*325,240+f*475,158+f*306);
        curve(480+f*200,-1+f*456,840-f*58,81+f*350,993-f*88,280+f*244);
        curve(1090-f*134,386+f*182,1096-f*210,485+f*104,1017-f*190,536+f*106);
    };
    const double scale=p.width/1440.;
    for(int band=0;band<6;++band){
        const double body=c.score?c.score->bandBody[1][band]:0;
        const double f=.09+band*.13;
        flow.color(mix(p.body,p.lines,.42+.07*band),.20+.55*body);stream(f);flow.stroke((26+14*body)*scale);
        flow.color(p.lines,.23+.35*body);stream(f+.015);flow.stroke(2.2*scale);
    }
    for(int i=0;i<18;++i){const double f=.045+(i+.13*hash2(i,p.seed+17))*.044;
        flow.color(i%6==0?p.foam:p.lines,i%6==0?.26:.34);stream(f);flow.stroke((i%6==0?1.8:.8)*scale);}
    const std::array<Cubic,3> crown={{{{-110,420},{-45,270},{110,165},{220,130}},
        {{220,130},{475,-25},{825,65},{990,245}},{{990,245},{1145,395},{1120,515},{1000,560}}}};
    for(int seg=0;seg<3;++seg){const auto& q=crown[seg];std::vector<V2> rim,inside;
        for(int j=0;j<=48;++j){const double u=j/48.;V2 n=unit(q.tangent(u));n={-n.y,n.x};
            const double w=(seg==0?38:68)*(1+.23*fbm1(u*17,seg+p.seed));
            rim.push_back(mapped(q.at(u)-n*(5+4*noise1(u*20,seg))));inside.push_back(mapped(q.at(u)+n*w));}
        for(auto i=inside.rbegin();i!=inside.rend();++i)rim.push_back(*i);
        polygon(foam,rim,p.foam);
    }
    struct Talon{V2 root;double angle,length,width,turn;};std::vector<Talon> talons;
    for(int i=0;i<p.clawCount;++i){
        const double f=(i+.5)/p.clawCount;
        const int seg=f<.18?0:(f<.82?1:2);const double u=seg==0?f/.18:(seg==1?(f-.18)/.64:(f-.82)/.18);
        const auto& q=crown[seg];const V2 a=mapped(q.at(std::max(0.,u-.004))),b=mapped(q.at(std::min(1.,u+.004)));
        const double angle=std::atan2(b.y-a.y,b.x-a.x);
        V2 n=unit(q.tangent(u));n={-n.y,n.x};
        // Lip claws live on the outer rim, never on the hollow's inward
        // sheet. Length is capped by each crest cell's arc spacing; forks
        // stay in that cell even at maximum rise and full beat extension.
        const double spacing=(b-a).len()/.008/(p.clawCount*(seg==0?.18:(seg==1?.64:.18)));
        const double requested=(seg==0?58:(seg==1?100:48))*scale*p.clawSize*(.8+.35*hash2(i,p.seed+30));
        const double length=std::min(requested,spacing*.95/(1+.4*(p.kickGain+p.onsetGain)))*(1+.4*s.flick);
        const double width=std::min((seg==2?7:12)*scale*p.clawSize,length*.19);
        talons.push_back({mapped(q.at(u)+n*(seg==2?-3:(seg==0?38:68)*(1+.23*fbm1(u*17,seg+p.seed))*.90)),
            angle+(p.anchorRight?-1:1)*(seg==2?-.58:.45),length,width,(p.anchorRight?-1:1)*(seg==2?.95:1.5)});
    }
    for(int pass=0;pass<2;++pass)for(const auto& t:talons){
        const Col col=pass?p.foam:p.underprint;const double extra=pass?0:2*scale;
        tendril(foam,t.root,t.angle,t.length+extra,t.width+extra,t.turn,col);
        for(int fork=0;fork<2;++fork){const double at=.43+.16*fork;V2 root=t.root;
            for(int j=0;j<int(at*20);++j){const double a=t.angle+t.turn*std::pow(j/20.,1.7);root=root+V2(std::cos(a),std::sin(a))*(t.length/20);}
            const double a=t.angle+t.turn*std::pow(at,1.7)+(p.anchorRight?-1:1)*(fork? .48:-.42);
            tendril(foam,root,a,t.length*(fork?.27:.34)+extra,t.width*(fork?.35:.48)+extra*.5,t.turn*.9,col);
        }
    }
    // Persistent specks breathe and flick in place. No modulo birth/death
    // clock or full-frame white flash can pop at a particle cycle boundary.
    for(int i=0;i<30;++i){const double u=.12+.74*hash2(i,p.seed+64);const V2 source=mapped(crown[1].at(u));
        const double phase=c.t*(.55+.15*hash2(i,p.seed+67))+Tau*hash2(i,p.seed+68);
        const double dist=12+55*hash2(i,p.seed+69)+12*std::sin(phase)+45*s.flick;
        const double x=source.x+(p.anchorRight?1:-1)*dist*.72,y=source.y-dist*.65;
        foam.color(p.foam,.16+.18*(.5+.5*std::sin(phase))+.7*s.flick);
        foam.ellipse(x,y,(1+1.5*hash2(i,p.seed+65))*scale,(1.2+2*hash2(i,p.seed+66))*scale);foam.fill();
    }
}
void GreatWaveV1::draw(Ctx& c,const GreatWaveParametersV1& p){
    GpuProfile::Group group(c.gpu.profile,name);
    Canvas& body=c.canvas();Canvas& flow=c.canvas();Canvas& foam=c.canvas();paint(body,flow,foam,c,p);
    const int mask=c.gpu.layer(body),details=c.gpu.layer(flow),crest=c.gpu.layer(foam);
    // Clip material contours to the authored body, then composite the foam.
    // One composite pass also feathers the foot into the shared printed sea.
    Program& shader=c.gpu.effect("greatWaveCompositeV1",R"(
        uniform sampler2D u_body,u_flow,u_foam;uniform float u_y;
        void main(){
            vec4 b=texture(u_body,v_uv),f=texture(u_flow,v_uv)*b.a,c=texture(u_foam,v_uv);
            vec4 water=f+b*(1.0-f.a);
            o=(c+water*(1.0-c.a))*(1.0-smoothstep(u_y-100.0,u_y+10.0,design().y));
        }
    )");
    c.gpu.pass(shader,Blend::Over,[&](Program& sh){
        c.gpu.bindTexture(0,mask,sh,"u_body");c.gpu.bindTexture(1,details,sh,"u_flow");
        c.gpu.bindTexture(2,crest,sh,"u_foam");sh.set("u_y",float(p.y));
    });
}
}
