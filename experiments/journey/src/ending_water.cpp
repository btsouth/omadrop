// Animated water chapter. Geometry, flow and particles share a deterministic
// scene clock and measured Score; no frame history or baked artwork.
#include "parts.h"
#include "rig.h"
#include "ending_road.h"
#include <array>
#include <vector>

namespace Journey {
namespace {
const Col ink=hex(0x121827),navy=hex(0x192b46),blue=hex(0x315985),teal=hex(0x638da2);
const Col cream=hex(0xe6ddbf),gold=hex(0xe3b87a),wood=hex(0xa77b4b),violet=hex(0x75638e);
struct Cubic {
    V2 a,b,c,d;
    V2 at(double t) const {
        const double v=1-t;
        return a*(v*v*v)+b*(3*v*v*t)+c*(3*v*t*t)+d*(t*t*t);
    }
    V2 tangent(double t) const {
        const double v=1-t;
        return (b-a)*(3*v*v)+(c-b)*(6*v*t)+(d-c)*(3*t*t);
    }
};
V2 unit(V2 p) {return p*(1/std::max(.001,p.len()));}
void ellipseFill(Canvas& cv,double x,double y,double rx,double ry,Col col,double alpha) {
    cv.color(col,alpha);cv.ellipse(x,y,rx,ry);cv.fill();
}
void polyfill(Canvas& cv,const std::vector<V2>& p,Col col,double alpha=1) {
    if(p.empty())return;
    cv.color(col,alpha);cv.moveTo(p[0].x,p[0].y);
    for(std::size_t i=1;i<p.size();++i)cv.lineTo(p[i].x,p[i].y);
    cv.closePath();cv.fill();
}

void calmSea(Ctx& c,bool aftermath) {
    const Col navy=hex(0x192b46),blue=hex(0x315985),teal=hex(0x638da2);
    Canvas& cv=c.canvas();
    cv.linear(0,610,0,1080,{{0,hex(0x536786),1},{.42f,blue,1},{1,navy,1}});
    cv.rect(0,612,1920,468);cv.fill();
    const double age=c.t-(kanagawaBreak(c).t+.60);
    for(int row=0;row<65;++row) {
        const double z=row/64.0,y=616+456*std::pow(z,1.72);
        const double amp=2+22*z*z;
        std::vector<V2> p;
        const double energy=c.score ? (c.score->integral(row%6,c.t)-c.score->integral(row%6,c.t-.8))/.8 : 0;
        const double drift=c.score ? .95*c.score->integral(row%6,c.t) : 0;
        for(int j=0;j<=110;++j) {
            const double x=j*18.0;
            const double swell=aftermath ? 35*z*std::exp(-std::max(0.0,age)*.35)*std::sin(x*.010-z*12-age*3-drift*.35) : 15*z*energy*std::sin(x*.006-c.t*.7+row);

            p.push_back({x,y+amp*std::sin(x*.0055+row*.82-c.t*.55-drift)+swell+2*noise1(x*.012,row)});
        }
        cv.polyline(p,row%7==0?1.65:.65,mix(teal,cream,.18),.11+.17*z*(row%3==0));
    }
    for(int i=0;i<115;++i) {
        const double y=620+350*hash2(i,5),x=1300+(hash2(i,6)-.5)*140*(y-600)/350;
        const double w=4+28*hash2(i,7);
        cv.line(x-w,y,x+w,y,1+hash2(i,8),i%3?gold:hex(0xc94a45),.25+.35*hash2(i,9));
    }
    c.gpu.over(cv,1,0,float(sstep(48,50,c.t)));
}


// One continuous support field supplies both the printed crests and boats.
// Depth controls scale; integrated band energy supplies the existing flow.
double printDrift(const Ctx& c,double z) {
    if(!c.score)return 0;
    const double role=(1-z)*5;
    const int lo=std::min(4,int(role));
    return .95*lerp(c.score->integral(lo,c.t),c.score->integral(lo+1,c.t),role-lo);
}
double printLift(const Ctx& c,double x,double y) {
    const double z=clamp01((y-614)/500),age=c.t-(kanagawaBreak(c).t+.60);
    const double drift=printDrift(c,z);
    const double pull=(sstep(54,65.3,c.t)+3*sstep(61.5,65.3,c.t))*(1-sstep(65.3,67.3,c.t));
    const double a=x/(55+115*z)-c.t*.52-drift+z*6+pull*1.1;
    const double churn=age>0?22*z*std::exp(-age*.30)*std::sin(x*.014-age*3.3+z*9):0;
    return (7+83*std::pow(z,1.15))*(std::sin(a)+.23*std::sin(a*2.1+.6))+churn;
}
void printSea(Ctx& c,double opacity) {
    const Col teal=hex(0x7397a4),blue=hex(0x285579);
    Canvas& cv=c.canvas();
    const double age=c.t-(kanagawaBreak(c).t+.60);
    for(int row=0;row<12;++row) {
        const double z=row/11.0,y=614+500*std::pow(z,1.45);
        auto top=[&](double x){return y+printLift(c,x,y);};
        std::vector<V2> crest;
        for(int j=0;j<=160;++j){double x=-30+j*12.5;crest.push_back({x,top(x)});}
        cv.color(mix(hex(0x376582),hex(0x102657),z*.9));
        cv.moveTo(-30,1140);for(V2 q:crest)cv.lineTo(q.x,q.y);cv.lineTo(2000,1140);cv.closePath();cv.fill();
        cv.polyline(crest,1.2+1.5*z,hex(0x143154),.9);
        // Broken foam is rooted on the high side of a moving swell.
        const double drift=printDrift(c,clamp01((y-614)/500));
        const double depth=clamp01((y-614)/500),waveScale=55+115*depth;
        const double pull=(sstep(54,65.3,c.t)+3*sstep(61.5,65.3,c.t))*(1-sstep(65.3,67.3,c.t));
        for(int k=-8;k<12;++k) {
            const double middle=(-Pi/2+2*Pi*k+c.t*.52+drift-depth*6-pull*1.1)*waveScale;
            if(middle<-200 || middle>2100)continue;
            // Each travelling crest keeps its own identity: some stay bare,
            // the rest break, swell and thin on their own clock, so the sea
            // never reads as a stamped grid of identical caps.
            const double id=hash2(k,row+71);
            if(id<.34)continue;
            const double breath=.5+.5*std::sin(c.t*(.45+.55*hash2(k,row+72))+Tau*hash2(k,row+73));
            const double size=(.45+.95*hash2(k,row+38))*(.38+.62*breath)*(1+.35*c.lift(std::clamp(int((1-depth)*5),0,5)));
            const double span=(40+170*depth)*size;
            if(span<14)continue;
            const double x0=middle-span*(.30+.22*hash2(k,row+74));
            const double thick=(4+30*depth)*(.7+.5*size);
            const int lobes=2+int(4*hash2(k,row+75));
            auto under=[&](double u) {
                const double lobe=.62+.38*std::abs(std::sin(Pi*u*lobes+hash2(k,row+76)*3));
                // Heavier on the leading (right) side where the crest spills.
                return thick*std::pow(std::sin(Pi*u),.65)*lobe*(.7+.6*u)*(1+.22*noise1(u*9,row+k));
            };
            for(int pass=0;pass<2;++pass) {
                // A thin Prussian underprint separates the cap from its swell.
                const double dy=pass?0:2.5+2*depth;
                cv.color(pass?cream:hex(0x0e2347),pass?.97:.55);
                cv.moveTo(x0,top(x0)+dy);
                for(int j=1;j<=24;++j){double x=x0+j*span/24;cv.lineTo(x,top(x)-(pass?0:1)+dy);}
                for(int j=24;j>=0;--j){double u=j/24.0,x=x0+u*span;cv.lineTo(x,top(x)+under(u)+dy);}
                cv.closePath();cv.fill();
            }
            // Small curled fingers spill forward off the leading edge.
            const int fingers=int(1+4*size*hash2(k,row+77)+.5);
            for(int j=0;j<fingers;++j) {
                const double u=.62+.34*(j+.5)/std::max(1,fingers),x=x0+span*u,yy=top(x)+under(u)*.55;
                const double ln=(5+19*depth)*(.7+.6*hash2(j+k,row))*(.6+.5*size);
                cv.color(cream,.96);cv.moveTo(x-3,yy-2);
                cv.curveTo(x+ln*.45,yy-6,x+ln*1.15,yy,x+ln*.95,yy+ln*.7);
                cv.curveTo(x+ln*.70,yy+ln*.25,x+ln*.3,yy+6,x-3,yy+3);cv.closePath();cv.fill();
            }
        }
        for(int j=0;j<3;++j) {
            std::vector<V2> water;
            for(int k=0;k<=90;++k){double x=-25+k*22;water.push_back({x,top(x)+10+j*(8+14*z)});}
            cv.polyline(water,1+j*.3,teal,.20);
        }
    }
    if(age>0 && age<4.5) {
        const double a=sstep(0,.25,age)*(1-sstep(2.8,4.5,age));
        for(int k=0;k<17;++k) {
            const double x=220+hash2(k,311)*1650-35*age,y=820+hash2(k,312)*240;
            const double w=(20+70*hash2(k,313))*a;
            cv.color(cream);cv.moveTo(x-w,y);
            cv.curveTo(x-w*.6,y-10*a,x-w*.2,y-29*a,x+w*.3,y-10*a);
            cv.curveTo(x+w*.7,y-22*a,x+w,y-9*a,x+w,y+5*a);
            cv.curveTo(x+w*.2,y+12*a,x-w*.5,y+10*a,x-w,y);cv.closePath();cv.fill();
        }
    }
    c.gpu.over(cv,1,0,float(opacity));
}
void studySea(Ctx& c,bool aftermath) {
    const double paper=kanagawaPaperWeight(c.t);
    if(paper<.999)calmSea(c,aftermath);
    if(paper>0)printSea(c,paper);
}

// Continuous curled fingers with broad roots and a finely tapered hook.
// Their blue underprint and cream crest are part of the same connected foam.
void tendril(Canvas& cv,V2 root,double angle,double length,double width,double turn,Col col,double alpha) {
    std::vector<V2> l,r;
    V2 p=root;
    for(int j=0;j<=30;++j) {
        const double u=j/30.0,a=angle+turn*std::pow(u,1.7);
        const V2 n(-std::sin(a),std::cos(a));
        const double w=width*std::pow(1-u,1.1)+.15;
        l.push_back(p+n*w);r.push_back(p-n*w);
        p=p+V2(std::cos(a),std::sin(a))*(length/30);
    }
    for(auto it=r.rbegin();it!=r.rend();++it)l.push_back(*it);
    polyfill(cv,l,col,alpha);
}

// Blunt, hooked foam digits have rounded ends, unlike hairline crescents.
void foamDigit(Canvas& cv,V2 root,double angle,double length,double width,double turn,Col col,double alpha) {
    const V2 d={std::cos(angle),std::sin(angle)},n={-d.y,d.x};
    auto point=[&](double x,double y){return root+d*x+n*y;};
    auto curve=[&](V2 a,V2 b,V2 q){cv.curveTo(a.x,a.y,b.x,b.y,q.x,q.y);};
    const double bend=length*turn*.28;
    V2 start=point(0,-width);cv.color(col,alpha);cv.moveTo(start.x,start.y);
    curve(point(length*.25,-width*1.2),point(length*.57,bend-width),point(length*.84,bend));
    curve(point(length*.99,bend-width*.2),point(length*1.10,bend+width*.55),point(length*1.04,bend+width*1.45));
    curve(point(length*.99,bend+width*.55),point(length*.90,bend+width*.6),point(length*.83,bend+width*.95));
    curve(point(length*.60,bend+width),point(length*.30,width),point(0,width));
    cv.closePath();cv.fill();
}

void printWaveFoot(Ctx& c,double growth,bool shadow) {
    const double impactAt=kanagawaBreak(c).t+.60;
    const double a=growth*(1-sstep(impactAt+.1,impactAt+.65,c.t));
    if(a<=0)return;
    Canvas& cv=c.canvas();
    if(shadow) {
        std::vector<V2> p;
        for(int j=0;j<=40;++j){double x=-100+j*34;p.push_back({x,982+10*std::sin(x*.008+c.t*.4)});}
        for(int j=40;j>=0;--j){double x=-100+j*34;p.push_back({x,1047+18*std::sin(x*.006+c.t*.3)});}
        polyfill(cv,p,hex(0x071f47),.28*a);
    } else for(int k=0;k<3;++k) {
        const double x=-120+k*370+18*std::sin(c.t*.4+k),y=1074+10*std::sin(c.t*.6+k),h=(66+34*k)*a;
        cv.color(hex(0x204b70),a);cv.moveTo(x-180,y+60);
        cv.curveTo(x-130,y-h*.4,x-55,y-h,x+20,y-h);cv.curveTo(x+104,y-h,x+155,y-20,x+190,y+50);cv.closePath();cv.fill();
        cv.color(cream,a);cv.moveTo(x-90,y-h*.55);cv.curveTo(x-44,y-h-8,x+25,y-h-8,x+77,y-h*.80);cv.stroke(13+8*a);
        for(int j=0;j<7;++j) {
            V2 q={x-55+j*18,y-h+8+15*std::pow((j-2.0)/5,2)};
            tendril(cv,q,-.5+j*.12,(22+14*hash2(j,k))*a,4+3*a,.7+hash2(j,k+9),cream,a);
        }
    }
    c.gpu.over(cv);
}

void studyWave(Ctx& c,double growth) {
    const Col navy=hex(0x102955),blue=hex(0x285579),teal=hex(0x7397a4);
    // Low, broad gathering mass evolves into a thick hollow breaker.
    const double impactAt=kanagawaBreak(c).t+.60;
    const double release=impactAt-.90, plunge=std::max(0.0,(c.t-release)/.90);
    // Continuous acceleration carries the lip into the hit. It keeps moving
    // through contact, instead of rushing a quadratic fall in the last .6 s.
    const double forward=620*plunge*plunge/(plunge*plunge+.75);
    const double followThrough=std::max(0.0,plunge-1);
    const double fall=650*plunge*plunge/(plunge+.85)+700*followThrough*followThrough;
    const double pressure=c.score ? (c.score->integral(0,c.t)-c.score->integral(0,c.t-.9))/.9 : 0;
    const double sx=lerp(.62,1.0,growth),sy=lerp(.035,1.0,growth),dx=lerp(-520,0,growth);
    auto map=[&](V2 p) {
        const double crest=1-sstep(580,1080,p.y),lip=sstep(540,1040,p.x)*crest;
        const double bend=30*std::sin((c.t-51)*.33)*growth*crest;
        return V2(p.x*sx+dx+bend+forward*lip,
            1080+(p.y-1080)*sy-140*pressure*crest*growth+fall*crest);
    };
    auto move=[&](Canvas& cv,double x,double y){V2 p=map({x,y});cv.moveTo(p.x,p.y);};
    auto curve=[&](Canvas& cv,double ax,double ay,double bx,double by,double dx,double dy) {
        V2 a=map({ax,ay}),b=map({bx,by}),d=map({dx,dy});cv.curveTo(a.x,a.y,b.x,b.y,d.x,d.y);
    };
    Canvas& body=c.canvas();
    body.linear(270,150,720,1080,{{0,blue,1},{.42f,hex(0x274569),1},{.76f,navy,1},{1,ink,1}});
    move(body,-180,1100);
    curve(body,-240,720,-100,275,220,130);
    curve(body,475,-25,825,65,990,245);
    curve(body,1145,395,1120,515,1000,560);
    curve(body,1090,465,985,360,850,390);
    curve(body,660,410,655,590,850,735);
    curve(body,1040,880,1230,875,1440,1040);
    curve(body,1040,1170,250,1190,-180,1100);body.closePath();body.fill();
    const int mask=c.gpu.layer(body);
    Program& surface=c.gpu.effect("directionWaveFoot",R"(
        uniform sampler2D u_tex;
        void main(){o=texture(u_tex,v_uv)*(1.0-smoothstep(970.0,1100.0,design().y));}
    )");
    c.gpu.pass(surface,Blend::Over,[&](Program& p){c.gpu.bindTexture(0,mask,p,"u_tex");});
    Canvas& flow=c.canvas();
    // Nested material curves share one family of flow coordinates. Their
    // order stays intact through the curl, so no spirograph crossings form.
    auto stream=[&](double f) {
        move(flow,-155+f*1540,1120);
        curve(flow,-230+f*1170,825,-145+f*700,245+f*325,240+f*475,158+f*306);
        curve(flow,480+f*200,-1+f*456,840-f*58,81+f*350,993-f*88,280+f*244);
        curve(flow,1090-f*134,386+f*182,1096-f*210,485+f*104,1017-f*190,536+f*106);
    };
    // Thick shoulder shadow and inner-barrel light describe water's volume.
    flow.radial(460,400,680,{{0,hex(0x233856),.55f},{.6f,navy,.18f},{1,navy,0}});
    flow.rect(-200,0,1600,1120);flow.fill();
    flow.radial(870,534,245,{{0,teal,.40f},{.6f,blue,.14f},{1,blue,0}});
    flow.rect(420,250,900,750);flow.fill();
    // Six broad material flows carry the frequency roles up the face.
    // Their centres travel continuously with integrated energy; accents
    // brighten water along the flow instead of displacing the whole sea.
    for(int band=0;band<6;++band) {
        const double clock=c.score?c.score->integral(band,c.t):0;
        const double f=.06+band*.145+.085*std::sin(clock*5.8+c.t*.7+band);
        flow.color(mix(blue,teal,.40+.08*band),.12+.95*c.lift(band)+.30*c.hit(4));
        stream(f);flow.stroke(38+16*c.band(band));
        flow.color(mix(teal,cream,.16),.2+.36*c.lift(band)+.18*c.hit(5));
        stream(f+.013);flow.stroke(3.1+2.2*c.lift(band));
    }
    for(int line=0;line<16;++line) {
        const int band=line%6;
        const double clock=c.score?c.score->integral(band,c.t):0;
        const double f=(line+.15*hash2(line,17)+.38*std::sin(clock*3+c.t*.3+line))/16.0;
        flow.color(mix(teal,cream,line%12==0?.45:.02),(.10+.15*hash2(line,16)+.26*c.lift(band))*.30);
        stream(f);flow.stroke(line%12==0?2.1+2.2*c.lift(band):.55+hash2(line,13)*.7);
    }
    // Broken flowing lace on the face. It follows these same curves and is
    // concentrated beside foam rather than covering the whole face.
    for(int line=0;line<4;++line) {
        const double f=(line+.5)/29.0;
        flow.color(cream,.04+.04*hash2(line,22));stream(f);flow.stroke(.8);
    }
    const int details=c.gpu.layer(flow);
    Program& clipped=c.gpu.effect("directionClip",R"(
        uniform sampler2D u_tex,u_mask;
        void main(){vec4 a=texture(u_tex,v_uv);o=a*texture(u_mask,v_uv).a*(1.0-smoothstep(970.0,1100.0,design().y));}
    )");
    c.gpu.pass(clipped,Blend::Over,[&](Program& p){c.gpu.bindTexture(0,details,p,"u_tex");c.gpu.bindTexture(1,mask,p,"u_mask");});
    Canvas& foam=c.canvas();
    const std::array<Cubic,3> crown={{{{-110,420},{-45,270},{110,165},{220,130}},
        {{220,130},{475,-25},{825,65},{990,245}},{{990,245},{1145,395},{1120,515},{1000,560}}}};
    for(int seg=0;seg<3;++seg) {
        const Cubic& q=crown[seg];std::vector<V2> rim,inside;
        for(int j=0;j<=90;++j) {
            const double u=j/90.0;V2 n=unit(q.tangent(u));n={-n.y,n.x};
            const double w=(seg==0?45:92)*(1+.45*fbm1(u*17,seg+10))*(.45+.55*growth);
            rim.push_back(map(q.at(u)-n*(5+6*noise1(u*20,seg))));inside.push_back(map(q.at(u)+n*w));
        }
        auto band=rim;for(auto it=inside.rbegin();it!=inside.rend();++it)band.push_back(*it);
        polyfill(foam,band,cream);
    }
    // Hokusai claws: tapered talons that reach forward along the crest and
    // hook back at the tip, each splitting into smaller fingers. A Prussian
    // underprint drawn first keeps neighbouring talons readable as separate.
    struct Talon {V2 root;double ang,len,width,turn;};
    std::vector<Talon> talons;
    for(int seg=0;seg<3;++seg) {
        const Cubic& q=crown[seg];
        const int count=seg==0?4:(seg==1?11:5);
        for(int k=0;k<count;++k) {
            const double u=std::clamp((k+.15+.70*hash2(k,seg+48))/count,.02,.98);
            const V2 tangent=map(q.at(std::min(1.0,u+.005)))-map(q.at(std::max(0.0,u-.005)));
            const double ang=std::atan2(tangent.y,tangent.x);
            V2 normal=unit(q.tangent(u));normal={-normal.y,normal.x};
            // Talons grow toward the lip, where the wave throws forward.
            const double reach=seg==0?.45:(seg==1?.55+.45*u:1.0);
            const double base=(70+110*hash2(k,seg+30))*reach*(.05+.95*growth)*(1+.22*c.hit(4));
            // Inner talons hang from the sheet; outer ones fringe the rim.
            const double inside=(seg==0?45:92)*(.45+.55*growth)*(.75+.2*hash2(k,seg+31));
            talons.push_back({map(q.at(u)+normal*inside),ang+.55+.35*hash2(k,seg+94),base,
                (13+10*hash2(k,seg))*(.1+.9*growth),1.25+.6*hash2(k,seg+77)});
            // Only the thrown lip fringes its outer edge, rooted in the sheet.
            if(seg==2 && u<.75 && hash2(k,seg+33)>.3)
                talons.push_back({map(q.at(u)+normal*10),ang-.35-.3*hash2(k,seg+95),base*.42,
                    (9+5*hash2(k,seg+2))*(.1+.9*growth),1.5+.5*hash2(k,seg+78)});
        }
    }
    for(int pass=0;pass<2;++pass) for(std::size_t i=0;i<talons.size();++i) {
        const Talon& t=talons[i];
        const Col col=pass?cream:hex(0x1d4673);
        const double grow=pass?0:3.2;
        tendril(foam,t.root,t.ang,t.len+grow,t.width+grow,t.turn,col,1);
        // Two or three fingers fork from the outer half and hook harder.
        const int forks=t.len>110?2:1;
        for(int f=0;f<forks;++f) {
            const double at=.42+.16*f+.08*hash2(int(i),f+120);
            const double a=t.ang+t.turn*std::pow(at,1.7);
            V2 p=t.root;
            // Walk the main talon's centreline to the fork root.
            for(int j=0;j<int(at*30);++j) {
                const double uu=j/30.0,aa=t.ang+t.turn*std::pow(uu,1.7);
                p=p+V2(std::cos(aa),std::sin(aa))*(t.len/30);
            }
            const double side=(f%2?1:-1)*(.55+.25*hash2(int(i),f+121));
            tendril(foam,p,a+side,t.len*(.30+.16*hash2(int(i),f+122))+grow,t.width*(.45-.08*f)+grow*.6,
                t.turn*(1.1+.4*f)*(side>0?1:.6),col,1);
        }
    }
    // Foam follows the same waterline feather as the face. The remaining
    // crest breaks into the existing impact spray rather than blinking out.
    const int foamLayer=c.gpu.layer(foam);
    Program& feather=c.gpu.effect("directionFoamFoot",R"(
        uniform sampler2D u_tex;
        uniform float u_gain,u_clear;
        void main(){o=texture(u_tex,v_uv)*u_gain*u_clear*
            (1.0-smoothstep(955.0,1090.0,design().y));}
    )");
    c.gpu.pass(feather,Blend::Over,[&](Program& p){
        c.gpu.bindTexture(0,foamLayer,p,"u_tex");
        p.set("u_gain",float(1+.40*c.hit(5)));
        p.set("u_clear",float(1-sstep(impactAt+.25,impactAt+.65,c.t)));
    });
    Canvas& spray=c.canvas();
    for(int i=0;i<95;++i) {
        const double u=hash2(i,64),a=.25+.55*u;
        const V2 source=map(crown[1].at(a));
        const double life=.75,phase=std::fmod(c.t*.7+hash2(i,67),life)/life;
        const double dist=20+(90+100*c.hit(3))*phase,speed=3+8*hash2(i,68);
        const V2 p=source+V2(-dist*.72,-dist*.60-15*hash2(i,69));
        const double alpha=(.15+.7*c.hit(4))*(1-phase)*growth;
        if(i%3) ellipseFill(spray,p.x,p.y,1+2*hash2(i,65),1.3+2.5*hash2(i,66),cream,alpha);
        else spray.line(p.x,p.y,p.x+speed,p.y+speed*.44,.6+hash2(i,65),cream,alpha);
    }
    c.gpu.over(spray,1,0,float(1-sstep(impactAt+.25,impactAt+.65,c.t)));
}

void studyBoat(Ctx& c,V2 at,double scale,double tilt,bool bearer,bool rescue=false) {
    Canvas& body=c.canvas();Canvas& light=c.canvas();
    const double sc=scale/(400.0/450.0);
    // Keep the crossing's hull, rowers, hands, oar pivots and bearer pose.
    // Fine wood structure develops with the dawn, without swapping the rig.
    drawBoat(c,body,light,at,450*scale,tilt,bearer?5:6,sstep(37,51,c.t),bearer?sc:scale/(250.0/450.0)*.7,bearer,rowingClock(c)+(bearer?0:.3));
    c.gpu.over(body);c.gpu.over(light,1.5);c.gpu.add(light,.35,12);
    Canvas& trim=c.canvas();trim.save();trim.translate(at.x,at.y);trim.rotate(tilt);
    const double detail=sstep(48,53,c.t),half=225*scale;
    for(int j=0;j<3;++j) {
        trim.color(j==1?gold:wood,detail*(j==1?.5:.55));trim.moveTo(-half+12,3+j*5);
        trim.curveTo(-half*.5,17+j*3,half*.5,18+j*3,half-13,-2+j*3);trim.stroke(j==1?1.1:.7);
    }
    for(int j=0;j<9;++j) {
        const double x=-half*.78+j*half*.17;
        trim.line(x,10+4*std::sin(j*.3),x+6,17+5*std::sin(j*.3),.6,hex(0x513b2b),detail*.65);
    }
    if(rescue) {
        RigIn r;r.h=66*scale/.57;r.hip={half*.72,-11};r.facing=1;r.lean=.65;
        r.garment=Garment::Jacket;r.footF={half*.87,7};r.footB={half*.70,7};
        r.handF={half+9,21};r.handB={half-2,11};drawBody(trim,solve(r),ink);
    }
    trim.restore();c.gpu.over(trim);
}

void studyWreck(Ctx& c,V2 rescueBoat,double boatScale) {
    Canvas& cv=c.canvas();
    const double age=c.t-(kanagawaBreak(c).t+.60),settle=sstep(0,2.0,age),pan=sstep(68.9,71.7,c.t);
    cv.save();cv.translate(0,-75*std::sin(Pi*settle)+5*std::sin(age*1.8));
    // Settled halves and scattered structural timbers, well separated crew.
    for(int half=0;half<2;++half) {
        cv.save();cv.translate(lerp(1385.0,850.0,settle)+half*lerp(300.0,470.0,settle)-1180*pan,865+20*settle+half*28*settle-55*pan);cv.rotate((half?.16:-.11)*sstep(0,.5,age));
        cv.color(wood);cv.moveTo(-120,-14);cv.curveTo(-95,20,50,30,112,1);
        cv.lineTo(97,-13);cv.lineTo(85,-4);cv.lineTo(65,-12);cv.lineTo(55,-3);cv.lineTo(34,-13);
        cv.curveTo(-30,1,-100,-3,-120,-14);cv.closePath();cv.fill();
        cv.polyline({{-107,-7},{-70,9},{0,14},{75,1}},2,gold,.7);cv.restore();
    }
    for(int i=0;i<13;++i) {
        const double x=740+hash2(i,110)*820-950*pan,y=824+hash2(i,111)*170;
        const double l=30+45*hash2(i,112),a=(hash2(i,113)-.5)*.65;
        cv.line(x,y,x+l*std::cos(a),y+l*std::sin(a),i%3?5:3,wood);cv.line(x,y-1,x+l*std::cos(a),y+l*std::sin(a)-1,1,gold,.65);
    }
    const V2 crew[5]={{970,865},{1100,915},{1230,876},{1370,945},{1510,831}};
    for(int i=0;i<5;++i) {
        const double tow=sstep(68.35+i*.12,69.15+i*.12,c.t);
        const V2 aboard=rescueBoat+V2((i-2)*55*boatScale,18*boatScale);
        const V2 q=lerp(crew[i]+V2(-950*pan,0),aboard,tow);
        const double x=q.x,y=q.y+3*std::sin(c.t*1.7+i);
        const double side=i%2?1:-1;
        cv.capsule({x,y+7},{x-3*side,y-19},7.5,9,ink);
        cv.capsule({x-3*side,y-24},{x-3*side,y-32},5,5.2,ink);
        cv.capsule({x-8,y-16},{x-22,y-3},3.5,3,ink);
        cv.capsule({x-22,y-3},{x-35,y+5},3,2.5,ink);
        cv.capsule({x+6,y-16},{x+20,y-4},3.5,3,ink);
        cv.capsule({x+20,y-4},{x+32,y+6},3,2.5,ink);
        cv.line(x-44,y+9,x+44,y+11,8,wood,1-tow);cv.line(x-40,y+7,x+40,y+9,1.7,gold,.75*(1-tow));
        cv.color(teal,.6*(1-tow));cv.moveTo(x-49,y+13);cv.curveTo(x-20,y+23,x+25,y+22,x+48,y+15);cv.stroke(1.7);
    }
    cv.restore();c.gpu.over(cv);
    Canvas& foam=c.canvas();
    for(int k=0;k<12;++k) {
        const double x=210+k*132,y=925+16*std::sin(k*.65);
        foam.color(cream,.6);foam.moveTo(x-65,y);foam.curveTo(x-20,y-22,x+48,y+12,x+82,y-10);foam.stroke(3.2);
        foam.color(teal,.4);foam.moveTo(x-30,y+16);foam.curveTo(x+16,y+6,x+54,y+23,x+96,y+10);foam.stroke(1.3);
    }
    c.gpu.over(foam);
}


// Velocity-aligned streaks and connected rolling foam, with a brief paper flash.
void impact(Ctx& c,double age) {
    if(age<0 || age>6)return;
    Canvas& cv=c.canvas();
    const double erupt=sstep(0,.17,age)*(1-sstep(.65,1.5,age));
    if(erupt>0) {
        // One domed explosion of whitewater, built from staggered plumes.
        // The union of their rounded profiles gives a single silhouette with
        // sloping shoulders; a shaded back layer and blue folds give volume.
        constexpr int plumes=7;
        double px[plumes],pw[plumes],ph[plumes];
        for(int m=0;m<plumes;++m) {
            const double lag=.035*std::abs(m-3.2);
            const double e=sstep(lag,.17+lag,age)*(1-sstep(.65+lag*.6,1.5,age));
            px[m]=860+m*150+30*std::sin(age*2+m);
            pw[m]=(210+70*hash2(m,281))*(.6+.4*e);
            ph[m]=(250+300*hash2(m,282))*(1-.55*std::pow(std::abs(m-3.2)/3.2,2))*e;
        }
        auto envelope=[&](double x,double scale,double shift) {
            double h=0;
            for(int m=0;m<plumes;++m) {
                const double d=(x-px[m]-shift)/(pw[m]*scale*1.25);
                if(std::abs(d)<1)h=std::max(h,ph[m]*.82*scale*std::pow(1-d*d,1.3));
            }
            return h;
        };
        // Billowing lobes ride the envelope: a cumulus of foam, not a peak.
        auto height=[&](double x,double scale,double shift) {
            double h=envelope(x,scale,shift);
            for(int b=0;b<26;++b) {
                const double bx=520+b*58+20*hash2(b,290)+shift;
                const double base=envelope(bx,scale,shift);
                if(base<30)continue;
                const double r=(38+46*hash2(b,291))*(.5+.5*base/450)*scale;
                const double d=x-bx;
                if(std::abs(d)<r)h=std::max(h,base+std::sqrt(r*r-d*d)-r*.55);
            }
            return h;
        };
        for(int layer=0;layer<2;++layer) {
            const double scale=layer?1.0:1.07,shift=layer?0:-18;
            std::vector<V2> edge={{420,1140}};
            for(double x=420;x<=2000;x+=8) {
                const double h=height(x,scale,shift);
                edge.push_back({x,1075-h-(h>4?10*erupt*noise1(x*.03,layer+64):0)});
            }
            edge.push_back({2000,1140});
            double top=1080;for(V2 p:edge)top=std::min(top,p.y);
            const Col body=layer?cream:mix(cream,teal,.38);
            cv.linear(0,top,0,1100,{{0,body,1},{.55f,body,1},{1,mix(body,hex(0x315985),.45),1}});
            cv.moveTo(edge[0].x,edge[0].y);for(V2 p:edge)cv.lineTo(p.x,p.y);cv.closePath();cv.fill();
        }
        // Talons fringe the dome, thrown outward from the point of contact.
        for(int k=0;k<34;++k) {
            const double x=560+1300*hash2(k,285);
            const double h=height(x,1,0);
            if(h<40)continue;
            const double y=1075-h+6;
            const double out=std::atan2(-(h+120),(x-1310)*1.4);
            const double side=x<1310?-1:1;
            const double ln=erupt*(28+70*hash2(k,288))*(.6+.4*h/550);
            tendril(cv,{x,y},out,ln+3,7+5*hash2(k,289)+3,1.3*side,hex(0x6f93ad),1);
            tendril(cv,{x,y},out,ln,6+5*hash2(k,289),1.3*side,cream,1);
        }
        for(int k=0;k<14;++k) {
            const double x=600+1250*hash2(k,299),h=height(x,1,0);
            if(h<70)continue;
            const double yy=1075-h*(.25+.55*hash2(k,301));
            cv.color(teal,.55);cv.moveTo(x,yy+34*erupt);
            cv.curveTo(x-24*erupt,yy+7*erupt,x+30*erupt,yy-14*erupt,x+44*erupt,yy-31*erupt);cv.stroke(2+2.5*erupt);
        }
    }
    // Spray follows a launch velocity with ballistic gravity, never bead discs.
    for(int i=0;i<260;++i) {
        const double launch=.09+.25*hash2(i,220),a=age-launch;
        if(a<0 || a>1.85)continue;
        const double vx=(hash2(i,221)-.42)*1040,vy=-280-660*hash2(i,222);
        const V2 p={1310+vx*a,830+vy*a+430*a*a},v={vx,vy+860*a};
        const V2 d=unit(v),n={-d.y,d.x};const double w=1.1+3.4*hash2(i,223),alpha=(1-sstep(1.2,1.85,a));
        polyfill(cv,{p+d*(w*4),p+n*w,p-d*w*2,p-n*w},cream,alpha);
    }
    // Expanding, curved fronts carry foam toward the viewer from contact.
    // Each has thin ends, so no bright straight line cuts across the frame.
    for(int k=0;k<7;++k) {
        const double u=age-.16*k;if(u<0)continue;
        const double radius=220+640*u,cy=860+35*u;
        const double alpha=(1-sstep(2.1,4.2,u))*.78;
        if(cy+radius*.24>1300)continue;
        std::vector<V2> front;
        for(int j=0;j<=72;++j) {
            const double a=j*Pi/72;
            front.push_back({1310+radius*std::cos(a),cy+radius*.24*std::sin(a)+5*std::sin(a*9-u*3)});
        }
        for(int j=1;j<=72;++j) {
            const double w=(5+23*(1-sstep(.3,2.8,u)))*std::pow(std::sin(j*Pi/73),.6);
            cv.line(front[j-1].x,front[j-1].y,front[j].x,front[j].y,w,cream,alpha);
        }
    }
    c.gpu.over(cv);
    if(age<.22) {Canvas& flash=c.canvas();flash.fillRect(0,0,1920,1080,cream,.88*window(age,0,.016,.095,.07));c.gpu.over(flash);}
}
void flyingTimbers(Ctx& c,double age) {
    if(age<0 || age>2.5)return;
    Canvas& cv=c.canvas();
    for(int i=0;i<21;++i) {
        const double a=age-(.025+.30*hash2(i,245));if(a<0)continue;
        const double vx=(hash2(i,240)-.5)*650,vy=-260-380*hash2(i,241);
        V2 p={1490+120*(hash2(i,246)-.5)+vx*a,850+vy*a+260*a*a};
        const double ang=(hash2(i,242)-.5)*3+age*(hash2(i,243)-.5)*7;
        V2 d={std::cos(ang),std::sin(ang)};
        const double len=25+55*hash2(i,244);
        if(p.y>1080)continue;
        cv.capsule(p-d*len*.5,p+d*len*.5,2.5,3.0,wood);
        cv.line(p.x,p.y,p.x+d.x*len*.5,p.y+d.y*len*.5,1,gold,.8);
    }
    c.gpu.over(cv);
}
}

double kanagawaPaperWeight(double t) {
    return sstep(38,43,t)*(1-sstep(68,70.7,t));
}
void kanagawaPrintSea(Ctx& c,double opacity) { printSea(c,opacity); }
BoatPose kanagawaPrintSupport(const Ctx& c,BoatPose pose,double half) {
    const double weight=kanagawaPaperWeight(c.t);
    if(weight<=0)return pose;
    const double y=pose.at.y;
    const double l=printLift(c,pose.at.x-half,y),m=printLift(c,pose.at.x,y),r=printLift(c,pose.at.x+half,y);
    pose.at.y+=(l+2*m+r)*.25*weight;
    pose.tilt+=std::clamp(std::atan2(r-l,2*half),-.15,.15)*weight;
    return pose;
}

void drawEndingWater(Ctx& c) {
    const double impactAt=kanagawaBreak(c).t+.60,age=c.t-impactAt;
    if(c.t<50 && kanagawaPaperWeight(c.t)<.999)kanagawaSea(c);else kanagawaHorizon(c);
    studySea(c,age>=0);
    kanagawaPlovers(c,1);
    if(c.t<51.0)drawFlock(c,Col(0.008f,0.026f,0.022f),1,1-sstep(48,51,c.t));
    if(c.t<52) {
        c.gpu.beginOffscreen(true);kanagawaEntryFront(c);
        c.gpu.composite(c.gpu.endOffscreen(),Blend::Over,1,float(1-sstep(48,52,c.t)));
    }
    if(c.t<69)crossingStrands(c,std::min(49.8,c.t));
    const double growth=sstep(50.0,impactAt-.52,c.t);
    // Boats draw back together and sample the same travelling swell.
    auto pose=[&](bool near) {
        const BoatPose entry=seaBoatPose(c,near);
        const double pull=std::sin(Pi*sstep(54,impactAt,c.t)),join=sstep(48,53,c.t);
        V2 target=near?V2(1535-85*pull,908-42*growth):V2(1645-45*pull,727);
        const double crest=40*std::exp(-std::pow((c.t-(near?61.4:59.8))/1.4,2));
        target.y-=crest;
        const double ripple=(3+7*std::exp(-std::max(0.0,age)*.5))*std::sin(c.t*1.1+(near?0:1.4));
        target.y+=ripple;
        double tilt=(near?-.02-.08*growth:-.035-.025*growth)-crest*.0018;
        if(age>=0) {target.y-=38*std::exp(-age*.9)*std::sin(age*4.2);tilt+=.16*std::exp(-age*.7)*std::sin(age*3.2);}
        const BoatPose supported=kanagawaPrintSupport(c,{target,tilt},near?140:85);
        return BoatPose{lerp(entry.at,supported.at,join),lerp(entry.tilt,supported.tilt,join)};
    };
    if(age<.65){printWaveFoot(c,growth,true);studyWave(c,growth);printWaveFoot(c,growth,false);}
    BoatPose far=pose(false),near=pose(true);
    const double rescue=sstep(impactAt+.7,68.4,c.t),recede=sstep(68.9,71.7,c.t);
    far.at=lerp(far.at,V2(1330,842),rescue);
    far.at=lerp(far.at,V2(380,810),recede);
    if(age<=.025) {studyBoat(c,far.at,lerp(250.0/450,.57,sstep(48,53,c.t)),far.tilt,false);studyBoat(c,near.at,lerp(400.0/450,.90,sstep(48,53,c.t)),near.tilt,true);}
    else {
        // Broken halves are exposed before/after the dense spray crown.
        if(age>.025)studyWreck(c,far.at,lerp(lerp(.57,.8,rescue),.40,recede));
        studyBoat(c,far.at,lerp(lerp(.57,.8,rescue),.40,recede),far.tilt*(1-.7*rescue),false,rescue>.5);
        impact(c,age);flyingTimbers(c,age);
    }
}

void drawEndingBearer(Ctx& c) {
    const double impactAt=kanagawaBreak(c).t+.60,age=c.t-impactAt;
    // The lantern-bearer remains visible as the surge carries him toward shore.
    if(age>.8 && c.t<70.7) {
        Canvas& body=c.canvas();Canvas& light=c.canvas();
        const double u=sstep(impactAt+.8,70.7,c.t);
        const double tumble=1-sstep(69.8,70.7,c.t);
        const V2 p=lerp(V2(1560,932),V2(1100,1010),u)+V2(0,12*std::sin(age*2.3)*tumble);
        const double turn=(2.25+.55*std::sin(age*4.7))*tumble;
        const double roll=-.6+.2*std::sin(c.t*2);
        auto rotate=[&](V2 q){return V2(q.x*std::cos(roll)-q.y*std::sin(roll),q.x*std::sin(roll)+q.y*std::cos(roll));};
        RigIn r;r.h=105;r.hat=true;r.garment=Garment::Jacket;r.hip=p+rotate({0,-30});r.facing=1;
        r.lean=roll;r.headTilt=roll;
        r.footF=p+rotate({30,8});r.footB=p+rotate({-20,10});
        r.handF=p+rotate({40,-63});r.handB=p+rotate({-26,-27});
        body.save();body.translate(p.x,p.y);body.rotate(turn);body.translate(-p.x,-p.y);
        drawBody(body,solve(r),ink);body.restore();c.gpu.over(body);
        light.save();light.translate(p.x,p.y);light.rotate(turn);light.translate(-p.x,-p.y);
        V2 hand=p+V2(30,-65);drawBearerLantern(light,light,hand,.15,105.0/146,1);light.restore();
        c.gpu.over(light,1.3);c.gpu.add(light,.2,8);
        if(tumble>0) {
            Canvas& wash=c.canvas();wash.color(cream,.94*tumble);
            wash.moveTo(p.x-82,p.y+12);wash.curveTo(p.x-42,p.y-2,p.x-9,p.y+25,p.x+24,p.y+8);
            wash.curveTo(p.x+65,p.y-6,p.x+88,p.y+10,p.x+98,p.y+25);
            wash.curveTo(p.x+30,p.y+41,p.x-40,p.y+34,p.x-82,p.y+12);wash.closePath();wash.fill();
            wash.line(p.x-44,p.y+25,p.x+46,p.y+26,2.1,teal,.7*tumble);c.gpu.over(wash);
        }
    }
}
// One wind-driven summit event, confined to the ending chapter. The snow
// stays attached to Fuji and thins into long wisps instead of particle dots.
void endingSummitWind(Ctx& c) {
    double accent=58.6,strength=0;
    if(c.score)for(const auto& e:c.score->onsets)
        if(e.t>=57 && e.t<=60 && e.strength>strength){accent=e.t;strength=e.strength;}
    const double age=c.t-accent;
    if(age<-.35 || age>2.1)return;
    const double visibility=sstep(-.35,.12,age)*(1-sstep(1.15,2.1,age));
    Canvas& cv=c.canvas();
    for(int j=0;j<7;++j) {
        const double y=477+j*2.4,travel=std::max(0.0,age)*(42+7*j);
        cv.color(cream,visibility*(.27-.025*j));
        cv.moveTo(1085+j*2,y);
        cv.curveTo(1107+travel*.35,y-8-j,1131+travel*.75,y-7-j,1150+travel,y-2-j);
        cv.stroke(1.8+.45*j);
    }
    c.gpu.over(cv);
}

void drawJourneyEnding(Ctx& c) {
    c.cameraX=0;
    kanagawaSky(c);kanagawaSun(c);kanagawaFuji(c,1);endingSummitWind(c);
    drawEndingWater(c);
    drawEndingRoad(c);
    drawEndingBearer(c);
    FinishParams f;f.time=float(c.t);f.paper=1;f.grain=.008;f.bloom=.28;f.vignette=.25;
    Canvas& title=c.canvas();drawTitle(title,1,window(c.t,48.6,.8,53.8,1.2),hex(0x7e9cd8),cream);
    drawEndingRoadTitle(title,c.t);
    c.gpu.finish(f,&title);
}
}
