#include "foam-flecks.h"
#include <cmath>
namespace Journey::Kit {
namespace {
double rowPulse(const std::deque<Event>&events,double t){double sum=0;
    for(const auto&e:events){double age=t-e.t;if(age>=0 && age<.9)sum+=e.strength*sstep(0,.06,age)*std::exp(-age*5)*(1-sstep(.65,.9,age));}
    return 1-std::exp(-sum);
}
}

void FoamFlecksV1::paint(Canvas& cv,const Ctx& c,const FoamFlecksParametersV1& p) {
    const auto& s=p.swell;
    std::vector<SwellRowV1> fields;for(int row=0;row<s.rows;++row)fields.push_back(SwellLinesV1::field(c,row,s));
    const double kick=c.kick(5);

    for(int i=0;i<p.count;++i) {
        const int row=i%s.rows;const auto& f=fields[row];const double depth=std::pow(f.z,s.depthFalloff);
        const int band=SwellLinesV1::band(row,s.rows);
        const double bandDrive=clamp01(3.6*(c.score?c.score->bandBody[0][band]:c.band(band)));
        const double flare=c.score?(p.onsetGain*rowPulse(c.score->onsets,c.t-row*.012)
            +s.kickGain*rowPulse(c.score->bassHits,c.t-row*.012))*(.35+.65*bandDrive):0;
        const double id=hash2(i,s.seed+71),spanScale=lerp(p.sizeMin,p.sizeMax,id*id);
        const double breath=.84+.16*std::sin(c.t*(.45+.55*hash2(i,s.seed+72))+Tau*hash2(i,s.seed+73));
        const double size=spanScale*breath*(1+p.responseGain*(c.score?c.score->bandBody[1][0]:c.band(0)))*(1+.20*s.liftGain*c.lift(band))*(1+1.5*flare);
        const double span=(18+130*depth)*size,thick=(2+19*depth)*(.7+.5*size);
        const double margin=260,period=s.region.width()+2*margin;
        const double travel=c.t*s.driftSpeed*(7+12*hash2(i,s.seed+32));
        const int lane=i/s.rows,lanes=(p.count+s.rows-row-1)/s.rows;
        const double start=(lane+.12+.76*hash2(i,s.seed+74))/lanes;
        const double x0=s.region.left()-margin+std::fmod(period*start+travel,period);
        const double offset=(hash2(i,s.seed+75)-.45)*(8+20*depth)-s.kickGain*kick*(2+8*depth);
        auto top=[&](double x){return f.y(x)+offset;};
        const int lobes=2+int(4*hash2(i,s.seed+76));
        auto under=[&](double u){
            const double lobe=.62+.38*std::abs(std::sin(Pi*u*lobes+hash2(i,s.seed+77)*3));
            return thick*std::pow(std::max(0.0,std::sin(Pi*u)),.65)*lobe*(.7+.6*u)*(1+.22*noise1(u*9,i));
        };
        double lo=top(x0),hi=lo;
        for(int j=1;j<=12;++j){const double y=top(x0+j*span/12);lo=std::min(lo,y);hi=std::max(hi,y);}
        const double finger=(5+19*depth)*(.7+.6*hash2(i,s.seed+78))*(.6+.5*size);
        // Cull the complete authored silhouette, including its fingers and
        // underprint, so a wave/hull reserve never receives a stray fragment.
        const int fingers=span>35 ? std::min(3,int(1+3*size*hash2(i,s.seed+79))) : 0;
        const double side=fingers?5:1,above=fingers?8:1;
        const double below=fingers?finger+20:4+2*depth;
        if(!SwellLinesV1::allowed(QRectF(x0-side,lo-above,span+2*side+(fingers?finger*1.2:0),hi-lo+thick*1.9+below+above),s))continue;
        // Depth rows flare with their frequency band and a small beat travel delay.
        const double flick=flare;
        const double alpha=std::min(1.0,.61+.28*f.brightness+flick);
        for(int pass=0;pass<2;++pass) {
            const double dy=pass?0:2.5+2*depth;
            cv.color(pass?p.color:p.underprint,pass?alpha:.42);cv.moveTo(x0,top(x0)+dy);
            for(int j=1;j<=12;++j){const double x=x0+j*span/12;cv.lineTo(x,top(x)-(pass?0:1)+dy);}
            for(int j=12;j>=0;--j){const double u=j/12.0,x=x0+u*span;cv.lineTo(x,top(x)+under(u)+dy);}
            cv.closePath();cv.fill();
        }
        // Prototype printSea's curled leading-edge fingers, kept only on the
        // larger fragments; tiny lobed chips supply most of the texture.
        for(int j=0;j<fingers;++j) {
            const double u=.62+.34*(j+.5)/fingers,x=x0+span*u,yy=top(x)+under(u)*.55;
            const double ln=finger*(.8+.2*hash2(j,i));
            cv.color(p.color,alpha);cv.moveTo(x-3,yy-2);
            cv.curveTo(x+ln*.45,yy-6,x+ln*1.15,yy,x+ln*.95,yy+ln*.7);
            cv.curveTo(x+ln*.70,yy+ln*.25,x+ln*.3,yy+6,x-3,yy+3);cv.closePath();cv.fill();
        }
    }
}
void FoamFlecksV1::draw(Ctx& c,const FoamFlecksParametersV1& p) {
    GpuProfile::Group group(c.gpu.profile,name);Canvas& cv=c.canvas();paint(cv,c,p);c.gpu.over(cv);
}
}
