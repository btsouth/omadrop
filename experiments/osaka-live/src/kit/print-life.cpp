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
