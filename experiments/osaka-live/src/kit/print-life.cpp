#include "print-life.h"
namespace Journey::Kit {
void PrintMomentsV1::paint(Canvas&cv,const Ctx&c,const PrintLifeParametersV1&p){
    const double surge=p.surgeEnabled && c.schedule?c.schedule->print.surge(c.t):0;
    for(int i=0;i<8;++i){double y=p.y+30+i*17,x=p.x+p.width*(.5+.5*std::sin(c.t*.23+i*.53));
        cv.color(p.accent,.20*surge);cv.moveTo(x-170,y);cv.curveTo(x-70,y-12,x+70,y+9,x+180,y-4);cv.stroke(1.2*p.scale);}
}
}
namespace Journey::Kit {
void SmokePlumeV1::paint(Canvas&cv,const Ctx&c,const PrintLifeParametersV1&p){
    const double bass=c.score?c.score->bandBody[1][p.band]:c.band(p.band);
    const double surge=p.surgeEnabled && c.schedule?c.schedule->print.surge(c.t):0;
    const double drift=c.t*.25+(c.score?c.score->bandIntegrals[p.band]*.35:0);
    // Long tapered ribbons begin at the vent. Their drift is continuous and
    // their far ends dissolve into the printed sky instead of puff cycling.
    for(int i=0;i<5;++i){std::vector<V2> points;
        for(int k=0;k<=24;++k){double u=k/24.,y=p.y-u*p.height*(.55+.09*i+.25*bass),x=p.x+u*p.width*(.35+.7*bass)+std::sin(u*7-drift+i*.3)*u*(8+18*bass);
            points.push_back({x,y});}
        for(int k=1;k<int(points.size());++k){double u=k/24.;auto a=points[k-1],b=points[k];cv.line(a.x,a.y,b.x,b.y,(2+5*u+10*bass)*p.scale,p.color,(.10+.16*bass)*(1-u));}}
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
        const double y=p.y+p.height*event.height+18*std::sin(c.t*.6+i*.8)-i*7-105*scatter*(.5+id);
        const double phase=c.t*(4+id)+(c.score?c.score->bandIntegrals[5]*3:0)+i*1.4;
        const double flap=std::sin(phase)*(1+.65*c.hit(7)+.7*scatter);
        out.push_back({x,y,p.scale*(.65+.35*id),flap,direction,alpha});}
    return out;
}
void BirdFlockV1::paint(Canvas&cv,const Ctx&c,const PrintLifeParametersV1&p){for(const auto&b:poses(c,p))printBird(cv,b,p);}
}
