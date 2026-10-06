#include "print-life.h"
namespace Journey::Kit { namespace {
double printWater(const Ctx&c,const PrintLifeParametersV1&p,double x) {
 if(p.followsBoat)return BoatOnWaterV1::surfaceY(c,p.boat,x,BoatOnWaterV1::pose(c,p.boat).row);
 const double row=std::clamp(p.row,0.,double(p.swell.rows-1));const int lo=std::min(p.swell.rows-2,int(row));
 return lerp(SwellLinesV1::field(c,lo,p.swell).y(x),SwellLinesV1::field(c,lo+1,p.swell).y(x),row-lo);
}
} }
namespace Journey::Kit {
void SmokePlumeV1::paint(Canvas&cv,const Ctx&c,const PrintLifeParametersV1&p){
    const double bass=p.gain*(c.score?c.score->bandBody[1][p.band]:c.band(p.band));
    const double surge=p.surgeEnabled && c.schedule?c.schedule->print.surge(c.t):0;
    const double drift=c.t*.25*p.speed+(c.score?c.score->bandIntegrals[p.band]*.35:0);
    // Long tapered ribbons begin at the vent. Their drift is continuous and
    // their far ends dissolve into the printed sky instead of puff cycling.
    for(int i=0;i<std::min(p.count,5);++i){std::vector<V2> points;
        for(int k=0;k<=24;++k){double u=k/24.,y=p.y-u*p.height*(.55+.09*i+.25*bass),x=p.x+u*p.width*(.35+.7*bass)+std::sin(u*7-drift+i*.3)*u*(8+18*bass);
            points.push_back({x,y});}
        // One flat tapered ribbon per plume strand: crisp edges, fading only
        // by width toward its tail, never stacked soft strokes.
        std::vector<V2> l,r;
        for(int k=0;k<=24;++k){const double u=k/24.;const V2 t=points[std::min(k+1,24)]-points[std::max(k-1,0)];
            const double len=std::max(1e-6,std::hypot(t.x,t.y));const V2 n{-t.y/len,t.x/len};
            const double w=(1.2+5.5*std::sin(Pi*std::min(1.,u*1.25))+4*bass)*p.scale*(1-.75*u);
            l.push_back(points[k]+n*w);r.push_back(points[k]-n*w);}
        const Col smoke=mix(p.color,hex(0x54546d),.25);const double alpha=(.50+.25*bass)*(1-.25*i);
        for(int k=0;k<24;++k){cv.tri(l[k],l[k+1],r[k+1],smoke,alpha);cv.tri(l[k],r[k+1],r[k],smoke,alpha);}}
    const double beat=c.kick(5);cv.color(p.ink,.25+.55*beat);cv.ellipse(p.x,p.y+1,(2+3*beat)*p.scale,1.8*p.scale);cv.fill();
    if(surge>0){const double age=c.t-c.schedule->print.surgeStart;
        for(int i=0;i<18;++i){double delay=hash2(i,p.seed)*2,life=(age-delay)/6;
            if(life<=0 || life>=1)continue;double fade=sstep(0,.12,life)*(1-sstep(.6,1,life));
            double x=p.x+(hash2(i,p.seed+1)-.3)*85*life,y=p.y-4-(70+80*hash2(i,p.seed+2))*std::sin(Pi*life);
            cv.color(i%3?p.accent:p.ink,.72*fade);cv.ellipse(x,y,1.2*p.scale,2.5*p.scale);cv.fill();}}
}
}
namespace Journey::Kit {
namespace {
void printBird(Canvas&cv,const PrintBirdPoseV1&b,const PrintLifeParametersV1&p,bool crane=false){
    cv.save();cv.translate(b.x,b.y);cv.scale(b.direction*b.size,b.size);
    cv.color(p.color,b.alpha);cv.moveTo(-1,0);cv.curveTo(-4,-3,-10,-b.flap*8,-15,-b.flap*7-1);cv.curveTo(-11,-b.flap*3,-5,2,0,2);
    cv.curveTo(4,1,9,-b.flap*3,14,-b.flap*8);cv.curveTo(8,-b.flap*9,4,-2,1,-1);cv.closePath();cv.fill();
    cv.color(p.accent,b.alpha*.7);cv.ellipse(0,0,4,1.2);cv.fill();
    if(crane){cv.color(p.accent,b.alpha);cv.moveTo(2,0);cv.curveTo(8,-1,3,-7,10,-6);cv.stroke(1.4);cv.line(-3,1,-13,3,.8,p.ink,b.alpha);cv.line(-3,1,-13,5,.8,p.ink,b.alpha);}
    else{cv.line(2,0,6,-1,1.6,p.color,b.alpha);}
    cv.restore();
}
}
std::vector<PrintBirdPoseV1> BirdFlockV1::poses(const Ctx&c,const PrintLifeParametersV1&p){
    std::vector<PrintBirdPoseV1> out;if(!c.schedule)return out;const auto&clock=c.schedule->print;
    const double sinceSurge=c.t-clock.surgeStart,scatter=p.surgeEnabled?clock.surge(c.t):0;
    const bool scattering=p.surgeEnabled && sinceSurge>=0 && sinceSurge<10;
    auto event=clock.flock;if(scattering){event.start=clock.surgeStart;event.duration=10;event.count=p.count;event.height=.55;}
    const double age=c.t-event.start,u=age/event.duration;if(u<0 || u>1)return out;
    const double alpha=sstep(0,.07,u)*(1-sstep(.86,1,u));
    for(int i=0;i<std::min(p.count,event.count);++i){const double id=hash2(i,p.seed+event.cycle),direction=scattering?(i%2?1:-1):event.direction;
        const double x=scattering?p.x+p.width*.36+direction*age*(90+35*id):p.x+(direction>0?u:1-u)*(p.width+180)-90-i*28*direction;
        const double y=p.y+p.height*event.height+(p.nearEvents?45:18)*std::sin(c.t*.6+i*.8)-i*7-105*scatter*(.5+id);
        const double phase=c.t*(4+id)+(c.score?c.score->bandIntegrals[p.band]*3:0)+i*1.4;
        const double flap=std::sin(phase)*(1+.65*c.hit(7)+.7*scatter);
        // The horizon flock is distant. Close passes stay high in the sky,
        // with a smooth size falloff toward the sea rather than equal sprites.
        const double perspective=p.nearEvents?lerp(1.,.28,sstep(280,612,y)):1.;
        out.push_back({x,y,p.scale*(.65+.35*id)*perspective,flap,direction,alpha});}
    return out;
}
void BirdFlockV1::paint(Canvas&cv,const Ctx&c,const PrintLifeParametersV1&p){for(const auto&b:poses(c,p))printBird(cv,b,p);}
}
namespace Journey::Kit {
void SeaCreatureV1::paint(Canvas&cv,const Ctx&c,const PrintLifeParametersV1&p){
    if(!c.schedule)return;const auto&v=c.schedule->print.dragon;
    const double age=c.t-v.start,u=age/v.duration;if(u<=0 || u>=1)return;
    const double emerge=sstep(0,.25,u)*(1-sstep(.65,1,u));
    const double bass=p.gain*(c.score?c.score->bandBody[1][p.band]:c.band(p.band));
    const double travel=(u-.5)*p.width*.25*v.direction;
    auto water=[&](double x){return printWater(c,p,x);};
    std::vector<V2> coil;
    for(int i=0;i<=96;++i){const double f=i/96.,x=p.x+(f-.5)*p.width+travel;
        const double y=water(x)+18*p.scale-(p.height*(.75+.35*bass)*emerge)*std::pow(.5+.5*std::sin(f*Tau*3-u*3),1.25);
        if(y>water(x)-1){if(coil.size()>1){cv.polyline(coil,18*p.scale,p.ink);cv.polyline(coil,13*p.scale,p.color);cv.polyline(coil,3*p.scale,p.accent,.6);}coil.clear();}else coil.push_back({x,y});
        if(i%3==0 && y<water(x)-5){cv.line(x-3,y-4,x+3,y+3,1.3*p.scale,p.accent,.65);}}
    if(coil.size()>1){cv.polyline(coil,18*p.scale,p.ink);cv.polyline(coil,13*p.scale,p.color);cv.polyline(coil,3*p.scale,p.accent,.6);}
    const double x=p.x+p.width*.42+travel,y=water(x)+15*p.scale-p.height*emerge;
    if(y<water(x)-5){const double alpha=sstep(5,20,water(x)-y);
        for(int pass=0;pass<2;++pass){cv.color(pass?p.color:p.ink,alpha);
            cv.moveTo(x-35*p.scale,water(x-35*p.scale)-1);cv.curveTo(x-45*p.scale,y+50*p.scale,x-30*p.scale,y+20*p.scale,x-12*p.scale,y+5*p.scale);cv.stroke((pass?13:18)*p.scale);}
        cv.save();cv.translate(x,y);cv.scale(p.scale,p.scale);
        cv.color(p.ink,alpha);cv.moveTo(-20,8);cv.curveTo(-22,-20,-2,-24,10,-10);cv.lineTo(26,-4);cv.curveTo(25,8,10,12,3,14);cv.lineTo(-20,8);cv.closePath();cv.fill();
        cv.color(p.color,alpha);cv.moveTo(-17,6);cv.curveTo(-18,-15,-1,-19,8,-8);cv.lineTo(23,-2);cv.lineTo(6,10);cv.closePath();cv.fill();
        cv.disc(8,-7,2.1,p.accent,alpha);cv.disc(8.5,-7,1,p.ink,alpha);
        for(int i=0;i<2;++i){double x=-12+i*12;cv.poly({{x,-11},{x-5,-29},{x+2,-22},{x+5,-32}},2,p.accent,alpha);}
        cv.color(p.accent,alpha);cv.moveTo(19,4);cv.curveTo(44,1,35,-18,51,-13);cv.stroke(1.4);
        cv.moveTo(17,8);cv.curveTo(37,22,46,11,49,16);cv.stroke(1.1);cv.restore();}
    for(int i=0;i<8;++i){double x=p.x+(i/7.-.5)*p.width+travel;cv.color(p.accent,.36*emerge);cv.ellipse(x,water(x)+3,(12+8*emerge)*p.scale,2*p.scale);cv.stroke(p.scale);}
}
}
namespace Journey::Kit {
void LeapingFishV1::paint(Canvas&cv,const Ctx&c,const PrintLifeParametersV1&p){
    if(!c.schedule)return;const auto&v=c.schedule->print.events[int(PrintMoment::Fish)];
    const double age=(c.t-v.start)*v.speed*p.speed;
    for(int i=0;i<std::min(p.count,5);++i){const double u=(age-i*.85)/2.2;if(u<=0 || u>=1)continue;
        const double origin=p.followsBoat?BoatOnWaterV1::pose(c,p.boat).at.x+p.boat.length*p.boat.scale*.8:p.x;
        const double x=origin+p.width*hash2(i,p.seed+v.cycle)+(u-.5)*110*v.direction;
        const double water=printWater(c,p,x);
        const double y=water-p.height*std::sin(Pi*u)*(1+.5*c.band(p.band));
        const double alpha=sstep(0,.05,u)*(1-sstep(.94,1,u));
        cv.save();cv.translate(x,y);cv.rotate(v.direction*(u-.5)*1.9);cv.scale(p.scale,p.scale);
        // Silver print fish with a key outline, readable against the swells.
        cv.color(p.ink,alpha);cv.ellipse(0,0,14.5,5.2);cv.fill();cv.tri({-11,0},{-22,-8.5},{-22,8.5},p.ink,alpha);
        cv.color(mix(p.accent,hex(0x7397a4),.35),alpha);cv.ellipse(0,0,13,4);cv.fill();cv.tri({-11,0},{-20,-7},{-20,7},p.accent,alpha);
        cv.line(-8,-1,9,-1,1,p.color,alpha);cv.disc(8,0,1.2,p.ink,alpha);cv.restore();
        cv.color(p.accent,.6*alpha);cv.moveTo(x-10-14*u,water+2);cv.curveTo(x-4,water-3-3*u,x+4,water-3-3*u,x+10+14*u,water+2);cv.stroke(1.6);
        for(int j=0;j<3;++j)cv.disc(x-15+j*9,water-12*std::sin(Pi*u)*(j+1),1,p.accent,.5*alpha);
    }
}
void PrintMomentsV1::paint(Canvas&cv,const Ctx&c,const PrintLifeParametersV1&p){
    if(!c.schedule)return;const auto&clock=c.schedule->print;
    const double surge=p.surgeEnabled?clock.surge(c.t):0;
    auto active=[&](PrintMoment m){const auto&v=clock.events[int(m)];double u=(c.t-v.start)/v.duration;return std::pair<double,double>{u,sstep(0,.10,u)*(1-sstep(.80,1,u))};};
    if(p.domain==0){
        auto [u,alpha]=active(PrintMoment::Cranes);const auto&v=clock.events[int(PrintMoment::Cranes)];
        if(alpha>0 && (!p.nearEvents || v.cycle%2==0))for(int i=0;i<(p.nearEvents?1:2);++i){double x=p.x+(v.direction>0?u:1-u)*(p.width+160)-80-i*65*v.direction,y=p.y+60+p.height*v.height*.45+i*23;
            printBird(cv,{x,y,(p.nearEvents?.65:1.35)*p.scale,std::sin(c.t*3.4+i*.5+(c.score?c.score->bandIntegrals[p.band]:0))*(1+.5*c.hit(6)),v.direction,alpha},p,true);}
        auto [su,sa]=active(PrintMoment::Star);if(sa>0){const auto&v=clock.events[int(PrintMoment::Star)];double x=p.x+p.width*(.30+.35*v.height)+su*330,y=p.y-130+su*115;
            cv.line(x-90,y-32,x,y,1.8*p.scale,p.accent,.6*sa);cv.disc(x,y,2.4*p.scale,p.accent,sa);}
        auto [qu,qa]=active(PrintMoment::Squall);if(qa>0){const auto&v=clock.events[int(PrintMoment::Squall)];
            const double close=p.nearEvents && v.cycle%2?1.65:1.;
            double x=p.x+(p.width+400)*(v.direction>0?qu:1-qu)-200,y=p.y+p.height-110-65*(close-1);
            const double music=clamp01(3*(c.score?c.score->bandBody[1][p.band]:c.band(p.band)));
            // One lobed printed rain cloud in the theme's muted ink, a single
            // outline so its tiers never darken where they overlap.
            {const double w=430*close,h=20*close;cv.color(mix(hex(0x54546d),hex(0xc9b2a0),.42),.55*qa);
             cv.moveTo(x-w/2,y+h/2);
             for(int k=0;k<=40;++k){const double f=k/40.,xx=x-w/2+w*f;
                const double tier=.45*h*sstep(.18,.30,f)*(1-sstep(.62,.74,f));
                const double bump=.24*h*std::pow(std::abs(std::sin(Pi*f*7+v.cycle)),1.4)*std::pow(std::sin(Pi*f),.5);
                cv.lineTo(xx,y-h/2-tier-bump);}
             cv.curveTo(x+w/2+h*.6,y-h/2,x+w/2+h*.6,y+h/2,x+w/2,y+h/2);
             cv.curveTo(x-w/2-h*.6,y+h/2,x-w/2-h*.6,y+h/2,x-w/2,y+h/2);cv.closePath();cv.fill();}
            for(int i=0;i<(p.nearEvents?38:18);++i){
                const double phase=c.t*(.38+.12*hash2(i,p.seed+4))+hash2(i,p.seed+5),f=phase-std::floor(phase);
                double xx=x+(hash2(i,p.seed)-.5)*320*close-f*24,yy=y+22+f*120*close;
                const double fade=sstep(0,.08,f)*(1-sstep(.8,1,f));
                cv.line(xx,yy,xx-12*close,yy+(18+30*hash2(i,p.seed+1))*close,
                    .7+music*.8,hex(0x54546d),(.32+.2*music)*qa*fade);}}
        for(int i=0;i<8;++i){double y=p.y+30+i*17,x=p.x+p.width*(.5+.5*std::sin(c.t*.23+i*.53));
            cv.color(p.accent,.20*surge);cv.moveTo(x-170,y);cv.curveTo(x-70,y-12,x+70,y+9,x+180,y-4);cv.stroke(1.2*p.scale);}
    }else if(p.domain==1){
        for(auto type:{PrintMoment::FishingBoat,PrintMoment::LanternBoat}){auto [u,alpha]=active(type);if(alpha<=0)continue;const auto&v=clock.events[int(type)];
            const double x=p.x+(v.direction>0?u:1-u)*(p.width+160)-80;
            const double y=printWater(c,p,x);
            // The occasional fishing and lantern hulls get the same crisp
            // contact language as the rowboats, scaled by their crossing speed.
            const double pace=std::min(1.,(p.width+160)/v.duration/240.);
            const double tail=x-v.direction*60*p.scale,wake=(16+22*pace)*p.scale;
            cv.color(p.accent,.5*alpha);cv.moveTo(tail,printWater(c,p,tail)+9*p.scale);
            cv.curveTo(tail-v.direction*wake*.35,printWater(c,p,tail)+10*p.scale,
                tail-v.direction*wake*.75,printWater(c,p,tail-v.direction*wake)+11*p.scale,
                tail-v.direction*wake,printWater(c,p,tail-v.direction*wake)+9*p.scale);
            cv.curveTo(tail-v.direction*wake*.65,printWater(c,p,tail-v.direction*wake)+13*p.scale,
                tail-v.direction*wake*.2,printWater(c,p,tail)+13*p.scale,tail,printWater(c,p,tail)+9*p.scale);
            cv.closePath();cv.fill();
            const double bow=x+v.direction*60*p.scale;
            cv.color(p.accent,.7*alpha);cv.moveTo(bow-3*v.direction*p.scale,printWater(c,p,bow)+4*p.scale);
            cv.curveTo(bow+6*v.direction*p.scale,printWater(c,p,bow)-2*p.scale,
                bow+12*v.direction*p.scale,printWater(c,p,bow)+p.scale,
                bow+14*v.direction*p.scale,printWater(c,p,bow)+7*p.scale);
            cv.lineTo(bow+5*v.direction*p.scale,printWater(c,p,bow)+6*p.scale);cv.closePath();cv.fill();
            cv.save();cv.translate(x,y);cv.scale(v.direction*p.scale,p.scale);
            cv.color(p.ink,alpha);cv.moveTo(-62,-6);cv.curveTo(-25,7,25,7,65,-8);cv.lineTo(48,13);cv.curveTo(10,19,-40,15,-62,-6);cv.closePath();cv.fill();
            cv.color(p.accent,.65*alpha);cv.moveTo(-58,-4);cv.curveTo(-20,9,20,9,60,-6);cv.stroke(1.6);
            cv.disc(-15,-19,4,p.ink,alpha);cv.line(-15,-15,-14,-1,5,p.ink,alpha);cv.line(-14,-5,20,-20,2,p.accent,alpha);
            if(type==PrintMoment::FishingBoat){cv.poly({{12,-1},{12,-48},{42,-15}},1.5,p.accent,alpha);cv.line(18,-20,45,10,.9,p.accent,alpha);}
            else{cv.line(22,1,22,-40,1.6,p.accent,alpha);cv.color(p.color,.85*alpha);cv.ellipse(22,-29,8,11);cv.fill();for(int j=0;j<3;++j)cv.line(16,-35+j*6,28,-35+j*6,.9,p.ink,.6*alpha);}
            cv.restore();cv.color(p.accent,.25*alpha);cv.ellipse(x-55,y+5,60,2);cv.stroke(1);
            if(type==PrintMoment::LanternBoat)for(int i=0;i<6;++i)cv.line(x+12-i*2,y+12+i*5,x+29+i*2,y+12+i*5,1.5,p.color,alpha*.15*(1-i/6.));}
        auto [u,alpha]=active(PrintMoment::Gust);if(alpha>0)for(int i=0;i<16;++i){double x=p.x+p.width*u+(hash2(i,p.seed)-.5)*430;
            const auto sea=SwellLinesV1::field(c,5+i%5,p.swell);double y=sea.y(x)+i*2;
            cv.color(p.accent,.38*alpha);cv.moveTo(x-70,y);cv.curveTo(x-10,y-7,x+55,y+2,x+130,y-4);cv.stroke(1.4);}
    }else if(p.domain==3){
        const auto& v=clock.foregroundSwell;const double u=(c.t-v.start)/v.duration;
        if(u<=0 || u>=1)return;
        const double envelope=sstep(0,.24,u)*(1-sstep(.70,1,u));
        const double bass=clamp01(3*(c.score?c.score->bandBody[1][0]:c.band(0)));
        auto crest=[&](double x){return 1122-envelope*(82+13*bass)+30*std::sin(x*.0036-c.t*.31*v.direction)
            +12*std::sin(x*.0081+1.3-c.t*.19*v.direction);};
        // A printed foreground wave: bokashi body, key line, broken cream crest
        // and a few irregular claws, matching the hero's language.
        std::vector<V2> edge;double hi=1e9;
        for(int i=0;i<=80;++i){const double x=-40+i*25;edge.push_back({x,crest(x)});hi=std::min(hi,crest(x));}
        cv.linear(0,hi,0,hi+120,{{0,mix(p.color,p.ink,.35),1},{.5f,p.ink,1},{1,p.ink,1}});
        cv.moveTo(-40,1140);for(V2 q:edge)cv.lineTo(q.x,q.y);cv.lineTo(1960,1140);cv.closePath();cv.fill();
        cv.polyline(edge,2.4,hex(0x0e2140),.85*envelope);
        {std::vector<V2> l,r;for(std::size_t i=0;i<edge.size();++i){const double w=(2+6*std::pow(.5+.5*noise1(i*.35+c.t*.1,61),2))*envelope;
            l.push_back(edge[i]+V2(0,-1));r.push_back(edge[i]+V2(0,w));}
         for(std::size_t j=0;j+1<l.size();++j){cv.tri(l[j],l[j+1],r[j+1],p.accent,.92*envelope);cv.tri(l[j],r[j+1],r[j],p.accent,.92*envelope);}}
        // Claws gather only at the crest's peaks, leaning the way it travels.
        for(std::size_t i=2;i+2<edge.size();++i){
            if(!(edge[i].y<edge[i-1].y && edge[i].y<=edge[i+1].y))continue;
            for(int k=0;k<4;++k){const double size=(1.05-.18*k)*(.7+.3*hash2(k,i+p.seed));
                const double xx=edge[i].x+v.direction*(k*15-8),y=crest(xx)+1,d=v.direction;
                cv.color(hex(0x0e2140),.8*envelope);cv.moveTo(xx-2*d,y);cv.curveTo(xx+8*size*d,y-10*size,xx+22*size*d,y-5*size,xx+23*size*d,y+10*size);
                cv.curveTo(xx+15*size*d,y+2*size,xx+6*size*d,y+1,xx-2*d,y);cv.closePath();cv.fill();
                cv.color(p.accent,envelope);cv.moveTo(xx,y);cv.curveTo(xx+8*size*d,y-8*size,xx+20*size*d,y-4*size,xx+21*size*d,y+8*size);
                cv.curveTo(xx+14*size*d,y+1.5*size,xx+6*size*d,y+1,xx,y);cv.closePath();cv.fill();}}
        const double hit=c.kick(5)+c.hit(5);
        for(int i=0;i<18;++i){const double phase=c.t*.36+hash2(i,p.seed),f=phase-std::floor(phase),x=90+i*102+f*14;
            const double alpha=envelope*sstep(0,.10,f)*(1-sstep(.5,1,f))*(.20+.45*hit);
            cv.disc(x,crest(x)-25*std::sin(Pi*f)*(1+.25*bass),1.2,p.accent,alpha);}
    }else{
        auto [u,alpha]=active(PrintMoment::SnowGlint);if(alpha>0){const double x=p.x-14+u*26,y=p.y+9+7*std::sin(u*Pi);
            alpha*=.1+.9*clamp01(2*c.band(p.band)*p.gain+.7*c.hit(5)+c.kick(5));
            cv.line(x-8,y,x+8,y,2.2,p.accent,.85*alpha);cv.line(x,y-5,x,y+5,1.8,p.accent,.85*alpha);cv.disc(x,y,3.6,p.accent,alpha);}
    }
}
}

namespace Journey::Kit {
QPainterPath printResponsePath(const Canvas&cv) {
    QPainterPath out;out.setFillRule(Qt::WindingFill);
    // Strokes and ink fills expose their submitted triangle support. Orient
    // triangles alike so overlaps do not cancel in the response mask.
    const auto&v=cv.vertices();
    for(const auto&cmd:cv.commands())for(int i=cmd.first;i+2<cmd.first+cmd.count;i+=3){
        if(i+2>=int(v.size()))break;const auto&a=v[i];const auto&b=v[i+1];const auto&c=v[i+2];
        if(std::max({a.a,b.a,c.a})<=.01)continue;
        QPolygonF tri;tri<<QPointF(a.x,a.y);
        const double cross=(b.x-a.x)*(c.y-a.y)-(b.y-a.y)*(c.x-a.x);
        if(cross>=0)tri<<QPointF(b.x,b.y)<<QPointF(c.x,c.y);else tri<<QPointF(c.x,c.y)<<QPointF(b.x,b.y);
        out.addPolygon(tri);out.closeSubpath();
    }
    return out;
}
}
