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
        SwellLinesParametersV1 shared;
        shared.orderedRows=true; // caps, contours and hulls share the same noncrossing crests
        shared.region=QRectF(p.x0,p.horizon,p.x1-p.x0,p.nearY-p.horizon);
        shared.surgeEnabled=p.surgeEnabled;shared.amplitudeGain=p.amplitudeGain;shared.rows=p.rows;shared.seed=p.swellSeed;shared.amplitude=p.amplitude;shared.driftSpeed=p.drift;shared.flowGain=p.flowGain;shared.rowFreedom=p.rowFreedom;shared.rollGain=p.rollGain;shared.rollDelay=p.rollDelay;shared.beatGain=p.beatGain;shared.liftGain=p.liftGain;shared.bandGain=p.bandGain;shared.kickGain=p.kickGain;
        const auto field=p.swellSeed>=0 ? SwellLinesV1::field(c,row,shared) : SwellRowV1{};
        auto top=[&](double x) {
            if(p.swellSeed>=0)return field.y(x);
            const double a=x/waveScale-clock+z*6;
            return y+(7+83*std::pow(z,1.15))*p.amplitude*(1+.35*e)
                *(std::sin(a)+.23*std::sin(a*2.1+.6));
        };
        std::vector<V2> crest;
        for(double x=p.x0;x<p.x1;x+=p.sampleStep)crest.push_back({x,top(x)});
        crest.push_back({p.x1,top(p.x1)});
        const Col base=mix(p.top,p.bottom,z*.9);
        if(p.shade>0) {
            // Each swell is printed light at its crest and darkens into the
            // trough before the next, nearer crest covers it.
            double hi=1e9,lo=-1e9;for(V2 q:crest){hi=std::min(hi,q.y);lo=std::max(lo,q.y);}
            const double depth=std::max(6.,(row+1<p.rows?rowY(row+1,p):1080)-hi+(lo-hi)*.4);
            const Col light=mix(base,p.shadeLight,p.shade*(.62-.30*z)),dark=mix(base,p.bottom,.35+.25*z);
            cv.linear(0,hi,0,hi+depth,{{0,light,1},{.45f,mix(light,dark,.55),1},{1,dark,1}});
        } else cv.color(base);
        if(p.swellSeed>=0 && row+1<p.rows) {
            // Nearer planes will cover everything below their own edge. Fill
            // only the interval to that edge, avoiding twelve full-height
            // stencil/cover passes. Max handles crossing contours; later
            // planes still paint over the older one in the original order.
            const auto next=SwellLinesV1::field(c,row+1,shared);
            cv.moveTo(crest.front().x,crest.front().y);
            for(V2 q:crest)cv.lineTo(q.x,q.y);
            for(auto q=crest.rbegin();q!=crest.rend();++q)cv.lineTo(q->x,std::max(q->y,next.y(q->x)));
        } else {
            cv.moveTo(p.x0,1140);for(V2 q:crest)cv.lineTo(q.x,q.y);cv.lineTo(p.x1,1140);
        }
        cv.closePath();cv.fill();
        const Col lineColor=p.swellSeed>=0 ? mix(p.crest,p.texture,std::min(.9,p.bandGain*e+p.liftGain*.5*lift)) : p.crest;
        if(p.crestOpacity>0)cv.polyline(crest,1.2+1.5*z,lineColor,p.crestOpacity);
        if(p.crestFoam>0 && p.swellSeed>=0) {
            // Foam belongs to the water: a cream rim over its underprint rides
            // each crest only where the row stands above its rest line. The
            // rim is fuller on the leading side of the travelling crest.
            const double reach=std::max(1.,field.displacementLimit>0?field.displacementLimit:field.amplitude);
            // Quiet water keeps only the tallest rims; a passing set and its
            // crash foam light the rows they reach.
            const double music=clamp01(.55*p.bandGain*e+.6*field.roll+.3*kick+.9*field.foam);
            // The set lights its own row even when the music already foams.
            const double threshold=.84-.54*music-.28*std::min(1.,field.set);
            auto height=[&](double x){return (field.base-top(x))/reach;};
            const int n=int(crest.size());
            for(int j=0;j<n;) {
                if(height(crest[j].x)<=threshold){++j;continue;}
                const int a=j;double peak=0;
                while(j<n && height(crest[j].x)>threshold){peak=std::max(peak,height(crest[j].x));++j;}
                const double x0=crest[a].x-p.sampleStep*.7,x1=crest[j-1].x+p.sampleStep*.7;
                if(x1-x0<8)continue;
                const double strength=clamp01((peak-threshold)/.22);
                const double thick=(1.1+5.2*z)*(.45+.75*strength)*(1+.8*field.foam+.9*field.set)*p.crestFoam;
                const int m=std::max(4,int((x1-x0)/6));
                for(int pass=0;pass<2;++pass) {
                    const double dy=pass?0:1.4+2.2*z;
                    cv.color(pass?p.foam:p.underprint,pass?std::min(1.,.55+.45*strength):.5);
                    cv.moveTo(x0,top(x0)+dy);
                    for(int k=1;k<=m;++k){const double x=lerp(x0,x1,k/double(m));cv.lineTo(x,top(x)-.7+dy);}
                    for(int k=m;k>=0;--k) {
                        const double u=k/double(m),x=lerp(x0,x1,u);
                        cv.lineTo(x,top(x)+thick*std::pow(std::sin(Pi*u),.8)*(.7+.6*u)+dy);
                    }
                    cv.closePath();cv.fill();
                }
            }
        }
        const double bright=std::min(1.0,.18+p.bandGain*e+p.liftGain*lift+p.kickGain*kick);
        for(int j=0;j<p.innerLines;++j) {
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
            const double size=p.capScale*(.45+.95*hash2(k,row+38+p.seed))*(.38+.62*breath)*(1+p.liftGain*lift)*(1+1.6*kick);
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
    // Each dash owns a hashed depth and lane in a column widening toward the
    // viewer. The treble strand clock sets how fast dashes swap on and off,
    // treble level and lift set how many are lit, and each onset reshuffles
    // the column at once. Lit dashes are flat sun colour, like lamps.
    if(p.glitter>0) {
        const double treble=.6*c.band(5)+.4*c.band(4),sparkle=std::max(c.lift(5),c.lift(4));
        const double clock=c.score?c.score->strandPhase[5]*22:c.t*3;
        const Event* onset=c.score?Score::last(c.score->onsets,c.t):nullptr;
        const double serial=onset?double(onset->serial%100000):0;
        const double duty=std::clamp(.12+.6*treble+.25*sparkle+.45*c.hit(7),0.,.9);
        for(int i=0;i<p.glitter;++i) {
            const double depth=std::pow(hash2(i,p.seed+301),1.6)*p.glintDepth,y=p.horizon+3+depth;
            const double half=p.glitterWidth+depth*p.glitterSpread;
            const double lane=(hash2(i,p.seed+302)+hash2(i,p.seed+303)+hash2(i,p.seed+304))/1.5-1;
            const double rate=.6+.8*hash2(i,p.seed+305),epoch=std::floor(clock*rate+hash2(i,p.seed+306)*7);
            if(hash2(i*3.1+epoch,serial+p.seed+307)>=duty*(1-.55*depth/p.glintDepth))continue;
            const double length=(5+depth*.17)*(.55+.9*hash2(i,p.seed+308)),x=p.glintX+lane*half;
            cv.line(x-length/2,y,x+length/2,y,1.3+depth/120,p.glint,1);
        }
    }
    c.gpu.over(cv,1,0,float(p.opacity));
}
}
