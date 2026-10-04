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
    const int mask=c.gpu.layer(body);c.gpu.over(body);
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
        void main(){vec4 a=texture(u_tex,v_uv);o=a*texture(u_mask,v_uv).a;}
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
    const V2 crew[4]={{970,865},{1100,915},{1230,876},{1510,831}};
    for(int i=0;i<4;++i) {
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
    const std::array<Col,4> colors={hex(0x625878),hex(0x635477),hex(0x514465),hex(0x302c49)};
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
    std::vector<V2> l,r;std::vector<double> width;
    for(int i=0;i<int(center.size());++i) {
        const double u=i/double(center.size()-1);
        const V2 before=center[i]-center[std::max(0,i-1)],after=center[std::min(i+1,int(center.size())-1)]-center[i];
        const double turn=std::abs(std::atan2(before.x*after.y-before.y*after.x,before.x*after.x+before.y*after.y));
        const double radius=(before.len()+after.len())*.5/std::max(.001,turn);
        const double w=std::min(lerp(145,3.5,std::pow(u,.78)),radius*.52);
        V2 d=unit(center[std::min(i+1,int(center.size())-1)]-center[std::max(0,i-1)]),n(-d.y,d.x);
        l.push_back(center[i]+n*w);r.push_back(center[i]-n*w);width.push_back(w);
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
            const double u=i/double(edge.size()-1),h=lerp(29,2.5,std::pow(u,.77));
            rail.push_back(edge[i]+V2(0,-h));
            if(i%7==0)cv.line(edge[i].x,edge[i].y,edge[i].x,edge[i].y-h,lerp(3,.6,u),hex(0x79758d));
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
    Canvas& cv=c.canvas();cv.save();cv.translate(1390,895);cv.scale(.92,.92);
    // Left rear three-quarter view; one front-hinged passenger door.
    cv.glowEllipse(-20,73,320,32,ink,.8);
    auto fill=[&](std::initializer_list<V2> pts,Col col) {polyfill(cv,std::vector<V2>(pts),col);};
    fill({{-292,8},{135,-42},{240,-20},{244,36},{-215,110},{-292,65}},hex(0x344154));
    cv.linear(-120,-45,120,95,{{0,hex(0x8699b1),1},{.48f,hex(0x5b708b),1},{1,hex(0x35445d),1}});
    cv.moveTo(-292,8);cv.lineTo(-208,-54);cv.lineTo(104,-71);cv.lineTo(242,-20);cv.lineTo(244,22);
    cv.lineTo(-205,98);cv.lineTo(-292,60);cv.closePath();cv.fill();
    // Rear face and bumper close the body consistently at the same corner.
    fill({{-292,8},{-205,35},{-205,100},{-292,66}},hex(0x455267));
    fill({{-292,60},{-205,87},{-205,99},{-292,73}},ink);
    cv.line(-291,59,-205,86,3,hex(0xa3a9b3),.8);
    fill({{-284,22},{-259,29},{-259,51},{-284,43}},hex(0xb54848));
    fill({{-234,35},{-210,41},{-210,65},{-234,58}},hex(0xbd4946));
    cv.line(-282,32,-260,39,2.3,gold,.65);cv.line(-230,48,-213,53,2.3,gold,.6);
    fill({{-253,33},{-238,38},{-238,60},{-253,54}},ink);
    // Wheels share the side's projected baseline and remain attached.
    for(int front=0;front<2;++front) {
        const double x=front?166:-158,y=front?43:96;
        cv.color(ink);cv.ellipse(x,y,29,37,-.10);cv.fill();
        cv.color(hex(0x838ea2));cv.ellipse(x+2,y,19,27,-.10);cv.fill();
        cv.color(hex(0x333e51));cv.ellipse(x+3,y,13,20,-.10);cv.fill();
        for(int j=0;j<8;++j)cv.line(x+3,y,x+3+16*std::cos(j*Tau/8),y+24*std::sin(j*Tau/8),1.4,hex(0xa4afbd));
        cv.disc(x+3,y,4,hex(0x6c788b));
    }
    // Roof, rear glass, far window and closed far-side door.
    fill({{-210,-51},{-137,-125},{42,-134},{104,-71}},hex(0x93a0b5));
    fill({{-207,-50},{-139,-118},{-58,-103},{-85,-31}},hex(0x1d283b));
    cv.line(-136,-120,-58,-103,2,gold,.5);cv.line(-201,-53,-85,-31,2,hex(0xa4b0c1));
    fill({{-128,-122},{39,-131},{108,-69},{-50,-94}},hex(0x768ca8));
    fill({{-48,-101},{32,-122},{90,-76},{-38,-43}},hex(0x212b40));
    fill({{-32,-104},{31,-112},{73,-79},{-26,-53}},hex(0x5d5060));
    cv.line(-42,-102,31,-122,2,gold,.75);
    // Warm empty passenger compartment and simple seat silhouette.
    fill({{-45,-40},{92,-76},{124,31},{-41,61}},hex(0x3f2e2d));
    fill({{-33,-27},{28,-51},{60,6},{-4,30}},hex(0xa97d60));
    fill({{-4,30},{62,5},{74,23},{8,50}},hex(0xc49c70));
    cv.line(-30,-25,23,-46,2,gold,.7);
    // Windshield and long wedge hood, no melted or ambiguous panels.
    fill({{40,-133},{104,-71},{153,-61},{92,-101}},hex(0x29384e));
    cv.line(42,-133,92,-101,2,hex(0xb0bac5));
    fill({{107,-69},{153,-61},{242,-20},{125,3}},hex(0x8294af));
    cv.line(117,-61,231,-22,2,gold,.7);cv.line(145,-49,206,-34,1,hex(0x546680));
    cv.line(241,-16,244,11,5,cream,.9);cv.line(244,23,211,29,5,ink);
    cv.line(-198,73,117,20,3,ink,.8);cv.line(-198,69,117,17,1.2,hex(0xc1c2c3),.7);
    cv.line(-167,89,-100,79,1.5,hex(0x8398b1));
    // Near door projects toward the viewer, front hinge exactly at cabin edge.
    fill({{92,-76},{160,-45},{180,68},{124,31}},hex(0x7088a5));
    fill({{99,-65},{155,-41},{173,59},{127,25}},hex(0x423b42));
    fill({{105,-6},{155,13},{166,52},{128,24}},hex(0xa48065));
    fill({{95,-78},{87,-125},{148,-105},{160,-46}},hex(0x28384f));
    fill({{98,-78},{92,-118},{143,-100},{153,-52}},hex(0x526078));
    cv.line(91,-122,147,-120,2,gold,.6);cv.line(160,-45,180,68,2,hex(0xb3c0cd),.8);
    cv.line(118,-12,138,-5,3,gold,.8);cv.line(128,9,150,17,3,hex(0x3c3438));
    cv.restore();c.gpu.over(cv);
    Canvas& light=c.canvas();
    light.glowEllipse(1515,935,84,19,gold,.23);light.glowEllipse(1610,878,115,28,gold,.17);
    c.gpu.add(light,.4,18);
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
