// Opt-in direction studies. These are live Canvas/GPU compositions, not
// raster concept art or a replacement for the accepted movie timeline.
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
            const double swell=aftermath ? 22*z*std::exp(-std::pow((x-1100)/560,2))*std::sin(x*.010-z*12) : 0;
            p.push_back({x,y+amp*std::sin(x*.0055+row*.82)+swell+2*noise1(x*.012,row)});
        }
        cv.polyline(p,row%7==0?1.65:.65,mix(teal,cream,.18),.11+.17*z*(row%3==0));
    }
    for(int i=0;i<115;++i) {
        const double y=620+350*hash2(i,5),x=1300+(hash2(i,6)-.5)*140*(y-600)/350;
        const double w=4+28*hash2(i,7);
        cv.line(x-w,y,x+w,y,1+hash2(i,8),i%3?gold:hex(0xc94a45),.25+.35*hash2(i,9));
    }
    c.gpu.over(cv);
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
    const double sx=lerp(.72,1.0,growth),sy=lerp(.48,1.0,growth),dx=lerp(-150,0,growth);
    auto map=[&](V2 p) {return V2(p.x*sx+dx,1080+(p.y-1080)*sy);};
    Canvas& body=c.canvas();
    body.save();body.translate(dx,1080);body.scale(sx,sy);body.translate(0,-1080);
    body.linear(270,150,720,1080,{{0,blue,1},{.42f,hex(0x274569),1},{.76f,navy,1},{1,ink,1}});
    body.moveTo(-180,1100);
    body.curveTo(-240,720,-100,275,220,130);
    body.curveTo(475,-25,825,65,990,245);
    body.curveTo(1145,395,1120,515,1000,560);
    body.curveTo(1090,465,985,360,850,390);
    body.curveTo(660,410,655,590,850,735);
    body.curveTo(1040,880,1230,875,1440,1040);
    body.curveTo(1040,1170,250,1190,-180,1100);body.closePath();body.fill();body.restore();
    const int mask=c.gpu.layer(body);
    Program& surface=c.gpu.effect("directionWaveFoot",R"(
        uniform sampler2D u_tex;
        void main(){o=texture(u_tex,v_uv)*(1.0-smoothstep(970.0,1100.0,design().y));}
    )");
    c.gpu.pass(surface,Blend::Over,[&](Program& p){c.gpu.bindTexture(0,mask,p,"u_tex");});
    Canvas& flow=c.canvas();
    flow.save();flow.translate(dx,1080);flow.scale(sx,sy);flow.translate(0,-1080);
    // Nested material curves share one family of flow coordinates. Their
    // order stays intact through the curl, so no spirograph crossings form.
    auto stream=[&](double f) {
        flow.moveTo(-155+f*1540,1120);
        flow.curveTo(-230+f*1170,825,-145+f*700,245+f*325,240+f*475,158+f*306);
        flow.curveTo(480+f*200,-1+f*456,840-f*58,81+f*350,993-f*88,280+f*244);
        flow.curveTo(1090-f*134,386+f*182,1096-f*210,485+f*104,1017-f*190,536+f*106);
    };
    // Thick shoulder shadow and inner-barrel light describe water's volume.
    flow.radial(460,400,680,{{0,hex(0x233856),.55f},{.6f,navy,.18f},{1,navy,0}});
    flow.rect(-200,0,1600,1120);flow.fill();
    flow.radial(870,534,245,{{0,teal,.40f},{.6f,blue,.14f},{1,blue,0}});
    flow.rect(420,250,900,750);flow.fill();
    for(int line=0;line<64;++line) {
        const double f=(line+.15*hash2(line,17))/64.0;
        flow.color(mix(teal,cream,line%12==0?.35:.02),.11+.20*hash2(line,16));
        stream(f);flow.stroke(line%12==0?2.1:.55+hash2(line,13)*.7);
    }
    // Broken flowing lace on the face. It follows these same curves and is
    // concentrated beside foam rather than covering the whole face.
    for(int line=0;line<12;++line) {
        const double f=(line+.5)/29.0;
        flow.color(cream,.12+.12*hash2(line,22));stream(f);flow.stroke(.8);
    }
    flow.restore();
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
            const double len=(55+95*hash2(k,seg+30))*(.55+.45*growth);
            for(int branch=0;branch<3;++branch) {
                const double a=ang+.95+branch*.37,ln=len*(.64+.19*branch),w=10+10*hash2(k*3+branch,seg);
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
        const double dist=20+150*hash2(i,67),speed=3+8*hash2(i,68);
        const V2 p=source+V2(-dist*.72,-dist*.60-15*hash2(i,69));
        spray.line(p.x,p.y,p.x+speed,p.y+speed*.44,.6+hash2(i,65),cream,.25+.6*hash2(i,66));
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
        const double x=-130+j*65,brace=1-sstep(65,69,c.t);
        const V2 hand={x+19,-23+8*brace},lock={x+27,13},tip={x+50+17*std::sin(c.t*.6+j*.22),79};
        cv.line(hand.x,hand.y,tip.x,tip.y,3,gold);
        cv.capsule(tip,tip+unit(tip-lock)*23,4.5,6,wood);
        cv.disc(lock.x,lock.y,2.5,ink);
        RigIn r;r.h=61;r.hip={x,-13};r.facing=1;r.garment=Garment::Jacket;r.hair=Hair::Short;
        r.lean=.34*brace;r.footF={x+26,-1};r.footB={x+6,1};r.handF=hand;r.handB={x+13,-25+8*brace};
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
    // Settled halves and scattered structural timbers, well separated crew.
    for(int half=0;half<2;++half) {
        cv.save();cv.translate(850+half*470,885+half*28);cv.rotate(half?.16:-.11);
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
    c.gpu.over(cv);
    Canvas& foam=c.canvas();
    for(int k=0;k<12;++k) {
        const double x=210+k*132,y=925+16*std::sin(k*.65);
        foam.color(cream,.6);foam.moveTo(x-65,y);foam.curveTo(x-20,y-22,x+48,y+12,x+82,y-10);foam.stroke(3.2);
        foam.color(teal,.4);foam.moveTo(x-30,y+16);foam.curveTo(x+16,y+6,x+54,y+23,x+96,y+10);foam.stroke(1.3);
    }
    c.gpu.over(foam);
}

std::vector<V2> smooth(const std::vector<V2>& p) {
    std::vector<V2> out;
    for(std::size_t i=0;i+1<p.size();++i) {
        const V2 a=i?p[i-1]:p[i]*2-p[i+1],b=p[i],d=p[i+1],e=i+2<p.size()?p[i+2]:d*2-b;
        for(int j=0;j<32;++j) {
            const double t=j/32.0;
            out.push_back((b*2+(d-a)*t+(a*2-b*5+d*4-e)*(t*t)+(b*3-a-d*3+e)*(t*t*t))*.5);
        }
    }
    out.push_back(p.back());return out;
}
void pine(Canvas& cv,V2 root,double h) {
    cv.line(root.x,root.y,root.x,root.y-h,1.3,ink);
    for(int j=0;j<4;++j) {
        const double y=root.y-h+j*h*.18,w=h*(.15+j*.045);
        polyfill(cv,{{root.x,y-3},{root.x-w,y+h*.31},{root.x-2,y+h*.24},
            {root.x+w*.85,y+h*.28}},ink);
    }
}
void studyRoad(Ctx& c) {
    Canvas& cv=c.canvas();
    // Layered curved hills with restrained geological seams and distant pines.
    const std::array<Col,4> colors={hex(0x8a749b),hex(0x71628b),hex(0x514767),hex(0x302c49)};
    for(int k=0;k<4;++k) {
        const double y=590+k*69;
        cv.linear(1200,y,1500,1080,{{0,colors[k],1},{1,mix(colors[k],ink,.30),1}});
        cv.moveTo(420+k*90,1080);cv.curveTo(700,y+270,1030,y+65,1390,y+12);
        cv.curveTo(1640,y-45,1810,y-100,2020,y-105);cv.lineTo(2020,1100);cv.closePath();cv.fill();
        for(int j=0;j<14;++j) {
            const double x=1010+j*74+hash2(j,k)*30,yy=y+140-std::sin(j*.20)*100;
            cv.color(mix(violet,gold,.17),.12);cv.moveTo(x,yy);cv.curveTo(x+28,yy-8,x+62,yy-1,x+96,yy-16);cv.stroke(.9);
            if(j%3==0)pine(cv,{x+20,yy-33},17+13*hash2(j,k));
        }
    }
    const auto center=smooth({{80,1120},{620,1010},{1270,931},{1660,800},{1490,683},{1770,590},{2130,540}});
    std::vector<V2> l,r;
    for(int i=0;i<int(center.size());++i) {
        const double u=i/double(center.size()-1);
        const double w=3.5+145*std::pow(1-u,2.45);
        V2 d=unit(center[std::min(i+1,int(center.size())-1)]-center[std::max(0,i-1)]),n(-d.y,d.x);
        l.push_back(center[i]+n*w);r.push_back(center[i]-n*w);
    }
    auto surface=l;for(auto it=r.rbegin();it!=r.rend();++it)surface.push_back(*it);
    polyfill(cv,surface,hex(0x242435));
    for(int i=1;i<int(center.size());++i) {
        const double u=i/double(center.size()-1);
        cv.line(l[i-1].x,l[i-1].y,l[i].x,l[i].y,lerp(2.8,.5,u),mix(violet,gold,.65),.8);
        cv.line(r[i-1].x,r[i-1].y,r[i].x,r[i].y,lerp(2.8,.5,u),cream,.65);
        if(i%9<4)cv.line(center[i-1].x,center[i-1].y,center[i].x,center[i].y,lerp(4,.6,u),gold,.75);
    }
    for(int side=0;side<2;++side) {
        const auto& edge=side?l:r;std::vector<V2> rail;
        for(int i=0;i<int(edge.size());++i) {
            const double u=i/double(edge.size()-1),h=29*std::pow(1-u,3.5)+1.5;
            rail.push_back(edge[i]+V2(0,-h));
            if(i%7==0 && i<85)cv.line(edge[i].x,edge[i].y,edge[i].x,edge[i].y-h,lerp(3,.6,u),hex(0x79758d));
            if(i>90 && i%27==0)cv.glow(edge[i].x,edge[i].y-h-2,7,gold,.55);
        }
        cv.polyline(rail,2.9,side?gold:hex(0x8b819b),.7);
    }
    for(int j=0;j<60;++j) {
        const double x=650+hash2(j,170)*1300,y=960+hash2(j,171)*120;
        cv.line(x,y,x+9,y-6-11*hash2(j,172),.8,violet,.3);
    }
    pine(cv,{1855,752},170);pine(cv,{1903,784},130);
    c.gpu.over(cv);
}

void studyCar(Ctx& c) {
    // One coherent model-space coupe. Every panel, wheel and door uses the
    // same projection; proportions are a low roof, long hood and wide track.
    struct P {double x,y,z;};
    struct Face {std::vector<P> pts;Col color;bool metal=false;double alpha=1;};
    std::vector<Face> faces;
    auto project=[](P p) {return V2(1405+110*(.94*p.x+.40*p.y),879+110*(-.23*p.x-.24*p.y-p.z));};
    auto depth=[](P p) {return .40*p.x-.94*p.y+.03*p.z;};
    auto panel=[&](std::initializer_list<P> p,Col color,bool metal=false,double alpha=1) {
        faces.push_back({std::vector<P>(p),color,metal,alpha});
    };
    const Col paint=hex(0x44496d),lit=hex(0x6f7299),trim=hex(0x101422),glass=hex(0x1b2037);
    const Col chrome=hex(0x9fa6bf),warm=hex(0xdbb78d),seat=hex(0x635065);
    Canvas& cv=c.canvas();
    std::vector<V2> shadow;
    for(P p:std::array<P,4>{{{-2.40,-1.02,0},{2.40,-1.02,0},{2.40,1.02,0},{-2.40,1.02,0}}})shadow.push_back(project(p));
    polyfill(cv,shadow,ink,.28);
    auto line=[&](P a,P b,double w,Col color,double alpha=1) {
        const V2 u=project(a),v=project(b);cv.line(u.x,u.y,v.x,v.y,w,color,alpha);
    };
    auto wheel=[&](double x,double y,bool near) {
        auto ring=[&](double radius,Col color,double yy) {
            std::vector<V2> p;
            for(int j=0;j<64;++j) {
                const double a=j*Tau/64;
                p.push_back(project({x+radius*std::cos(a),yy,.33+radius*std::sin(a)}));
            }
            polyfill(cv,p,color);
        };
        ring(.345,hex(0x080c16),y);
        if(!near)return;
        ring(.292,hex(0x20273a),y-.009);
        ring(.230,hex(0xadb0c3),y-.018);
        ring(.197,hex(0x282d43),y-.024);
        // Deep dish and clean five-spoke period alloys, all on the wheel plane.
        for(int j=0;j<5;++j) {
            const double a=j*Tau/5;
            std::vector<V2> spoke;
            for(auto q:std::array<V2,4>{{{.06,a-.35},{.205,a-.14},{.205,a+.12},{.07,a+.32}}})
                spoke.push_back(project({x+q.x*std::cos(q.y),y-.027,.33+q.x*std::sin(q.y)}));
            polyfill(cv,spoke,chrome);
        }
        ring(.066,hex(0x727b94),y-.030);
        const V2 center=project({x,y-.031,.33});cv.disc(center.x,center.y,3.4,trim);
        // A fine rim highlight catches the purple sky, not a white cartoon ring.
        std::vector<V2> edge;
        for(int j=0;j<=28;++j) {
            const double a=.14+j*Pi/28;
            edge.push_back(project({x+.251*std::cos(a),y-.032,.33+.251*std::sin(a)}));
        }
        cv.polyline(edge,1.0,hex(0xc0a6cc),.72);
    };
    // In this low near-side view the chassis occludes the far tyres.
    // The opening's dark interior is behind the seat and surrounding panels.
    std::vector<V2> doorway;
    for(P p:std::array<P,5>{{{-1.01,-.91,.32},{.69,-.91,.32},{.64,-.91,.73},{-.06,-.64,1.19},{-1.01,-.64,1.17}}})doorway.push_back(project(p));
    polyfill(cv,doorway,hex(0x161727));
    // Far side, rear deck, low broad wedge hood and rear face.
    panel({{-2.30,.88,.29},{2.32,.88,.29},{2.32,.88,.54},{.64,.88,.73},{-1.55,.88,.72},{-2.30,.88,.60}},paint,true);
    panel({{-2.30,-.88,.60},{-1.55,-.88,.72},{-1.55,.88,.72},{-2.30,.88,.60}},lit,true);
    panel({{.64,-.88,.73},{2.30,-.88,.55},{2.30,.88,.55},{.64,.88,.73}},lit,true);
    panel({{-2.30,-.88,.29},{-2.30,.88,.29},{-2.30,.88,.60},{-2.30,-.88,.60}},paint,true);
    // Near side is cut around the two wheel arches instead of painting wheels
    // over a rectangular body. A narrow sill gives it a planted low stance.
    std::vector<P> frontSide={{.69,-.90,.31},{.64,-.90,.73},{2.30,-.90,.55},{2.33,-.90,.29}};
    for(int j=0;j<=32;++j) {
        const double a=j*Pi/32;frontSide.push_back({1.48+.385*std::cos(a),-.90,.29+.385*std::sin(a)});
    }
    frontSide.push_back({.69,-.90,.31});faces.push_back({frontSide,paint,true});
    std::vector<P> backSide={{-2.30,-.90,.29},{-2.30,-.90,.60},{-1.55,-.90,.72},{-1.01,-.90,.72},{-1.01,-.90,.29}};
    for(int j=0;j<=32;++j) {
        const double a=j*Pi/32;backSide.push_back({-1.43+.385*std::cos(a),-.90,.29+.385*std::sin(a)});
    }
    backSide.push_back({-2.30,-.90,.29});faces.push_back({backSide,paint,true});
    panel({{-1.01,-.91,.29},{.69,-.91,.29},{.69,-.91,.34},{-1.01,-.91,.34}},paint,true);
    // Fastback glazing, a low cabin and defined pillars.
    panel({{-1.55,.87,.73},{-.89,.64,1.17},{-.06,.64,1.19},{.64,.87,.73}},paint,true);
    panel({{-1.55,.81,.74},{-.88,.59,1.14},{-.09,.59,1.16},{.55,.81,.75}},glass);
    panel({{-.89,-.64,1.17},{-.06,-.64,1.19},{-.06,.64,1.19},{-.89,.64,1.17}},lit,true);
    panel({{-1.55,-.88,.73},{-.89,-.64,1.17},{-.89,.64,1.17},{-1.55,.88,.73}},paint,true);
    panel({{-1.48,-.78,.76},{-.91,-.57,1.135},{-.91,.57,1.135},{-1.48,.78,.76}},glass);
    panel({{-.06,-.64,1.19},{.64,-.88,.73},{.64,.88,.73},{-.06,.64,1.19}},paint,true);
    panel({{.01,-.59,1.15},{.59,-.80,.75},{.59,.80,.75},{.01,.59,1.15}},glass);
    // Near rear quarter glass and passenger doorway stay physically separate.
    panel({{-1.55,-.90,.73},{-.89,-.64,1.17},{-1.02,-.64,1.17},{-1.02,-.90,.72}},paint,true);
    panel({{-1.46,-.873,.755},{-.95,-.655,1.10},{-1.08,-.66,1.10},{-1.09,-.873,.755}},glass);
    // The warm seat is compact and sits inside the cabin, not on the body side.
    panel({{-.60,-.76,.46},{-.60,-.37,.46},{-.77,-.37,.94},{-.77,-.76,.94}},seat);
    panel({{-.58,-.77,.47},{-.05,-.77,.47},{-.05,-.36,.47},{-.58,-.36,.47}},hex(0x8c6c77));
    panel({{.27,-.78,.70},{.58,-.80,.70},{.58,.80,.70},{.27,.74,.70}},trim);
    // Small black spoiler on the rear deck. It does not change the silhouette
    // into a racing caricature or use enormous contemporary bodywork.
    panel({{-2.19,-.89,.69},{-2.05,-.89,.71},{-2.05,.89,.71},{-2.19,.89,.69}},trim);
    panel({{-2.19,-.88,.60},{-2.19,-.88,.69},{-2.05,-.88,.71},{-2.05,-.88,.62}},paint);
    // Rectangular front bumper and period pop-up headlamps.
    panel({{2.33,-.90,.27},{2.33,.90,.27},{2.33,.90,.48},{2.33,-.90,.48}},trim);
    panel({{2.339,-.65,.31},{2.339,-.32,.31},{2.339,-.32,.40},{2.339,-.65,.40}},hex(0xa8a493));
    panel({{2.339,.32,.31},{2.339,.65,.31},{2.339,.65,.40},{2.339,.32,.40}},hex(0xa8a493));
    panel({{2.34,-.23,.31},{2.34,.23,.31},{2.34,.23,.41},{2.34,-.23,.41}},hex(0x293047));
    for(double y:{-.55,.55}) {
        panel({{1.73,y-.19,.61},{1.98,y-.19,.61},{1.98,y+.19,.61},{1.73,y+.19,.61}},paint,true);
        panel({{1.98,y-.19,.61},{1.98,y+.19,.61},{1.94,y+.19,.87},{1.94,y-.19,.87}},trim);
        panel({{1.985,y-.155,.655},{1.985,y+.155,.655},{1.955,y+.155,.83},{1.955,y-.155,.83}},cream);
        panel({{1.73,y-.19,.61},{1.94,y-.19,.87},{1.94,y+.19,.87},{1.73,y+.19,.61}},lit,true);
    }
    // One front-hinged passenger door, rotated in model space as a real panel.
    auto door=[](P p) {
        constexpr double a=Pi*.32,hx=.69,hy=-.91;
        const double x=p.x-hx,y=p.y-hy;
        return P{hx+x*std::cos(a)-y*std::sin(a),hy+x*std::sin(a)+y*std::cos(a),p.z};
    };
    auto doorPanel=[&](std::initializer_list<P> pts,Col col,bool metal=false) {
        std::vector<P> p;for(P q:pts)p.push_back(door(q));faces.push_back({p,col,metal});
    };
    doorPanel({{-.99,-.91,.32},{.69,-.91,.32},{.64,-.91,.73},{-.99,-.91,.72}},paint,true);
    doorPanel({{-.92,-.918,.39},{.57,-.918,.39},{.54,-.918,.67},{-.92,-.918,.66}},hex(0x343047));
    doorPanel({{-.86,-.922,.42},{.45,-.922,.42},{.44,-.922,.54},{-.86,-.922,.54}},seat);
    doorPanel({{-.99,-.91,.72},{.64,-.91,.73},{-.06,-.91,1.19},{-.99,-.91,1.17}},trim);
    doorPanel({{-.91,-.918,.76},{.51,-.918,.77},{-.10,-.918,1.13},{-.91,-.918,1.11}},hex(0x50526e),true);
    // Sort all panels together, including the open door, by camera depth.
    std::stable_sort(faces.begin(),faces.end(),[&](const Face& a,const Face& b) {
        double aa=0,bb=0;for(P p:a.pts)aa+=depth(p);for(P p:b.pts)bb+=depth(p);
        return aa/a.pts.size()<bb/b.pts.size();
    });
    for(const Face& f:faces) {
        std::vector<V2> p;for(P q:f.pts)p.push_back(project(q));
        if(f.metal) {
            double top=2000,bottom=0;for(V2 q:p){top=std::min(top,q.y);bottom=std::max(bottom,q.y);}
            cv.linear(1300,top,1360,bottom+1,{{0,mix(f.color,hex(0xbb9aba),.25),float(f.alpha)},
                {.48f,f.color,float(f.alpha)},{1,mix(f.color,trim,.25),float(f.alpha)}});
            cv.moveTo(p[0].x,p[0].y);for(V2 q:p)cv.lineTo(q.x,q.y);cv.closePath();cv.fill();
        } else polyfill(cv,p,f.color,f.alpha);
    }
    for(double x:{-1.43,1.48}) {
        const V2 foot=project({x,-.945,-.012});cv.glowEllipse(foot.x,foot.y,26,5,ink,.8);
    }
    wheel(-1.43,-.945,true);wheel(1.48,-.945,true);
    // Precisely projected trim and fine reflections, rather than heavy outlines.
    line({-2.24,-.905,.46},{-1.02,-.905,.46},2.1,trim,.8);
    line({.72,-.905,.46},{2.25,-.905,.43},2.1,trim,.8);
    line({-2.22,-.909,.49},{-1.02,-.909,.48},.85,chrome,.7);
    line({.72,-.909,.49},{2.24,-.909,.45},.85,chrome,.7);
    line({-.89,-.64,1.17},{-.06,-.64,1.19},1.2,warm,.85);
    line({-.06,-.64,1.19},{.64,-.88,.73},1.2,chrome,.8);
    line({.71,-.83,.73},{2.24,-.83,.56},1.1,warm,.75);
    line({.72,.80,.73},{2.24,.80,.56},.7,chrome,.5);
    line({-1.42,-.78,.78},{-.94,-.57,1.12},1.0,hex(0xc0a0ba),.55);
    line({.04,-.50,1.13},{.43,-.67,.80},.9,hex(0x8887ac),.65);
    line({.83,-.57,.72},{1.64,-.57,.64},.9,trim,.6);
    line({.83,.57,.72},{1.64,.57,.64},.9,trim,.6);
    line({2.345,-.86,.46},{2.345,.86,.46},1.2,chrome,.55);
    line(door({-.94,-.924,.74}),door({.56,-.924,.75}),1.0,warm,.8);
    line(door({-.88,-.925,.62}),door({-.56,-.925,.62}),2.8,chrome,.85);
    line(door({-.78,-.925,.55}),door({.23,-.925,.55}),3.8,trim,.9);
    // Door mirror is attached near the hinge and follows the same rotation.
    const P m=door({.48,-1.02,.78});const V2 mp=project(m);
    cv.capsule(mp,mp+V2(15,-1),4,5,paint);cv.line(mp.x+5,mp.y-2,mp.x+14,mp.y-3,.9,chrome,.7);
    c.gpu.over(cv);
    Canvas& glow=c.canvas();
    const V2 lampA=project({1.985,-.55,.74}),lampB=project({1.985,.55,.74});
    glow.glowEllipse(lampA.x+55,lampA.y+65,96,17,gold,.18);
    glow.glowEllipse(lampB.x+55,lampB.y+65,84,15,gold,.13);
    glow.glowEllipse(1250,957,65,16,hex(0xd4a5ba),.12);
    c.gpu.add(glow,.4,12);
}
void studyWalker(Ctx& c) {
    const double h=155,x=1055,y=1004,distance=29;
    Gait g;g.stride=55;g.lift=7;g.bob=2;
    const Steps st=gaitAt(x-distance,distance,y,1,g,1);
    RigIn r;r.h=h;r.hip={x,y-hipHeight(h)+st.hipBob};r.hat=true;r.garment=Garment::Jacket;r.facing=1;r.lean=.04;
    r.footF=st.footF;r.footB=st.footB;r.handF={x+34,r.hip.y+4};r.handB={x-19,r.hip.y+6};
    for(V2 foot:{r.footF,r.footB}) {
        const double dx=foot.x-r.hip.x,leg=.505*h;
        r.hip.y=std::max(r.hip.y,foot.y-std::sqrt(std::max(0.0,leg*leg-dx*dx)));
    }
    Canvas& cv=c.canvas();const Body b=solve(r);drawBody(cv,b,ink);
    cv.line(b.shoulder.x,b.shoulder.y,b.chest.x+7,b.chest.y+18,2.8,teal,.65);
    cv.line(b.chest.x+7,b.chest.y+18,r.hip.x+9,r.hip.y+13,1.7,violet,.7);
    // Straw hat bands and a sleeve catching the lantern.
    cv.line(b.head.x-19,b.head.y-9,b.head.x+22,b.head.y-9,1.3,gold,.7);
    cv.line(b.elbowF.x,b.elbowF.y,r.handF.x,r.handF.y,2,gold,.6);c.gpu.over(cv);
    Canvas& light=c.canvas();drawBearerLantern(light,light,r.handF,0,h/146,1.0);c.gpu.over(light,1.25);c.gpu.add(light,.2,10);
}
}

void drawDirectionStudy(Ctx& c) {
    c.cameraX=0;
    kanagawaSky(c);kanagawaSun(c);kanagawaFuji(c,1);
    const bool road=c.t>=74,after=c.t>=66;
    studySea(c,after);
    if(road) {
        // Small rescue remains visible below the road, closing the water story.
        studyBoat(c,{380,810},.40,.025,false,true);
        Canvas& distant=c.canvas();distant.capsule({474,829},{474,814},3.5,4,ink);
        distant.disc(474,807,3.5,ink);distant.line(454,833,496,834,4,wood);
        distant.line(453,837,497,838,1,cream,.5);
        distant.line(250,861,317,865,5,wood);distant.line(269,857,311,860,1.2,gold,.6);c.gpu.over(distant);
        studyRoad(c);studyCar(c);studyWalker(c);
    } else if(after) {
        studyWreck(c);
        // The rear boat has approached the survivors after riding the surge.
        studyBoat(c,{1330,808},.80,.045,false,true);
        Canvas& wake=c.canvas();wake.color(cream,.5);wake.moveTo(1545,847);
        wake.curveTo(1650,861,1740,844,1810,854);wake.stroke(2);c.gpu.over(wake);
    } else {
        const double growth=sstep(54,64.2,c.t);
        studyWave(c,growth);
        studyBoat(c,{1645,727},.57,-.035-.025*growth,false);
        studyBoat(c,{1535,908-42*growth},.90,-.02-.08*growth,true);
    }
    FinishParams f;f.time=float(c.t);f.paper=1;f.grain=.008;f.bloom=.28;f.vignette=.25;
    c.gpu.finish(f,nullptr);
}
}
