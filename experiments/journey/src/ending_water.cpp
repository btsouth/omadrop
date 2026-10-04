// Animated water chapter. Geometry, flow and particles share a deterministic
// scene clock and measured Score; no frame history or baked artwork.
#include "parts.h"
#include "rig.h"
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
void polyfill(Canvas& cv,const std::vector<V2>& p,Col col,double alpha=1) {
    if(p.empty())return;
    cv.color(col,alpha);cv.moveTo(p[0].x,p[0].y);
    for(std::size_t i=1;i<p.size();++i)cv.lineTo(p[i].x,p[i].y);
    cv.closePath();cv.fill();
}

void studySea(Ctx& c,bool aftermath) {
    Canvas& cv=c.canvas();
    cv.linear(0,610,0,1080,{{0,hex(0x536786),1},{.42f,blue,1},{1,navy,1}});
    cv.rect(0,612,1920,468);cv.fill();
    for(int row=0;row<65;++row) {
        const double z=row/64.0,y=616+456*std::pow(z,1.72);
        const double amp=2+22*z*z;
        std::vector<V2> p;
        for(int j=0;j<=110;++j) {
            const double x=j*18.0;
            const double age=c.t-(kanagawaBreak(c).t+.60);
            const double energy=c.score ? (c.score->integral(row%6,c.t)-c.score->integral(row%6,c.t-.8))/.8 : 0;
            const double swell=aftermath ? 35*z*std::exp(-std::max(0.0,age)*.35)*std::sin(x*.010-z*12-age*3) : 15*z*energy*std::sin(x*.006-c.t*.7+row);

            p.push_back({x,y+amp*std::sin(x*.0055+row*.82-c.t*.55)+swell+2*noise1(x*.012,row)});
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

void studyWave(Ctx& c,double growth) {
    // Low, broad gathering mass evolves into a thick hollow breaker.
    const double release=kanagawaBreak(c).t, plunge=std::max(0.0,(c.t-release)/.60);
    const double pressure=c.score ? (c.score->integral(0,c.t)-c.score->integral(0,c.t-.9))/.9 : 0;
    const double sx=lerp(.62,1.0,growth),sy=lerp(.035,1.0,growth),dx=lerp(-520,0,growth);
    auto map=[&](V2 p) {
        const double crest=1-sstep(580,1080,p.y),lip=sstep(540,1040,p.x)*crest;
        const double bend=30*std::sin((c.t-51)*.33)*growth*crest;
        const double forward=470*std::pow(std::min(1.4,plunge),1.65);
        const double fall=320*std::pow(std::min(1.4,plunge),2.0);
        return V2(p.x*sx+dx+bend+forward*lip,
            1080+(p.y-1080)*sy-14*pressure*crest*growth+fall*crest);
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
    for(int line=0;line<64;++line) {
        const int band=line%6;
        const double clock=c.score?c.score->integral(band,c.t):0;
        const double f=(line+.15*hash2(line,17)+.38*std::sin(clock*3+c.t*.3+line))/64.0;
        flow.color(mix(teal,cream,line%12==0?.45:.02),.10+.15*hash2(line,16)+.18*c.lift(band));
        stream(f);flow.stroke(line%12==0?2.1+2.2*c.lift(band):.55+hash2(line,13)*.7);
    }
    // Broken flowing lace on the face. It follows these same curves and is
    // concentrated beside foam rather than covering the whole face.
    for(int line=0;line<12;++line) {
        const double f=(line+.5)/29.0;
        flow.color(cream,.12+.12*hash2(line,22));stream(f);flow.stroke(.8);
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
            const double w=(seg==0?17:35)*(1+.45*fbm1(u*17,seg+10))*(.45+.55*growth);
            rim.push_back(map(q.at(u)-n*(5+6*noise1(u*20,seg))));inside.push_back(map(q.at(u)+n*w));
        }
        auto band=rim;for(auto it=inside.rbegin();it!=inside.rend();++it)band.push_back(*it);
        polyfill(foam,band,cream);
        // Unequal clusters: rounded curling limbs rooted in the cream sheet.
        for(int k=0;k<(seg==0?4:8);++k) {
            const double u=(k+.3)/(seg==0?4:8),ang=std::atan2(q.tangent(u).y,q.tangent(u).x);
            const V2 root=map(q.at(u));
            const double len=(55+95*hash2(k,seg+30))*(.05+.95*growth)*(1+.12*c.hit(4));
            for(int branch=0;branch<3;++branch) {
                const double a=ang+.95+branch*.37,ln=len*(.64+.19*branch),w=(10+10*hash2(k*3+branch,seg))*(.08+.92*growth);
                const V2 start=root+V2(std::cos(ang),std::sin(ang))*(branch-1)*8;
                const double curl=1.05+1.3*hash2(k*3+branch,seg+77);
                tendril(foam,start+V2(3,5),a,ln+5,w+3,curl,blue,.9);
                tendril(foam,start,a,ln,w,curl+.1,cream,1);
                const V2 fork=start+V2(std::cos(a+.20),std::sin(a+.20))*ln*.52;
                tendril(foam,fork,a-.65,ln*.48,w*.35,-1.95,cream,1);
                if(branch==1)tendril(foam,fork,a+.55,ln*.4,w*.31,2.45,cream,1);
            }
        }
        // Blue cavities in the foam read as lace rather than outlined beads.
        for(int k=0;k<68;++k) {
            const double u=(k+.5)/68;V2 n=unit(q.tangent(u));n={-n.y,n.x};
            const V2 p=map(q.at(u)+n*(7+16*hash2(k,seg+90)));
            const double a=std::atan2(n.y,n.x)+.7;
            tendril(foam,p,a,7+19*hash2(k,seg+2),1.2+2.5*hash2(k,seg+3),2.9,blue,.95);
        }
    }
    for(int k=0;k<8;++k) {
        const double u=(k+.25)/8;
        const V2 q=map(crown[1].at(u));
        const double ang=std::atan2(crown[1].tangent(u).y,crown[1].tangent(u).x);
        tendril(foam,q,ang-.9,25+30*hash2(k,82),5+4*hash2(k,83),-1.8,cream,.94);
    }
    c.gpu.over(foam);
    Canvas& spray=c.canvas();
    for(int i=0;i<95;++i) {
        const double u=hash2(i,64),a=.25+.55*u;
        const V2 source=map(crown[1].at(a));
        const double life=.75,phase=std::fmod(c.t*.7+hash2(i,67),life)/life;
        const double dist=20+(90+100*c.hit(3))*phase,speed=3+8*hash2(i,68);
        const V2 p=source+V2(-dist*.72,-dist*.60-15*hash2(i,69));
        spray.line(p.x,p.y,p.x+speed,p.y+speed*.44,.6+hash2(i,65),cream,(.15+.7*c.hit(4))*(1-phase)*growth);
    }
    c.gpu.over(spray);
}

void studyBoat(Ctx& c,V2 at,double scale,double tilt,bool bearer,bool rescue=false) {
    Canvas& cv=c.canvas();Canvas& light=c.canvas();
    cv.save();cv.translate(at.x,at.y);cv.rotate(tilt);cv.scale(scale,scale);
    // Rounded tapered fishing hull with distinct inside, rim, side and keel.
    cv.linear(0,-15,0,40,{{0,gold,1},{.3f,wood,1},{1,hex(0x563f30),1}});
    cv.moveTo(-225,-18);cv.curveTo(-140,20,120,25,224,-18);
    cv.curveTo(207,20,166,41,38,45);cv.curveTo(-120,47,-192,22,-225,-18);cv.closePath();cv.fill();
    cv.color(hex(0x493929));cv.moveTo(-212,-16);cv.curveTo(-102,7,115,9,212,-15);
    cv.curveTo(92,22,-96,21,-212,-16);cv.closePath();cv.fill();
    cv.color(gold);cv.moveTo(-225,-18);cv.curveTo(-134,12,139,15,224,-18);cv.stroke(4);
    cv.color(hex(0xd0a56e));cv.moveTo(-222,-14);cv.curveTo(-138,33,144,39,220,-14);cv.stroke(2.4);
    cv.color(hex(0x644a35));cv.moveTo(-202,4);cv.curveTo(-123,38,122,42,202,4);cv.stroke(1.1);
    for(int j=0;j<5;++j) {
        const double x=-148+j*70;cv.line(x,-4,x+7,13,6,wood);cv.line(x,-4,x+7,13,1.2,gold,.6);
    }
    // Oars originate at each rower's hands, pivot through the rim, with blades.
    for(int j=0;j<5;++j) {
        const double x=-130+j*65,brace=window(c.t,63.2,.7,66.4,.5);
        const double stroke=c.t*.65+2*(c.score?c.score->integral(0,c.t):0),phase=stroke+j*.02;
        const double row=std::sin(phase)*(1-brace);
        const V2 hand={x+19+13*row,-23+8*brace},lock={x+27,13},tip={x+50+45*row,65+14*std::cos(phase)};
        cv.line(hand.x,hand.y,tip.x,tip.y,3,gold);
        cv.capsule(tip,tip+unit(tip-lock)*23,4.5,6,wood);
        cv.disc(lock.x,lock.y,2.5,ink);
        RigIn r;r.h=61;r.hip={x,-13};r.facing=1;r.garment=Garment::Jacket;r.hair=Hair::Short;
        r.lean=.34*brace+.10*row;r.footF={x+26,-1};r.footB={x+6,1};r.handF=hand;r.handB={x+13,-25+8*brace};
        const Body b=solve(r);drawBody(cv,b,ink);cv.line(b.shoulder.x,b.shoulder.y,b.chest.x,b.chest.y+8,1.5,teal,.5);
        // Restrained faces and kerchiefs catch the horizon light.
        cv.capsule(b.head+V2(2,-2),b.head+V2(3,1),2.4,2,gold,.6);
    }
    if(bearer) {
        RigIn r;r.h=94;r.hat=true;r.garment=Garment::Jacket;r.facing=1;
        r.hip={172,-47};r.lean=-.10;r.footF={182,2};r.footB={158,1};r.handF={191,-54};r.handB={157,-41};
        Body b=solve(r);drawBody(cv,b,ink);cv.line(b.shoulder.x,b.shoulder.y,b.chest.x,b.chest.y+15,2,teal,.7);
        light.save();light.translate(at.x,at.y);light.rotate(tilt);light.scale(scale,scale);
        drawBearerLantern(light,light,r.handF,0,.64,1);light.restore();
    }
    if(rescue) {
        // One rescuer leans over the gunwale toward a survivor below.
        RigIn r;r.h=75;r.hip={180,-14};r.facing=-1;r.lean=-.45;r.garment=Garment::Jacket;
        r.footF={173,15};r.footB={195,11};r.handF={213,23};r.handB={194,11};drawBody(cv,solve(r),ink);
    }
    cv.restore();c.gpu.over(cv);c.gpu.over(light,1.3);c.gpu.add(light,.18,8);
    Canvas& wake=c.canvas();
    for(int k=0;k<4;++k) {
        wake.color(k?teal:cream,.35/(k+1));wake.moveTo(at.x-230*scale-k*15,at.y+23*scale+k*6);
        wake.curveTo(at.x-115*scale,at.y+54*scale+k*6,at.x+135*scale,at.y+48*scale+k*7,at.x+225*scale,at.y+2*scale+k*5);wake.stroke(k?1:2);
    }
    c.gpu.over(wake);
}

void studyWreck(Ctx& c) {
    Canvas& cv=c.canvas();
    const double age=c.t-(kanagawaBreak(c).t+.60),settle=sstep(0,2.0,age);
    cv.save();cv.translate(0,-75*std::sin(Pi*settle)+5*std::sin(age*1.8));
    // Settled halves and scattered structural timbers, well separated crew.
    for(int half=0;half<2;++half) {
        cv.save();cv.translate(lerp(1385.0,850.0,settle)+half*lerp(300.0,470.0,settle),865+20*settle+half*28*settle);cv.rotate((half?.16:-.11)*sstep(0,.5,age));
        cv.color(wood);cv.moveTo(-120,-14);cv.curveTo(-95,20,50,30,112,1);
        cv.lineTo(97,-13);cv.lineTo(85,-4);cv.lineTo(65,-12);cv.lineTo(55,-3);cv.lineTo(34,-13);
        cv.curveTo(-30,1,-100,-3,-120,-14);cv.closePath();cv.fill();
        cv.polyline({{-107,-7},{-70,9},{0,14},{75,1}},2,gold,.7);cv.restore();
    }
    for(int i=0;i<13;++i) {
        const double x=740+hash2(i,110)*820,y=824+hash2(i,111)*170;
        const double l=30+45*hash2(i,112),a=(hash2(i,113)-.5)*.65;
        cv.line(x,y,x+l*std::cos(a),y+l*std::sin(a),i%3?5:3,wood);cv.line(x,y-1,x+l*std::cos(a),y+l*std::sin(a)-1,1,gold,.65);
    }
    const V2 crew[5]={{970,865},{1100,915},{1230,876},{1370,945},{1510,831}};
    for(int i=0;i<5;++i) {
        const double x=crew[i].x,y=crew[i].y;
        const double side=i%2?1:-1;
        cv.capsule({x,y+7},{x-3*side,y-19},7.5,9,ink);
        cv.capsule({x-3*side,y-24},{x-3*side,y-32},5,5.2,ink);
        cv.capsule({x-8,y-16},{x-22,y-3},3.5,3,ink);
        cv.capsule({x-22,y-3},{x-35,y+5},3,2.5,ink);
        cv.capsule({x+6,y-16},{x+20,y-4},3.5,3,ink);
        cv.capsule({x+20,y-4},{x+32,y+6},3,2.5,ink);
        cv.line(x-44,y+9,x+44,y+11,8,wood);cv.line(x-40,y+7,x+40,y+9,1.7,gold,.75);
        cv.color(teal,.6);cv.moveTo(x-49,y+13);cv.curveTo(x-20,y+23,x+25,y+22,x+48,y+15);cv.stroke(1.7);
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
        cv.color(cream,erupt);cv.moveTo(150,1120);
        cv.curveTo(280,770,580,880,690,730);cv.curveTo(760,570,940,610,1080,720);
        cv.curveTo(1190,430,1430,445,1530,760);cv.curveTo(1710,640,1880,830,1990,1060);
        cv.lineTo(1990,1120);cv.closePath();cv.fill();
        // Structured branching crown. Dark channels expose depth inside
        // the dense white sheet; nested tips follow the outward velocity.
        for(int j=0;j<38;++j) {
            const double f=j/37.0,x=430+f*1460;
            const double y=920-160*std::sin(Pi*f)-140*std::sin(f*Pi*3)*std::sin(f*Pi*3);
            const double a=-2.35+f*1.7,ln=(120+170*hash2(j,213))*erupt;
            tendril(cv,{x+6,y+10},a,ln,12+15*hash2(j,214),.55,blue,erupt*.85);
            tendril(cv,{x,y},a,ln,9+10*hash2(j,214),.6,cream,erupt);
            const V2 fork={x+std::cos(a)*ln*.55,y+std::sin(a)*ln*.55};
            for(int b=0;b<3;++b)tendril(cv,fork,a+(b-1)*.45,ln*.45,4+2*hash2(j,b),.65,cream,erupt);
        }
        for(int j=0;j<27;++j) {
            const double x=340+j*60,y=1010-170*std::sin(j*.10);
            const double a=-1.4+.35*std::sin(j*.8);
            tendril(cv,{x,y},a,60+130*hash2(j,231),3+6*hash2(j,232),.6,teal,erupt*.65);
            tendril(cv,{x+4,y},a+.18,50+100*hash2(j,231),2+4*hash2(j,232),.8,blue,erupt*.5);
        }
        for(int j=0;j<12;++j) {
            cv.color(teal,erupt*.45);cv.moveTo(150+j*80,1060);
            cv.curveTo(600+j*60,950,1100+j*30,1050,2010,900+j*17);cv.stroke(1.4+j*.1);
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
    // The leading wash travels toward the foreground, then drains to lace.
    for(int k=0;k<9;++k) {
        const double u=age-.12*k;if(u<0)continue;
        const double y=790+125*u+14*k,alpha=(1-sstep(1.2,3.8,u))*.65;
        if(y>1150)continue;
        cv.color(cream,alpha);cv.moveTo(-100,y);
        cv.curveTo(470,y-45,1080,y+40,2020,y-20);cv.stroke(lerp(28,3,sstep(.4,2.8,u)));
        cv.color(teal,alpha*.8);cv.moveTo(-100,y+16);cv.curveTo(440,y-25,1310,y+43,2020,y+3);cv.stroke(2.0);
    }
    c.gpu.over(cv);
    if(age<.22) {Canvas& flash=c.canvas();flash.fillRect(0,0,1920,1080,cream,.75*window(age,.025,.035,.13,.07));c.gpu.over(flash);}
}
void flyingTimbers(Ctx& c,double age) {
    if(age<0 || age>2.5)return;
    Canvas& cv=c.canvas();
    for(int i=0;i<21;++i) {
        const double vx=(hash2(i,240)-.5)*650,vy=-260-380*hash2(i,241),a=age;
        V2 p={1490+vx*a,850+vy*a+260*a*a};
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

void drawEndingWater(Ctx& c) {
    const double impactAt=kanagawaBreak(c).t+.60,age=c.t-impactAt;
    kanagawaSea(c);studySea(c,age>=0);
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
        return BoatPose{lerp(entry.at,target,join),lerp(entry.tilt,tilt,join)};
    };
    if(age<.65)studyWave(c,growth);
    BoatPose far=pose(false),near=pose(true);
    const double rescue=sstep(impactAt+2,72,c.t),recede=sstep(71.5,76,c.t);
    far.at=lerp(far.at,V2(1330,808),rescue);
    far.at=lerp(far.at,V2(380,810),recede);
    studyBoat(c,far.at,lerp(.57,.40,recede),far.tilt*(1-.7*rescue),false,rescue>.5);
    if(age<0)studyBoat(c,near.at,.90,near.tilt,true);
    else {
        // Broken halves are exposed before/after the dense spray crown.
        if(age>.08)studyWreck(c);
        impact(c,age);flyingTimbers(c,age);
    }
    // The lantern-bearer remains visible as the surge carries him toward shore.
    if(age>.8 && c.t<70.7) {
        Canvas& body=c.canvas();Canvas& light=c.canvas();
        const double u=sstep(impactAt+.8,70.7,c.t);
        const V2 p=lerp(V2(1560,932),V2(1100,1010),u);
        body.save();body.translate(p.x,p.y);body.rotate(-.6+.2*std::sin(c.t*2));
        RigIn r;r.h=105;r.hat=true;r.garment=Garment::Jacket;r.hip={0,-30};r.facing=1;
        r.footF={30,8};r.footB={-20,10};r.handF={40,-63};r.handB={-26,-27};
        drawBody(body,solve(r),ink);body.restore();c.gpu.over(body);
        V2 hand=p+V2(30,-65);drawBearerLantern(light,light,hand,.15,105.0/146,1);
        c.gpu.over(light,1.3);c.gpu.add(light,.2,8);
    }
}
void drawJourneyEnding(Ctx& c) {
    c.cameraX=0;
    kanagawaSky(c);kanagawaSun(c);kanagawaFuji(c,1);
    drawEndingWater(c);
    FinishParams f;f.time=float(c.t);f.paper=1;f.grain=.008;f.bloom=.28;f.vignette=.25;
    Canvas& title=c.canvas();drawTitle(title,1,window(c.t,48.6,.8,53.8,1.2),hex(0x7e9cd8),cream);
    c.gpu.finish(f,&title);
}
}
