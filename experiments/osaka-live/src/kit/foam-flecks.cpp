#include "foam-flecks.h"
#include <cmath>
#include <algorithm>
namespace Journey::Kit {
namespace {
double rowPulse(const std::deque<Event>&events,double t){double sum=0;
    for(const auto&e:events){double age=t-e.t;if(age>=0 && age<.9)sum+=e.strength*sstep(0,.06,age)*std::exp(-age*5)*(1-sstep(.65,.9,age));}
    return 1-std::exp(-sum);
}
// Mean of the short band bodies: births and sizes follow the loudness of the mix.
double seaLevel(const Ctx& c){
    double sum=0;
    for(int b=0;b<6;++b)sum+=c.score?.6*c.score->bandBody[0][b]+.4*c.score->bandBody[1][b]:c.band(b);
    return clamp01(sum/6*2.2);
}
struct Breaker { int row; double x,birth,life,size,id,spray; };
// A bounded pool, newest first. Each onset or kick seeds a cluster whose
// members are born later the farther they sit from its origin, so a ring of
// crests ripples outward across rows. Hashed ambient lanes keep a few lazy
// crests in quiet passages and fill the sea when the music is loud.
std::vector<Breaker> collectBreakers(const Ctx& c,const FoamFlecksParametersV1& p,double level,
                                    const std::vector<SwellRowV1>& fields,const QPainterPath& mask){
    const auto& s=p.swell;std::vector<Breaker> out;
    auto depthOf=[&](int row){return std::pow(row/double(s.rows-1),s.depthFalloff);};
    auto lifeOf=[&](int row,double id){return p.breakerLife*(.8+.4*id)*(.75+.45*depthOf(row));};
    auto cluster=[&](const Event& e,bool kick){
        const double h=double(e.serial%100000)+(kick?.5:0),strength=clamp01(e.strength);
        const int n=2+int(std::round((kick?18:12)*std::sqrt(strength)*(.3+.7*level)));
        // Prefer open water: retry a hidden origin a few times.
        double ox=0,orow=0;
        for(int attempt=0;attempt<4;++attempt){
            ox=s.region.left()+s.region.width()*hash2(h+attempt*.37,s.seed+201);
            const double pick=hash2(h+attempt*.37,s.seed+202);
            orow=kick?lerp(s.rows*.45,s.rows-2,pick):lerp(1,s.rows-2,pick);
            if(!mask.contains(QPointF(ox,fields[int(std::round(orow))].base)))break;
        }
        const int sources=strength<p.sprayThreshold?0:strength>.8?3:2;
        for(int j=0;j<n && int(out.size())<p.breakers;++j){
            const double radius=j?std::sqrt((j-hash2(h+j,s.seed+203))/n):0,angle=Tau*hash2(h+j,s.seed+204);
            const int row=std::clamp(int(std::round(orow+std::sin(angle)*radius*4)),0,s.rows-1);
            const double id=hash2(h+j,s.seed+205),birth=e.t+.32*radius,life=lifeOf(row,id);
            const double x=ox+std::cos(angle)*radius*(320+520*depthOf(row));
            if(c.t<=birth || c.t>=birth+life || mask.contains(QPointF(x,fields[row].base)))continue;
            out.push_back({row,x,birth,life,
                (kick?1.15:.9)*(.55+.45*strength)*(.7+.5*hash2(h+j,s.seed+206)),id,kick && j<sources?strength:0});
        }
    };
    const double oldest=p.breakerLife*1.5+.4;
    if(c.score){
        auto on=c.score->onsets.rbegin(),bass=c.score->bassHits.rbegin();
        const auto onEnd=c.score->onsets.rend(),bassEnd=c.score->bassHits.rend();
        while(int(out.size())<p.breakers){
            while(on!=onEnd && on->t>c.t)++on;
            while(bass!=bassEnd && bass->t>c.t)++bass;
            const bool kick=bass!=bassEnd && (on==onEnd || bass->t>on->t);
            if(!kick && on==onEnd)break;
            const Event& e=kick?*bass++:*on++;
            if(c.t-e.t>oldest)break;
            cluster(e,kick);
        }
    }
    const int lanes=std::max(3,p.breakers/6);
    for(int lane=0;lane<lanes && int(out.size())<p.breakers;++lane){
        const double period=p.breakerLife*(1.3+1.4*hash2(lane,s.seed+211)),shift=period*hash2(lane,s.seed+212);
        const double slot=std::floor((c.t+shift)/period);
        for(double k=slot-1;k<=slot && int(out.size())<p.breakers;++k){
            const double key=lane*131+k,gate=hash2(key,s.seed+213);
            const double presence=sstep(gate-.12,gate+.12,1.1*level-.1);
            if(presence<.02)continue;
            int row=0;double x=0;
            for(int attempt=0;attempt<4;++attempt){
                row=std::min(s.rows-1,int(hash2(key+attempt*.37,s.seed+214)*s.rows));
                x=s.region.left()+s.region.width()*hash2(key+attempt*.37,s.seed+215);
                if(!mask.contains(QPointF(x,fields[row].base)))break;
            }
            const double id=hash2(key,s.seed+217),birth=k*period-shift+.4*period*hash2(key,s.seed+216),life=lifeOf(row,id);
            if(c.t<=birth || c.t>=birth+life)continue;
            out.push_back({row,x,birth,life,.8*presence*(.7+.5*hash2(key,s.seed+218)),id,0});
        }
    }
    return out;
}
QPainterPath hullPath(std::vector<V2> points){
    QPainterPath envelope;
    if(points.size()<3)return envelope;
    std::sort(points.begin(),points.end(),[](V2 a,V2 b){return a.x<b.x || (a.x==b.x && a.y<b.y);});
    auto cross=[](V2 a,V2 b,V2 q){return (b.x-a.x)*(q.y-a.y)-(b.y-a.y)*(q.x-a.x);};
    std::vector<V2> hull;
    for(auto q:points){while(hull.size()>1 && cross(hull[hull.size()-2],hull.back(),q)<=0)hull.pop_back();hull.push_back(q);}
    const auto lower=hull.size();
    for(auto i=points.rbegin()+1;i!=points.rend();++i){while(hull.size()>lower && cross(hull[hull.size()-2],hull.back(),*i)<=0)hull.pop_back();hull.push_back(*i);}
    envelope.moveTo(hull[0].x,hull[0].y);
    for(auto q:hull)envelope.lineTo(q.x,q.y);envelope.closeSubpath();
    return envelope;
}
// Breakers hug the live wave: its own body outline, which follows the low
// trailing swell closely, plus a hull of only the curl, lip and claws, which
// still seals the barrel.
QPainterPath breakerMask(const Ctx& c,const FoamFlecksParametersV1& p){
    QPainterPath path;
    if(!p.masksWaveTrain)return path;
    for(const auto& crest:WaveTrainV2::worldProfile(c,p.waveTrain).crests){
        if(crest.boundary.size()>2){
            QPainterPath body;body.moveTo(crest.boundary[0].x,crest.boundary[0].y);
            for(auto q:crest.boundary)body.lineTo(q.x,q.y);body.closeSubpath();
            path=path.united(body);
        }
        std::vector<V2> points=crest.outerLip;
        auto add=[&](const auto& v){points.insert(points.end(),v.begin(),v.end());};
        add(crest.foamRim);add(crest.foamInside);
        for(const auto& f:crest.fingers){add(f.left);add(f.right);add(f.shadow.left);add(f.shadow.right);
            for(const auto& st:f.twigs){add(st.left);add(st.right);}}
        for(const auto& st:crest.tangle){add(st.left);add(st.right);}
        for(const auto& st:crest.falling){add(st.left);add(st.right);}
        path=path.united(hullPath(points));
    }
    return path;
}
bool clear(const QRectF& r,const SwellLinesParametersV1& s,const QPainterPath& wave){
    for(const auto& box:s.exclusions)if(box.intersects(r))return false;
    return !wave.intersects(r);
}
// Hokusai crest: the swell lifts into a forward-leaning light face, a cream
// rim runs over its top into an overhanging lip, and two to five claws hang
// from the lip and hook back toward the face. Then the face sinks and the rim
// breaks into foam. A dark key print sits under the whole silhouette.
void paintBreakers(Canvas& cv,const Ctx& c,const FoamFlecksParametersV1& p,
                   const std::vector<SwellRowV1>& fields,const QPainterPath& wave){
    const auto& s=p.swell;
    const auto list=collectBreakers(c,p,seaLevel(c),fields,wave);
    std::vector<std::vector<std::pair<double,double>>> used(s.rows);
    for(const auto& b:list){
        const double age=c.t-b.birth,a=age/b.life;
        if(a<=0 || a>=1)continue;
        const auto& f=fields[b.row];
        const double depth=std::pow(f.z,s.depthFalloff),W=(18+150*depth)*p.breakerScale*b.size*(1+.3*c.kick(6)),H=.42*W;
        if(W<6)continue;
        // Ride the drifting swell line at its phase rate since birth.
        const double role=(1-f.z)*5;const int lo=std::min(4,int(role));
        const double rate=s.driftSpeed*(.81+.37*hash2(b.row,s.seed+13))
            +(c.score?(s.surgeEnabled?1.9:1)*.95*s.flowGain*lerp(c.a.bands[lo],c.a.bands[lo+1],role-lo):0);
        const double cx=b.x+age*rate*f.scale,cy=f.y(cx);
        bool crowded=false;for(auto [x,w]:used[b.row])if(std::abs(x-cx)<.3*(w+W))crowded=true;
        if(crowded)continue;
        const QRectF bounds(cx-.75*W,cy-1.2*H,1.45*W,2.2*H);
        // The near edge and screen sides may clip a crest; masks never do.
        if(!s.region.intersects(bounds) || bounds.top()>1080 || !clear(bounds,s,wave))continue;
        used[b.row].push_back({cx,W});
        const double rise=easeOut(sstep(0,.22,a)),reach=sstep(.12,.45,a),fade=sstep(.5,1,a);
        const double lift=H*rise*(1-.9*fade),curl=rise*(1-fade),xa=cx-.68*W,xb=cx+.24*W,seed=b.id*977;
        // A long rounded back rises to the crest, the lip rolls over the
        // front and a hollow falls back under it to the swell.
        std::vector<V2> line;
        for(int j=0;j<=14;++j){const double u=j/14.,x=lerp(xa,cx,u);
            line.push_back({x,f.y(x)-lift*std::pow(std::sin(Pi/2*u),1.8)});}
        const V2 peak=line.back(),lip{peak.x+.3*W*curl,peak.y+.38*lift},over{peak.x+.24*W*curl,peak.y-.1*lift};
        for(int j=1;j<=8;++j){const double t=j/8.;
            line.push_back(peak*((1-t)*(1-t))+over*(2*(1-t)*t)+lip*(t*t));}
        const V2 hollow{peak.x+.09*W*curl,peak.y+.62*lift};
        std::vector<double> along{0};
        for(std::size_t j=1;j<line.size();++j)along.push_back(along.back()+std::hypot(line[j].x-line[j-1].x,line[j].y-line[j-1].y));
        // A band of the crest line, thickened along its inward normal and
        // pointed at both ends.
        auto band=[&](double from,double thick,double dy){
            const double s0=from*along.back(),span=along.back()-s0;
            if(thick<.4 || span<=1)return;
            std::vector<V2> inner;bool open=false;
            for(std::size_t j=0;j<line.size();++j){
                if(along[j]<s0)continue;
                const V2 q=line[j];
                if(!open){cv.moveTo(q.x,q.y+dy);open=true;}else cv.lineTo(q.x,q.y+dy);
                const V2 d=line[std::min(j+1,line.size()-1)]-line[j?j-1:0];
                const double n=std::max(1e-6,std::hypot(d.x,d.y)),w=(along[j]-s0)/span;
                const double taper=std::pow(std::sin(Pi*std::min(1.,w*1.15)),.6)*(.85+.15*std::sin(w*19+seed));
                inner.push_back({q.x-d.y/n*thick*taper,q.y+d.x/n*thick*taper+dy});
            }
            for(auto i=inner.rbegin();i!=inner.rend();++i)cv.lineTo(i->x,i->y);
            cv.closePath();cv.fill();
        };
        auto silhouette=[&](double dy){
            cv.moveTo(line[0].x,line[0].y+dy);
            for(auto q:line)cv.lineTo(q.x,q.y+dy);
            cv.quadTo(lip.x-.02*W,lip.y+.22*lift+dy,hollow.x,hollow.y+dy);
            cv.quadTo(hollow.x-.03*W,lerp(hollow.y,f.y(xb),.7)+dy,xb,f.y(xb)+dy);
            for(int j=14;j>=0;--j){const double x=lerp(xa,xb,j/14.);cv.lineTo(x,f.y(x)+dy+(1+2*depth)*(1-fade)*std::sin(Pi*j/14.));}
            cv.closePath();cv.fill();
        };
        const int claws=2+int(4*hash2(seed,s.seed+221));
        auto claw=[&](double dy){
            const double hang=lift*1.35*reach*(1-fade);
            if(hang<1.5)return;
            for(int j=0;j<claws;++j){
                const double t=claws>1?j/double(claws-1):1;
                const V2 root=line[14+int(std::round(t*8))]+V2(0,dy);
                const double len=hang*(.5+.5*t)*(.8+.4*hash2(seed+j,s.seed+222)),ang=lerp(.5,1.25,t);
                const double rw=std::max(.6,lift*.09*(1-.4*fade));
                const V2 bend{root.x+.6*len*std::cos(ang),root.y+.6*len*std::sin(ang)};
                const V2 tip{bend.x+.5*len*std::cos(ang+1.5),bend.y+.5*len*std::sin(ang+1.5)};
                cv.moveTo(root.x-rw,root.y-rw);
                cv.quadTo(bend.x+1.4*rw,bend.y-rw,tip.x,tip.y);
                cv.quadTo(bend.x-.2*rw,bend.y+.2*rw,root.x+rw,root.y+rw);
                cv.closePath();cv.fill();
            }
        };
        auto foam=[&](double dy){
            if(fade<=0)return;
            const int bits=3+int(4*hash2(seed,s.seed+223));
            for(int j=0;j<bits;++j){
                const double x=xa+W*(.25+.85*hash2(seed+j,s.seed+224));
                const double rr=(1+3.2*depth)*(.5+.7*hash2(seed+j,s.seed+225))*std::sin(Pi*fade)*(.8+.4*b.size);
                if(rr<.5)continue;
                cv.ellipse(x,f.y(x)-.4*rr+dy,1.6*rr,.8*rr);cv.fill();
            }
        };
        const double key=1+2*depth;
        cv.color(p.underprint,.8);
        if(lift>.8){silhouette(key);band(.3,.3*lift,key);claw(key);}
        foam(key);
        if(lift>.8){
            cv.color(p.body,1);silhouette(0);
            cv.color(p.face,.92);band(.18,.5*lift,0);
            cv.color(p.color,.97);band(.32,.3*lift,0);claw(0);
        }
        cv.color(p.color,.97);foam(0);
        if(b.spray>0 && p.spray>0){
            const double launch=b.birth+.18*b.life;
            for(int pass=0;pass<2;++pass)for(int q=0;q<p.spray;++q){
                const double hq=seed+q*7.3,tau=c.t-launch-.12*hash2(hq,s.seed+231),life=.75+.55*hash2(hq,s.seed+232);
                if(tau<=0 || tau>=life)continue;
                const double vx=(20+170*hash2(hq,s.seed+233))*(.5+depth);
                const double vy=-(200+320*hash2(hq,s.seed+234))*(.45+.75*depth)*(.55+.45*b.spray);
                const double x=cx+W*.3*hash2(hq,s.seed+235)+vx*tau;
                const double y=cy-.9*H+vy*tau+.5*900*(.45+.75*depth)*tau*tau;
                if(tau>.15 && y>f.y(x))continue;
                const double rr=(1+3*depth)*(.55+.6*hash2(hq,s.seed+236))*(1-sstep(.6,1,tau/life));
                if(rr<.45 || x<s.region.left() || x>s.region.right() || !clear(QRectF(x-rr,y-rr,2*rr,2*rr),s,wave))continue;
                cv.color(pass?p.color:p.underprint,pass?.96:.6);
                cv.ellipse(x,y+(pass?0:.6+.8*depth),rr,rr);cv.fill();
            }
        }
    }
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
    if(p.breakers>0)paintBreakers(cv,c,p,fields,breakerMask(c,p));
}
void FoamFlecksV1::draw(Ctx& c,const FoamFlecksParametersV1& p) {
    GpuProfile::Group group(c.gpu.profile,name);Canvas& cv=c.canvas();paint(cv,c,p);c.gpu.over(cv);
}
}
