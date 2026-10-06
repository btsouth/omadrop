#include "water-surface.h"
#include <cmath>
namespace Journey::Kit {
namespace {
// The prototype's interpolated band integral preserves continuous water flow.
// Double precision keeps the continuous clock stable over long sessions.
double flow(const Ctx& c, double z) {
    if (!c.score) return 0;
    const double role=(1-z)*5;
    const int lo=std::min(4,int(role));
    return .95*lerp(c.score->bandIntegrals[lo],c.score->bandIntegrals[lo+1],role-lo);
}
double energy(const Ctx& c, int band) {
    if (!c.score) return c.band(band);
    // The live Score uses causal exponential envelopes at the prototype's
    // .4/.9/1.6 second scales rather than querying an offline timeline.
    return .20*c.score->bandBody[0][band]+.50*c.score->bandBody[1][band]+.30*c.score->bandBody[2][band];
}
}
int WaterSurfaceV1::band(int row,int rows) { return std::clamp(int(std::round((1-row/double(rows-1))*5)),0,5); }
double WaterSurfaceV1::rowY(int row,const WaterSurfaceParametersV1& p) {
    return p.horizon+2+(p.nearY-p.horizon-2)*std::pow(row/double(p.rows-1),1.45);
}
QRectF WaterSurfaceV1::responseArea(int row,const WaterSurfaceParametersV1& p) {
    const double z=row/double(p.rows-1),y=rowY(row,p);
    // The moving crest, caps and adjacent inner ripples, like a label light area.
    const double travel=(7+83*std::pow(z,1.15))*p.amplitude*1.23;
    return QRectF(p.x0,std::max(p.horizon,y-travel-4),p.x1-p.x0,2*travel+30+42*z);
}
void WaterSurfaceV1::draw(Ctx& c,const WaterSurfaceParametersV1& p) {
    GpuProfile::Group group(c.gpu.profile,name);
    Canvas& cv=c.canvas();
    cv.linear(0,p.horizon,0,1080,{{0,p.top,1},{1,p.bottom,1}});
    cv.rect(p.x0,p.horizon,p.x1-p.x0,1080-p.horizon);cv.fill();
    const double kick=c.kick(5);
    // Resolve measured controls once per depth, not per sampled vertex.
    for(int row=0;row<p.rows;++row) {
        const double z=row/double(p.rows-1),y=rowY(row,p);
        const int b=band(row,p.rows);
        const double e=energy(c,b),lift=c.lift(b),travel=flow(c,z);
        const double clock=c.t*p.drift+travel-p.phase;
        const double waveScale=(55+115*z)*p.wavelength;
        auto top=[&](double x) {
            const double a=x/waveScale-clock+z*6;
            return y+(7+83*std::pow(z,1.15))*p.amplitude*(1+.35*e)
                *(std::sin(a)+.23*std::sin(a*2.1+.6));
        };
        std::vector<V2> crest;
        for(double x=p.x0;x<p.x1;x+=p.sampleStep)crest.push_back({x,top(x)});
        crest.push_back({p.x1,top(p.x1)});
        cv.color(mix(p.top,p.bottom,z*.9));
        cv.moveTo(p.x0,1140);for(V2 q:crest)cv.lineTo(q.x,q.y);cv.lineTo(p.x1,1140);cv.closePath();cv.fill();
        cv.polyline(crest,1.2+1.5*z,p.crest,.9);
        const double bright=std::min(1.0,.18+p.bandGain*e+p.liftGain*lift+p.kickGain*kick);
        for(int j=0;j<3;++j) {
            std::vector<V2> water;
            for(double x=p.x0;x<p.x1;x+=p.sampleStep)water.push_back({x,top(x)+10+j*(8+14*z)});
            water.push_back({p.x1,top(p.x1)+10+j*(8+14*z)});
            cv.polyline(water,1+j*.3,mix(p.texture,p.foam,.35),bright);
        }
        // The same authored broken crest silhouette, with fewer caps and no
        // impact, churn or spray. Bare crests keep it from becoming a grid.
        const int first=int(std::floor((p.x0/waveScale+Pi/2-clock+z*6)/(2*Pi)))-1;
        const int last=int(std::ceil((p.x1/waveScale+Pi/2-clock+z*6)/(2*Pi)))+1;
        for(int k=first;k<=last;++k) {
            const double middle=(-Pi/2+2*Pi*k+clock-z*6)*waveScale;
            const double id=hash2(k,row+71+p.seed);
            if(id<1-p.capDensity || middle<p.x0-200 || middle>p.x1+200)continue;
            const double breath=.5+.5*std::sin(c.t*(.45+.55*hash2(k,row+72+p.seed))+Tau*hash2(k,row+73+p.seed));
            const double size=p.capScale*(.45+.95*hash2(k,row+38+p.seed))*(.38+.62*breath)*(1+p.liftGain*lift);
            const double span=(40+170*z)*size;
            if(span<10)continue;
            const double x0=middle-span*(.30+.22*hash2(k,row+74+p.seed));
            const double thick=(4+30*z)*(.7+.5*size)*p.capScale;
            const int lobes=2+int(4*hash2(k,row+75+p.seed));
            auto under=[&](double u) {
                const double lobe=.62+.38*std::abs(std::sin(Pi*u*lobes+hash2(k,row+76+p.seed)*3));
                return thick*std::pow(std::sin(Pi*u),.65)*lobe*(.7+.6*u)*(1+.22*noise1(u*9,row+k));
            };
            for(int pass=0;pass<2;++pass) {
                const double dy=pass?0:2.5+2*z;
                cv.color(pass?p.foam:p.underprint,pass?std::min(1.0,.5+bright):.55);
                cv.moveTo(x0,top(x0)+dy);
                for(int j=1;j<=16;++j){double x=x0+j*span/16;cv.lineTo(x,top(x)-(pass?0:1)+dy);}
                for(int j=16;j>=0;--j){double u=j/16.0,x=x0+u*span;cv.lineTo(x,top(x)+under(u)+dy);}
                cv.closePath();cv.fill();
            }
        }
    }
    // Calm-sea texture rows reuse the prototype's perspective and noise, at a
    // lower count. Each depth's band brightens its own live ripples.
    for(int row=0;row<p.textureRows;++row) {
        const double z=(row+.5)/p.textureRows,y=p.horizon+4+(1080-p.horizon-8)*std::pow(z,1.72);
        const int b=std::clamp(int((1-z)*5),0,5);
        const double drift=flow(c,z),e=energy(c,b);
        std::vector<V2> points;
        for(double x=p.x0;x<=p.x1;x+=24) {
            const double swell=15*z*e*std::sin(x*.006-c.t*.7+row);
            points.push_back({x,y+p.amplitude*((2+22*z*z)*std::sin(x*.0055+row*.82-c.t*.55-drift)+swell+2*noise1(x*.012,row))});
        }
        cv.polyline(points,row%7==0?1.65:.65,p.texture,std::min(.7,.07+.10*z+p.bandGain*.35*e));
    }
    // Sunset reflections from sea(), with live twinkle, band level and a soft
    // kick shimmer. They stay localized rather than flashing the whole plane.
    Rng rng(p.seed);
    for(int i=0;i<p.glints;++i) {
        const double yy=p.horizon+4+std::pow(rng.uni(),1.8)*p.glintDepth;
        const double depth=yy-p.horizon;
        const double w=(10+rng.uni()*60)*(.4+depth/200);
        const double xx=p.glintX+rng.normal()*(26+depth*.42)+std::sin(c.t*2+i)*4;
        const bool hot=rng.uni()>=.65;
        const int b=std::clamp(int((1-depth/p.glintDepth)*5),0,5);
        const double tw=.6+.4*std::sin(c.t*(1.5+i%7*.4)+i);
        const double response=.16+p.bandGain*c.band(b)+p.liftGain*c.lift(b)+p.kickGain*kick;
        cv.line(xx-w/2,yy,xx+w/2,yy,1.6+depth/90,hot?p.hotGlint:p.glint,
                std::min(1.0,response)*(1-depth/(p.glintDepth+50))*tw);
    }
    c.gpu.over(cv,1,0,float(p.opacity));
}
}
