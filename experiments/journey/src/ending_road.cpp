// Live ending, using the road and low fastback authored in direction_study.
// All choreography is keyed in actual movie seconds, with no playback state.
#include "ending_road.h"
#include "parts.h"
#include "rig.h"
#include <array>
#include <vector>

namespace Journey {
namespace {
const Col ink=hex(0x121827),teal=hex(0x638da2),cream=hex(0xe6ddbf);
const Col gold=hex(0xe3b87a),violet=hex(0x75638e);
V2 unit(V2 p) {return p*(1/std::max(.001,p.len()));}
V2 rotate(V2 p,double a) {return {p.x*std::cos(a)-p.y*std::sin(a),p.x*std::sin(a)+p.y*std::cos(a)};}
void polyfill(Canvas& cv,const std::vector<V2>& p,Col col,double alpha=1) {
    if(p.empty())return;
    cv.color(col,alpha);cv.moveTo(p[0].x,p[0].y);
    for(std::size_t i=1;i<p.size();++i)cv.lineTo(p[i].x,p[i].y);
    cv.closePath();cv.fill();
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
// The same sampled curve drives the road edges, parked pose, contact points,
// departure and distance-dependent size. u is an index in this fixed path.
struct Road {
    std::vector<V2> center;
    std::vector<double> length;
    Road():center(smooth({{80,1120},{620,1010},{1270,931},{1660,800},
                         {1490,683},{1770,590},{2130,540}})) {
        length.push_back(0);
        for(std::size_t i=1;i<center.size();++i)
            length.push_back(length.back()+(center[i]-center[i-1]).len());
    }
    V2 at(double u) const {
        u=std::clamp(u,0.0,double(center.size()-1));
        const int i=std::min(int(u),int(center.size())-2);
        return lerp(center[i],center[i+1],u-i);
    }
    double width(double u) const {return 3.5+145*std::pow(1-u/(center.size()-1),2.45);}
    V2 tangent(double u) const {return unit(at(u+.35)-at(u-.35));}
    double travelled(double u) const {
        const int i=std::min(int(u),int(length.size())-2);
        return lerp(length[i],length[i+1],u-i);
    }
};
const Road& road() {static const Road r;return r;}
constexpr double parked=72.4;
struct CarPose {V2 at;double scale,angle,door,wheel;};
CarPose carPose(double t) {
    const Road& r=road();
    // Ease through both bends and settle at the distant exit for the title.
    // The car stays visible; reaching the hold never removes it.
    const double d=std::clamp(t-78.0,0.0,5.0);
    const double advance=d<3 ? 15*d*d-1.6*d*d*d : 91.8+46.8*(d-3);
    const double u=parked+91.8*sstep(0,91.8,advance);
    const V2 tangent=r.tangent(u),normal(-tangent.y,tangent.x);
    const double s=r.width(u)/r.width(parked);
    // The study car rests in the far half of the near road. The lane offset
    // shrinks with the road, then merges to center before the hairpin. The
    // study road compresses very quickly there: additional foreshortening
    // keeps the complete wheelbase on asphalt, with a readable distant floor.
    const V2 at=r.at(u)-normal*(23*s*(1-sstep(78,79,t)));
    const double angle=std::atan2(tangent.y,tangent.x)-std::atan2(-.23,.94);
    const double open=sstep(74.6,75.7,t)*(1-sstep(77.3,78.0,t));
    // Integrate distance / projected tyre radius along the fixed road. This
    // preserves rolling contact even while perspective continuously shrinks.
    double wheel=0;
    for(double from=parked;from<u;) {
        const double to=std::min(u,std::floor(from)+1);
        const double ratio=r.width((from+to)*.5)/r.width(parked);
        const double radius=.345*110*(.075+.925*std::pow(ratio,4.5));
        wheel+=(r.travelled(to)-r.travelled(from))/radius;from=to;
    }
    return {at,110*(.075+.925*std::pow(s,4.5)),angle,Pi*.32*open,wheel};
}
V2 carPoint(const CarPose& p,double x,double y,double z) {
    return p.at+rotate({.94*x+.40*y,-.23*x-.24*y},p.angle)*p.scale+V2(0,-p.scale*z);
}
// Ground under the walker's ankles follows the foreground shoulder, rather
// than sliding a flat gait up a sloping road. Only the swing foot leaves it.
double walkingGround(double x) {
    const auto& r=road();
    double best=1e9,y=1020;
    for(int i=43;i<73;++i) {
        const V2 a=r.center[i],b=r.center[i+1];
        if(x<a.x || x>b.x)continue;
        const double k=(x-a.x)/std::max(.001,b.x-a.x),u=i+k;
        const V2 q=lerp(a,b,k),d=r.tangent(u);
        const double yy=q.y+d.x*r.width(u)*.68;
        if(std::abs(q.x-x)<best){best=std::abs(q.x-x);y=yy;}
    }
    return y;
}
struct Bearer {
    RigIn rig;
    V2 lanternHand;
    double lanternSize=1,depth=10,swing=0,bright=1;
    bool visible=false,grounded=false;
};
V2 actualHand(const Body& b) {
    const V2 d=b.in.handF-b.elbowF;
    return b.elbowF+d*std::min(1.0,.16*b.in.h/std::max(.001,d.len()));
}
RigIn blendRig(const RigIn& a,const RigIn& b,double k) {
    RigIn r=a;r.h=lerp(a.h,b.h,k);r.hip=lerp(a.hip,b.hip,k);
    r.lean=lerp(a.lean,b.lean,k);r.headTilt=lerp(a.headTilt,b.headTilt,k);
    r.footF=lerp(a.footF,b.footF,k);r.footB=lerp(a.footB,b.footB,k);
    r.handF=lerp(a.handF,b.handF,k);r.handB=lerp(a.handB,b.handB,k);
    return r;
}
Bearer bearerAt(const Ctx& c,const CarPose& car) {
    Bearer out;if(c.t<70.7)return out;
    out.visible=true;
    const double h=155,ground=walkingGround(1100);
    RigIn lying;lying.h=h;lying.hat=true;lying.garment=Garment::Jacket;lying.facing=1;
    lying.hip={1100,ground-17};lying.lean=-1.42;
    lying.footF={1174,ground-2};lying.footB={1160,ground+1};
    lying.handF={1050,ground-3};lying.handB={1085,ground+1};
    RigIn kneel=lying;kneel.hip={1105,ground-47};kneel.lean=-.75;
    kneel.footF={1142,ground-1};kneel.footB={1095,ground};
    kneel.handF={1071,ground-18};kneel.handB={1082,ground};
    RigIn stood=lying;stood.hip={1100,ground-hipHeight(h)};stood.lean=.03;
    stood.footF={1116.5,walkingGround(1116.5)};stood.footB={1089,walkingGround(1089)};
    stood.handF={1128,stood.hip.y+3};stood.handB={1079,stood.hip.y+4};
    if(c.t<71.45) {
        // Exact water pose at the 70.7 handoff, rolled onto the bank without
        // a frame boundary jump in scale, head, ankles or wrists.
        const V2 p=endingBearerWashAt(70.7);
        const double roll=-.6+.2*std::sin(70.7*2);
        RigIn washed=lying;washed.h=105;washed.lean=roll;
        washed.hip=p+rotate({0,-30},roll);
        washed.footF=p+rotate({30,8},roll);washed.footB=p+rotate({-20,10},roll);
        washed.handF=p+rotate({40,-63},roll);washed.handB=p+rotate({-26,-27},roll);
        out.rig=blendRig(washed,lying,sstep(70.7,71.2,c.t));
        out.grounded=c.t>=71.1;
    } else if(c.t<72.25) {
        out.rig=blendRig(lying,kneel,sstep(71.45,72.25,c.t));out.grounded=true;
    } else if(c.t<73) {
        out.rig=blendRig(kneel,stood,sstep(72.25,73,c.t));out.grounded=true;
    } else {
        // Two complete strides finish with both feet planted.
        const double distance=110*sstep(73,75.8,c.t),x=1100+distance;
        Gait gait;gait.stride=55;gait.lift=7;gait.bob=2;
        const Steps st=gaitAt(1100,distance,0,1,gait,1);
        out.rig=stood;out.rig.hip={x,walkingGround(x)-hipHeight(h)+st.hipBob};
        out.rig.footF={st.footF.x,walkingGround(st.footF.x)+st.footF.y};
        out.rig.footB={st.footB.x,walkingGround(st.footB.x)+st.footB.y};
        out.rig.handF={x+28,out.rig.hip.y+3};
        out.rig.handB={x-20+5*std::sin(Tau*st.phase),out.rig.hip.y+5};
        // Lower the hip only as much as IK requires. Stance ankles stay fixed.
        for(V2 foot:{out.rig.footF,out.rig.footB}) {
            const double dx=foot.x-out.rig.hip.x,leg=.505*h;
            out.rig.hip.y=std::max(out.rig.hip.y,foot.y-std::sqrt(std::max(0.0,leg*leg-dx*dx)));
        }
        out.grounded=true;
        if(c.t>=75.8) {
            RigIn duck=out.rig;
            const V2 sill=carPoint(car,-.73,-1.04,.34);
            duck.hip=sill+V2(-34,-12);duck.lean=.9;duck.headTilt=-.2;
            duck.footF=sill+V2(-7,2);duck.footB=out.rig.footB;
            duck.handF=carPoint(car,-.20,-.94,.71);duck.handB=sill+V2(-5,-20);
            RigIn seated=duck;seated.h=130*car.scale/110;seated.hat=true;
            seated.hip=carPoint(car,-.43,-.64,.48);seated.lean=.17;
            seated.footF=carPoint(car,.34,-.65,.34);
            seated.footB=carPoint(car,.26,-.47,.34);
            seated.handF=carPoint(car,-.08,-.49,.66);
            seated.handB=carPoint(car,.16,-.62,.74);
            out.rig=c.t<76.45?blendRig(out.rig,duck,sstep(75.8,76.45,c.t))
                                  :blendRig(duck,seated,sstep(76.45,77.3,c.t));
            out.grounded=c.t<76.45;
            const double inside=sstep(76.05,77.3,c.t);
            // Sorted between the interior and near door/sill, with no fade or
            // removed pose. The lantern follows the same wrist into the cabin.
            out.depth=lerp(2.25,.65,inside);
        }
    }
    const Body body=solve(out.rig);
    out.lanternHand=actualHand(body);out.lanternSize=out.rig.h/146;
    // Continuous path from the exact last water lantern to its resting
    // position on the bank, then into the recovered bearer's hand.
    if(c.t<71.2)
        out.lanternHand=lerp(endingBearerWashAt(70.7)+V2(30,-65),
                             {1049,ground-38},sstep(70.7,71.2,c.t));
    else if(c.t<73)
        out.lanternHand=lerp({1049,ground-38},out.lanternHand,sstep(72.35,73,c.t));
    out.swing=.055*std::sin(c.t*2.1)*(1-sstep(75.8,77.3,c.t));
    out.bright=1+.12*(c.score?Score::envelope(c.score->onsets,c.t,5):0);
    return out;
}
void drawBearer(Ctx& c,const Bearer& bearer) {
    if(!bearer.visible)return;
    Canvas& cv=c.canvas();
    if(bearer.grounded) {
        for(V2 f:{bearer.rig.footF,bearer.rig.footB})
            cv.glowEllipse(f.x,f.y+3,11,2.7,ink,.35);
    }
    const Body b=solve(bearer.rig);drawBody(cv,b,ink);
    const double s=bearer.rig.h/155;
    cv.line(b.shoulder.x,b.shoulder.y,b.chest.x+5*s,b.chest.y+15*s,2.2*s,teal,.65);
    cv.line(b.head.x-18*s,b.head.y-9*s,b.head.x+20*s,b.head.y-9*s,1.1*s,gold,.6);
    c.gpu.over(cv);
    Canvas& light=c.canvas();
    drawBearerLantern(light,light,bearer.lanternHand,bearer.swing,bearer.lanternSize,bearer.bright);
    c.gpu.over(light,1.15);c.gpu.add(light,.15,float(8*s));
}
void roadLandscape(Ctx& c) {
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
    const auto& center=road().center;
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

void endingCar(Ctx& c, const CarPose& pose, const Bearer& bearer) {
    // One coherent model-space coupe. Every panel, wheel and door uses the
    // same projection; proportions are a low roof, long hood and wide track.
    struct P {double x,y,z;};
    struct Face {std::vector<P> pts;Col color;bool metal=false;double alpha=1;};
    std::vector<Face> faces;
    auto project=[&](P p) {
        const V2 ground=rotate({.94*p.x+.40*p.y,-.23*p.x-.24*p.y},pose.angle);
        return pose.at+ground*pose.scale+V2(0,-pose.scale*p.z);
    };
    auto depth=[&](P p) {
        // Turn the same coupe around the bend in its ground plane. Camera
        // ordering turns with it; verticals stay vertical throughout.
        return (.40*std::cos(pose.angle)+.94*std::sin(pose.angle))*p.x
             + (.40*std::sin(pose.angle)-.94*std::cos(pose.angle))*p.y+.03*p.z;
    };
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
        const V2 u=project(a),v=project(b);cv.line(u.x,u.y,v.x,v.y,w*pose.scale/110,color,alpha);
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
            const double a=j*Tau/5+pose.wheel;
            std::vector<V2> spoke;
            for(auto q:std::array<V2,4>{{{.06,a-.35},{.205,a-.14},{.205,a+.12},{.07,a+.32}}})
                spoke.push_back(project({x+q.x*std::cos(q.y),y-.027,.33+q.x*std::sin(q.y)}));
            polyfill(cv,spoke,chrome);
        }
        ring(.066,hex(0x727b94),y-.030);
        const V2 center=project({x,y-.031,.33});cv.disc(center.x,center.y,3.4*pose.scale/110,trim);
        // A fine rim highlight catches the purple sky, not a white cartoon ring.
        std::vector<V2> edge;
        for(int j=0;j<=28;++j) {
            const double a=.14+j*Pi/28;
            edge.push_back(project({x+.251*std::cos(a),y-.032,.33+.251*std::sin(a)}));
        }
        cv.polyline(edge,pose.scale/110,hex(0xc0a6cc),.72);
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
    // Quiet twin rear lamps remain legible as the car rounds the bend.
    panel({{-2.31,-.75,.39},{-2.31,-.40,.39},{-2.31,-.40,.51},{-2.31,-.75,.51}},hex(0xc57983));
    panel({{-2.31,.40,.39},{-2.31,.75,.39},{-2.31,.75,.51},{-2.31,.40,.51}},hex(0xc57983));
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
    auto door=[&](P p) {
        const double a=pose.door;constexpr double hx=.69,hy=-.91;
        const double x=p.x-hx,y=p.y-hy;
        return P{hx+x*std::cos(a)-y*std::sin(a),hy+x*std::sin(a)+y*std::cos(a),p.z};
    };
    auto doorPanel=[&](std::initializer_list<P> pts,Col col,bool metal=false) {
        std::vector<P> p;for(P q:pts)p.push_back(door(q));faces.push_back({p,col,metal});
    };
    doorPanel({{-.99,-.91,.32},{.69,-.91,.32},{.64,-.91,.73},{-.99,-.91,.72}},paint,true);
    doorPanel({{-.92,-.918,.39},{.57,-.918,.39},{.54,-.918,.67},{-.92,-.918,.66}},hex(0x343047));
    doorPanel({{-.86,-.922,.42},{.45,-.922,.42},{.44,-.922,.54},{-.86,-.922,.54}},seat);
    doorPanel({{-.99,-.91,.72},{.64,-.91,.73},{.51,-.89,.77},{-.91,-.89,.76}},trim);
    doorPanel({{.64,-.91,.73},{-.06,-.64,1.19},{-.10,-.675,1.13},{.51,-.89,.77}},trim);
    doorPanel({{-.06,-.64,1.19},{-.99,-.64,1.17},{-.91,-.675,1.11},{-.10,-.675,1.13}},trim);
    doorPanel({{-.99,-.64,1.17},{-.99,-.91,.72},{-.91,-.89,.76},{-.91,-.675,1.11}},trim);
    doorPanel({{-.91,-.89,.76},{.51,-.89,.77},{-.10,-.675,1.13},{-.91,-.675,1.11}},glass);
    // The glazing stays transparent enough to retain the rescued passenger
    // and warm lantern once the door is shut.
    faces.back().alpha=.38+.40*clamp01(pose.door/(Pi*.32));
    // Sort all panels together, including the open door, by camera depth.
    std::stable_sort(faces.begin(),faces.end(),[&](const Face& a,const Face& b) {
        double aa=0,bb=0;for(P p:a.pts)aa+=depth(p);for(P p:b.pts)bb+=depth(p);
        return aa/a.pts.size()<bb/b.pts.size();
    });
    bool drewBearer=false;
    const double bearerDepth=bearer.depth;
    auto occupant=[&] {
        if(drewBearer || !bearer.visible)return;
        c.gpu.over(cv);drawBearer(c,bearer);cv.reset(c.gpu.pixelScale());
        cv.translate(-c.cameraX,0);drewBearer=true;
    };
    for(const Face& f:faces) {
        double z=0;for(P q:f.pts)z+=depth(q);z/=f.pts.size();
        if(z>bearerDepth)occupant();
        std::vector<V2> p;for(P q:f.pts)p.push_back(project(q));
        if(f.metal) {
            double top=2000,bottom=0,x=0;for(V2 q:p){top=std::min(top,q.y);bottom=std::max(bottom,q.y);x+=q.x;}
            x/=p.size();
            cv.linear(x,top,x,bottom+1,{{0,mix(f.color,hex(0xbb9aba),.25),float(f.alpha)},
                {.48f,f.color,float(f.alpha)},{1,mix(f.color,trim,.25),float(f.alpha)}});
            cv.moveTo(p[0].x,p[0].y);for(V2 q:p)cv.lineTo(q.x,q.y);cv.closePath();cv.fill();
        } else polyfill(cv,p,f.color,f.alpha);
    }
    occupant();
    for(double x:{-1.43,1.48}) {
        const V2 foot=project({x,-.945,-.012});cv.glowEllipse(foot.x,foot.y,26*pose.scale/110,5*pose.scale/110,ink,.8);
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
    const V2 tip=project(door({.60,-1.12,.78}));
    cv.capsule(mp,tip,4*pose.scale/110,5*pose.scale/110,paint);
    cv.line(mp.x,mp.y-2*pose.scale/110,tip.x,tip.y-2*pose.scale/110,.9*pose.scale/110,chrome,.7);
    c.gpu.over(cv);
    Canvas& glow=c.canvas();
    const V2 poolA=project({2.8,-.55,.005});
    glow.glowEllipse(poolA.x,poolA.y,96*pose.scale/110,17*pose.scale/110,gold,.18);
    const V2 poolB=project({2.8,.55,.005});
    glow.glowEllipse(poolB.x,poolB.y,84*pose.scale/110,15*pose.scale/110,gold,.13);
    const V2 sill=project({-.40,-1.0,0});
    glow.glowEllipse(sill.x,sill.y,65*pose.scale/110,16*pose.scale/110,hex(0xd4a5ba),.12);
    c.gpu.add(glow,.4,float(12*pose.scale/110));
}

void washFoam(Ctx& c) {
    if(c.t<70.7 || c.t>72.5)return;
    Canvas& cv=c.canvas();const double a=1-sstep(71.1,72.5,c.t);
    const double ground=walkingGround(1100);
    for(int k=0;k<4;++k) {
        const double x=1095-k*27,y=ground+12+k*5;
        cv.color(cream,a*(.55-.08*k));cv.moveTo(x-84,y);
        cv.curveTo(x-36,y-12,x+39,y-15,x+77,y-7);
        cv.curveTo(x+30,y+5,x-33,y+10,x-84,y);cv.closePath();cv.fill();
        cv.color(teal,a*.5);cv.moveTo(x-57,y+3);
        cv.curveTo(x-17,y+8,x+38,y-5,x+56,y-3);cv.stroke(1.2);
    }
    c.gpu.over(cv);
}
void reveal(Ctx& c,int tex) {
    static const char* shader=R"(
        uniform sampler2D u_tex;
        uniform float u_arrival;
        void main() {
            vec2 p=design();
            float offset=mix(1920.0,0.0,u_arrival);
            vec2 uv=v_uv-vec2(offset/1920.0,0.0);
            o=(uv.x<0.0 || uv.x>1.0) ? vec4(0.0) : texture(u_tex,uv);
        }
    )";
    Program& p=c.gpu.effect("endingRoadArrival",shader);
    c.gpu.pass(p,Blend::Over,[&](Program& q) {
        c.gpu.bindTexture(0,tex,q,"u_tex");q.set("u_arrival",float(.58*sstep(69,70.7,c.t)+.42*sstep(70.7,73,c.t)));
    });
}
} // namespace

V2 endingBearerWashAt(double t) {
    // Shared with the water owner's 65.30 impact / 66.10 wash departure.
    return lerp(V2(1560,932),V2(1100,1010),sstep(66.1,70.7,t));
}

void drawEndingRoad(Ctx& c) {
    if(c.t<68.8)return;
    const CarPose car=carPose(c.t);const Bearer bearer=bearerAt(c,car);
    if(c.t<73)c.gpu.beginOffscreen(true);
    roadLandscape(c);
    // A quiet warm strip above the guardrail links the storm wash to the
    // violet slope without moving or tinting the untouched sea horizon.
    Canvas& bank=c.canvas();
    bank.color(mix(violet,gold,.35),.7);bank.moveTo(870,990);
    bank.curveTo(927,981,1022,960,1120,940);
    bank.curveTo(1060,984,960,1002,870,990);bank.closePath();bank.fill();
    c.gpu.over(bank);
    if(c.t<73)reveal(c,c.gpu.endOffscreen());
    washFoam(c);
    // Early bearer belongs to water until the explicit 70.7-second handoff.
    // Later, depth sorting inside the car owns the single boarding silhouette.
    if(c.t<75.8)drawBearer(c,bearer);
    if(c.t<73)c.gpu.beginOffscreen(true);
    endingCar(c,car,c.t>=75.8?bearer:Bearer{});
    if(c.t<73)reveal(c,c.gpu.endOffscreen());
}

void drawEndingRoadTitle(Canvas& overlay,double t) {
    // Owner passes this canvas to finish(). No title is baked into the scene.
    const double alpha=sstep(80.65,81,t);
    drawTitle(overlay,2,alpha,hex(0x7e9cd8),cream);
}
} // namespace Journey
