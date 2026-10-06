#include "foam-flecks.h"
#include <cmath>
namespace Journey::Kit {
namespace {
double rowPulse(const std::deque<Event>&events,double t){double sum=0;
    for(const auto&e:events){double age=t-e.t;if(age>=0 && age<.9)sum+=e.strength*sstep(0,.06,age)*std::exp(-age*5)*(1-sstep(.65,.9,age));}
    return 1-std::exp(-sum);
}
}

QPainterPath FoamFlecksV1::exclusionPath(const Ctx& c,const FoamFlecksParametersV1& p) {
    QPainterPath path;
    if(!p.masksWaveTrain)return path;
    const auto& field=WaveTrainV2::worldProfile(c,p.waveTrain);
    for(const auto& crest:field.crests) {
        // A live convex envelope seals the barrel as well as the body. Include
        // claw tips and their blue shadows, rather than a stale world rectangle.
        std::vector<V2> points=crest.boundary;
        auto add=[&](const auto& v){points.insert(points.end(),v.begin(),v.end());};
        add(crest.foamRim);add(crest.foamInside);
        for(const auto& f:crest.fingers){add(f.left);add(f.right);add(f.shadow.left);add(f.shadow.right);
            for(const auto& st:f.twigs){add(st.left);add(st.right);}}
        for(const auto& st:crest.tangle){add(st.left);add(st.right);}
        for(const auto& st:crest.falling){add(st.left);add(st.right);}
        std::sort(points.begin(),points.end(),[](V2 a,V2 b){return a.x<b.x || (a.x==b.x && a.y<b.y);});
        auto cross=[](V2 a,V2 b,V2 q){return (b.x-a.x)*(q.y-a.y)-(b.y-a.y)*(q.x-a.x);};
        std::vector<V2> hull;
        for(auto q:points){while(hull.size()>1 && cross(hull[hull.size()-2],hull.back(),q)<=0)hull.pop_back();hull.push_back(q);}
        const auto lower=hull.size();
        for(auto i=points.rbegin()+1;i!=points.rend();++i){while(hull.size()>lower && cross(hull[hull.size()-2],hull.back(),*i)<=0)hull.pop_back();hull.push_back(*i);}
        if(hull.empty())continue;
        QPainterPath envelope;envelope.moveTo(hull[0].x,hull[0].y);
        for(auto q:hull)envelope.lineTo(q.x,q.y);envelope.closeSubpath();
        path=path.united(envelope);
    }
    return path;
}

void FoamFlecksV1::paint(Canvas& cv,const Ctx& c,const FoamFlecksParametersV1& p) {
    const auto& s=p.swell;
    const auto waveExclusion=exclusionPath(c,p);
    std::vector<SwellRowV1> fields;for(int row=0;row<s.rows;++row)fields.push_back(SwellLinesV1::field(c,row,s));
    // Seeded patches choose real local minima of the shared swell, never
    // evenly spaced lanes. Reject duplicate crests instead of stacking stamps.
    std::vector<std::vector<double>> used(s.rows);
    for(int i=0;i<p.count;++i) {
        const int row=i%s.rows;const auto& f=fields[row];
        const double depth=std::pow(f.z,s.depthFalloff),id=hash2(i,s.seed+71);
        const int band=SwellLinesV1::band(row,s.rows);
        const double drive=clamp01(3.6*(c.score?c.score->bandBody[0][band]:c.band(band)));
        const double flare=c.score?(p.onsetGain*rowPulse(c.score->onsets,c.t-row*.012)
            +s.kickGain*rowPulse(c.score->bassHits,c.t-row*.012))*(.35+.65*drive):0;
        const double visibility=sstep(id-.18,id+.12,.52+.40*drive+.20*flare);
        if(visibility<.01)continue;
        const double margin=260,wavelength=Tau*f.scale;
        const int cycles=int(std::ceil((s.region.width()+2*margin)/wavelength))+1;
        const double period=cycles*wavelength;
        const double seedPhase=Tau*hash2(row,s.seed+19)-depth*6;
        const double target=s.region.left()-margin+period*hash2(i,s.seed+74);
        const double k=std::round((target/f.scale-seedPhase+Pi/2)/Tau);
        const double origin=(f.phase-Pi/2+Tau*k)*f.scale;
        // Wrap by complete wavelengths only outside the viewport. Each
        // seeded patch rides one crest continuously rather than changing
        // crests when a drifting candidate crosses a nearest-point boundary.
        const double middle=s.region.left()-margin+
            std::fmod(std::fmod(origin-s.region.left()+margin,period)+period,period);
        double a=middle-.65*f.scale,b=middle+.65*f.scale;
        for(int j=0;j<18;++j){const double l=(2*a+b)/3,r=(a+2*b)/3;
            if(f.y(l)<f.y(r))b=r;else a=l;}
        const double crest=(a+b)*.5+(hash2(i,s.seed+75)-.5)*f.scale*1.3;
        bool duplicate=false;for(double x:used[row])if(std::abs(x-crest)<f.scale*.10)duplicate=true;
        if(duplicate)continue;
        used[row].push_back(crest);
        const double breath=.9+.1*std::sin(c.t*(.45+.55*hash2(i,s.seed+72))+Tau*hash2(i,s.seed+73));
        const double size=lerp(p.sizeMin,p.sizeMax,id)*breath
            *(1+p.responseGain*(c.score?c.score->bandBody[1][0]:c.band(0)))
            *(1+.15*s.liftGain*c.lift(band))*(1+.65*flare);
        const int shape=int(hash2(i,s.seed+76)*3);
        const double span=(12+82*depth)*size,thick=(1+6*depth)*(.7+.35*size);
        const double x0=crest-span*(.35+.3*hash2(i,s.seed+77));
        const double finger=(2+8*depth)*(.6+.4*size);
        double lo=f.y(x0),hi=lo;for(int j=1;j<=16;++j){double y=f.y(x0+j*span/16);lo=std::min(lo,y);hi=std::max(hi,y);}
        const QRectF bounds(x0-2,lo-2,span+finger+4,hi-lo+thick+finger+8);
        if(!SwellLinesV1::allowed(bounds,s) || waveExclusion.intersects(bounds))continue;
        const double alpha=visibility*(.72+.20*f.brightness+.18*flare)*(.65+.35*depth);
        const int pieces=shape==2?2:1;
        for(int part=0;part<pieces;++part){
            const double begin=part? .61:0,end=pieces==2 && !part?.43:1;
            for(int pass=0;pass<2;++pass){
                const double dy=pass?0:1.5+depth;
                cv.color(pass?p.color:p.underprint,pass?alpha:alpha*.34);
                cv.moveTo(x0+span*begin,f.y(x0+span*begin)+dy);
                for(int j=1;j<=16;++j){const double u=lerp(begin,end,j/16.),x=x0+span*u;cv.lineTo(x,f.y(x)+dy);}
                for(int j=16;j>=0;--j){const double u=lerp(begin,end,j/16.),x=x0+span*u;
                    const double taper=std::sin(Pi*(u-begin)/(end-begin));
                    cv.lineTo(x,f.y(x)+dy+thick*taper*(.8+.2*std::sin(u*11+i)));}
                cv.closePath();cv.fill();
            }
        }
        // Only one third of patches curl; slivers and broken pairs leave ink
        // water between the two or three small, pointed claws.
        if(shape==1 && span>18)for(int j=0;j<2+int(id>.65);++j){
            const double x=x0+span*(.52+.16*j),y=f.y(x),ln=finger*(1-.18*j);
            cv.color(p.color,alpha);cv.moveTo(x-2,y);
            cv.curveTo(x+ln*.5,y-1,x+ln*1.1,y+ln*.25,x+ln*.85,y+ln);
            cv.curveTo(x+ln*.6,y+ln*.35,x+ln*.15,y+thick,x-2,y+thick*.65);
            cv.closePath();cv.fill();
        }
    }
}
void FoamFlecksV1::draw(Ctx& c,const FoamFlecksParametersV1& p) {
    GpuProfile::Group group(c.gpu.profile,name);Canvas& cv=c.canvas();paint(cv,c,p);c.gpu.over(cv);
}
}
