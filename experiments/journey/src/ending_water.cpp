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
    const double impactAt=kanagawaBreak(c).t+.60;
    const double release=impactAt-.90, plunge=std::max(0.0,(c.t-release)/.90);
    // Continuous acceleration carries the lip into the hit. It keeps moving
    // through contact, instead of rushing a quadratic fall in the last .6 s.
    const double forward=620*plunge*plunge/(plunge*plunge+.75);
    const double fall=850*plunge*plunge/(plunge+.65);
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
    for(int line=0;line<64;++line) {
        const int band=line%6;
        const double clock=c.score?c.score->integral(band,c.t):0;
        const double f=(line+.15*hash2(line,17)+.38*std::sin(clock*3+c.t*.3+line))/64.0;
        flow.color(mix(teal,cream,line%12==0?.45:.02),.10+.15*hash2(line,16)+.26*c.lift(band));
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
            const double u=(k+.3)/(seg==0?4:8);
            const V2 tangent=map(q.at(std::min(1.0,u+.005)))-map(q.at(std::max(0.0,u-.005)));
            const double ang=std::atan2(tangent.y,tangent.x);
            const V2 root=map(q.at(u));
            const double len=(55+95*hash2(k,seg+30))*(.05+.95*growth)*(1+.25*c.hit(4));
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
        const V2 tangent=map(crown[1].at(u+.005))-map(crown[1].at(std::max(0.0,u-.005)));
        const double ang=std::atan2(tangent.y,tangent.x);
        tendril(foam,q,ang-.9,25+30*hash2(k,82),5+4*hash2(k,83),-1.8,cream,.94);
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
        spray.line(p.x,p.y,p.x+speed,p.y+speed*.44,.6+hash2(i,65),cream,(.15+.7*c.hit(4))*(1-phase)*growth);
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
        cv.save();cv.translate(lerp(1385.0,850.0,settle)+half*lerp(300.0,470.0,settle)-950*pan,865+20*settle+half*28*settle);cv.rotate((half?.16:-.11)*sstep(0,.5,age));
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
        Canvas& sheet=c.canvas();
        // A spreading crown with ragged capillaries at its edge. The print
        // stays cream, but dark lacy channels break up the body of the foam.
        std::vector<V2> edge={{120,1140}};
        auto crownRadius=[&](double a){return 290+190*std::pow(std::sin(-a),2)+40*fbm1((a+Pi)*4.5,278);};
        for(int j=0;j<=300;++j) {
            const double a=-Pi+j*Pi/300;
            const double radius=crownRadius(a);
            edge.push_back({1260+radius*1.85*std::cos(a)*erupt,1040+radius*1.13*std::sin(a)*erupt});
        }
        edge.push_back({2120,1140});polyfill(sheet,edge,cream,erupt);
        const int mask=c.gpu.layer(sheet);
        Program& foam=c.gpu.effect("endingFoamLace",R"(
            uniform sampler2D u_tex;uniform float u_t,u_age;
            void main(){
                vec2 p=design();vec4 a=texture(u_tex,v_uv);
                vec2 q=vec2((p.x-1260.0)/160.0,(1040.0-p.y)/115.0);
                q.x+=.35*sin(q.y*1.7-u_age*2.0);
                float n=fbm(q*3.4+vec2(u_age*.45,-u_age*.8),4);
                float fine=fbm(q*12.0+vec2(u_age*.7,-u_age),3);
                float lace=1.0-smoothstep(.012,.028,abs(n-.50));
                float holes=smoothstep(.55,.70,n)*smoothstep(.50,.65,fine);
                vec3 col=mix(vec3(.88,.85,.73),vec3(.36,.52,.61),.14*lace+.20*holes+.08*smoothstep(650.0,1080.0,p.y));
                o=vec4(col*a.a,a.a);
            }
        )");
        c.gpu.pass(foam,Blend::Over,[&](Program& p){c.gpu.bindTexture(0,mask,p,"u_tex");p.set("u_age",float(age));p.set("u_t",float(c.t));});
        for(int j=0;j<44;++j) {
            const double a=-Pi+Pi*hash2(j,213),radius=crownRadius(a)-5;
            const V2 root={1260+radius*1.85*std::cos(a)*erupt,1040+radius*1.13*std::sin(a)*erupt};
            const double ln=(32+75*hash2(j,217))*erupt,w=5+7*hash2(j,218),curl=1.1+1.5*hash2(j,219);
            tendril(cv,root,a,ln,w,curl,cream,erupt);
            const V2 fork=root+V2(std::cos(a+curl*.2),std::sin(a+curl*.2))*ln*.4;
            tendril(cv,fork,a-.55,ln*.6,w*.35,-1.6,cream,erupt);
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
        const double alpha=(1-sstep(1.0,3.7,u))*.5;
        if(cy+radius*.24>1300)continue;
        std::vector<V2> front;
        for(int j=0;j<=72;++j) {
            const double a=j*Pi/72;
            front.push_back({1310+radius*std::cos(a),cy+radius*.24*std::sin(a)+5*std::sin(a*9-u*3)});
        }
        for(int j=1;j<=72;++j) {
            const double w=(3+13*(1-sstep(.3,2.3,u)))*std::pow(std::sin(j*Pi/73),.6);
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

void drawEndingWater(Ctx& c) {
    const double impactAt=kanagawaBreak(c).t+.60,age=c.t-impactAt;
    if(c.t<50)kanagawaSea(c);else kanagawaHorizon(c);
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
        return BoatPose{lerp(entry.at,target,join),lerp(entry.tilt,tilt,join)};
    };
    if(age<.65)studyWave(c,growth);
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
        const V2 p=lerp(V2(1560,932),V2(1100,1010),u);
        const double roll=-.6+.2*std::sin(c.t*2);
        auto rotate=[&](V2 q){return V2(q.x*std::cos(roll)-q.y*std::sin(roll),q.x*std::sin(roll)+q.y*std::cos(roll));};
        RigIn r;r.h=105;r.hat=true;r.garment=Garment::Jacket;r.hip=p+rotate({0,-30});r.facing=1;
        r.lean=roll;r.headTilt=roll;
        r.footF=p+rotate({30,8});r.footB=p+rotate({-20,10});
        r.handF=p+rotate({40,-63});r.handB=p+rotate({-26,-27});
        drawBody(body,solve(r),ink);c.gpu.over(body);
        V2 hand=p+V2(30,-65);drawBearerLantern(light,light,hand,.15,105.0/146,1);
        c.gpu.over(light,1.3);c.gpu.add(light,.2,8);
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
        const double y=477+j*2.4,travel=std::max(0.0,age)*(24+5*j);
        cv.color(cream,visibility*(.17-.015*j));
        cv.moveTo(1085+j*2,y);
        cv.curveTo(1107+travel*.35,y-8-j,1131+travel*.75,y-7-j,1150+travel,y-2-j);
        cv.stroke(1.1+.35*j);
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
