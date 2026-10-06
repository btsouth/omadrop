#include "wave-train.h"
namespace Journey::Kit {
void CriticalSpringV2::advance(double target,double omega,double dt) {
    // Exact critically damped solution for a held target, stable at every hop.
    const double error=value-target,b=velocity+omega*error,e=std::exp(-omega*dt);
    value=target+(error+b*dt)*e;velocity=(velocity-omega*b*dt)*e;
}
WaveTrainMotionV2::WaveTrainMotionV2() {
    amplitude_.value=pose_.amplitude;q_.value=pose_.q;speed_.value=pose_.phaseSpeed;
}
void WaveTrainMotionV2::advance(const Audio& a,const Score& score,double seconds,double dt) {
    if(!std::isfinite(dt)||dt<=0||dt>.25||!std::isfinite(seconds))return;
    double power=0;for(double b:a.bands)power+=b*b;
    const double energy=clamp01(3.1*std::sqrt(power/6)+.35*clamp01(a.surge));
    // Median inter-onset interval, folded into 60..180 bpm; recent onset rate
    // also raises travel speed. No precomputed track chapters or lookahead.
    std::vector<double> intervals;int count=0;double previous=-1;
    for(const auto& e:score.onsets)if(e.t<=seconds && e.t>seconds-8) {
        ++count;if(previous>=0 && e.t-previous>.12)intervals.push_back(e.t-previous);previous=e.t;
    }
    double tempo=90;
    if(!intervals.empty()) {
        std::sort(intervals.begin(),intervals.end());tempo=60/intervals[intervals.size()/2];
        while(tempo<60)tempo*=2;while(tempo>180)tempo*=.5;
    }
    const double beat=tempo/60,oldSpeed=speed_.value;
    amplitude_.advance(100+340*std::max(0.,a.bassLevel)/(.20+std::max(0.,a.bassLevel)),1.05*beat,dt);
    q_.advance(.2+1.4*energy,1.55*beat,dt);
    speed_.advance(42+22*beat+12*clamp01(count/16.),.65*beat,dt);
    const double mid=std::max(0.,(a.bands[2]+a.bands[3])*.5);
    throw_.advance(165*mid/(.18+mid),2.25*beat,dt);
    pose_.amplitude=amplitude_.value;pose_.q=q_.value;pose_.phaseSpeed=speed_.value;
    pose_.lipThrow=throw_.value;pose_.seconds=seconds;pose_.energy=energy;pose_.tempo=tempo;
    pose_.distance+=(oldSpeed+speed_.value)*.5*dt;pose_.flow+=(.065+.16*energy)*dt;
}
namespace {
QPainterPath polygon(const std::vector<V2>& points) {
    QPainterPath p;p.setFillRule(Qt::WindingFill);if(points.empty())return p;
    p.moveTo(points.front().x,points.front().y);
    for(std::size_t i=1;i<points.size();++i)p.lineTo(points[i].x,points[i].y);
    p.closeSubpath();return p;
}
V2 unit(V2 v) {return v*(1/std::max(.0001,v.len()));}
// Three shorter deep-water components with omega proportional to sqrt(k).
// Their horizontal displacement is small enough to keep the capped sheet
// monotone. The crest window prevents tiny ripples tearing the lip root.
V2 surface(double a,const WaveTrainPoseV2& s,const WaveTrainParametersV2& p,bool raw) {
    const double k=Tau/p.wavelength,theta=k*(a-s.distance-380);
    const double g=WaveTrainV2::envelope(a,s,p),A=s.amplitude*g;
    const double q=(raw?s.q:std::min(.94,s.q))*g;
    V2 v{a-q/k*std::sin(theta),p.waterline-A*std::cos(theta)};
    const double crown=1-.9*std::pow(.5+.5*std::cos(theta),8);
    const double ratio[]={2.37,3.91,6.13},height[]={.042,.020,.009};
    for(int j=0;j<3;++j) {
        const double phase=ratio[j]*k*(a-380)-std::sqrt(ratio[j])*k*s.distance+.7+j*1.31;
        v.x-=crown*.008/(k*ratio[j])*g*std::sin(phase);
        v.y-=crown*height[j]*A*std::cos(phase);
    }
    return v;
}
void fill(Canvas& cv,const QPainterPath& p) {
    // Each subpath is a simple exterior after loop resolution.
    for(const auto& poly:p.toSubpathPolygons()) {
        if(poly.isEmpty())continue;cv.moveTo(poly[0].x(),poly[0].y());
        for(int i=1;i<poly.size();++i)cv.lineTo(poly[i].x(),poly[i].y());cv.closePath();
    }
    cv.fill();
}
}
double WaveTrainV2::envelope(double a,const WaveTrainPoseV2& s,const WaveTrainParametersV2& p) {
    const double d=wrap(a-p.groupOrigin-.5*s.distance+p.groupPeriod*.5,p.groupPeriod)-p.groupPeriod*.5;
    // Periodic Gaussian sets. Neighboring tails make both value and derivative
    // agree across the wrap, and the trough between sets approaches still sea.
    double group=0;for(int j=-2;j<=2;++j) {
        const double u=(d+j*p.groupPeriod)/p.groupWidth;group+=std::exp(-.5*u*u);
    }
    double peak=0;for(int j=-2;j<=2;++j)peak+=std::exp(-.5*std::pow(j*p.groupPeriod/p.groupWidth,2));
    group/=peak;
    // Favor the hero in the left half and reduce the following train.
    // The smooth compositional falloff has no selected-crest switch.
    return (.08+.92*group)*(1-.62*sstep(820,1600,a));
}
WaveTrainProfileV2 WaveTrainV2::profile(const WaveTrainPoseV2& s,const WaveTrainParametersV2& p) {
    WaveTrainProfileV2 out;
    const double k=Tau/p.wavelength;const int samples=int(std::ceil((p.x1-p.x0)/4));
    for(int i=0;i<=samples;++i) {
        const double a=lerp(p.x0,p.x1,i/double(samples));
        out.surface.push_back(surface(a,s,p,false));out.raw.push_back(surface(a,s,p,true));
    }
    auto body=out.surface;body.push_back({body.back().x,p.waterline+p.depth});
    body.push_back({body.front().x,p.waterline+p.depth});out.silhouette=polygon(body);
    double hero=-1;
    const int first=int(std::floor((p.x0-s.distance-380)/p.wavelength));
    const int last=int(std::ceil((p.x1-s.distance-380)/p.wavelength));
    for(int n=first;n<=last;++n) {
        const double a=380+s.distance+n*p.wavelength,g=envelope(a,s,p),q=s.q*g;
        if(a<p.x0 || a>p.x1)continue;
        WaveTrainProfileV2::Crest crest;crest.a=a;crest.envelope=g;crest.q=q;
        if(a>=0 && a<=960 && g>hero){hero=g;out.hero=int(out.crests.size());}
        if(q>1) {
            // The raw Gerstner fold occurs at acos(1/Q). Split there instead
            // of drawing the self-intersecting loop: the body uses Q<=.94,
            // and the loop's forward branch becomes a finite thickness lip.
            // Stop at 1.28*fold, before the branch returns to its intersection.
            // No falling particle, ballistic plunge, impact or crash clock.
            const V2 root=surface(a,s,p,false);
            const double fold=std::acos(1/q),curl=sstep(1,1.6,q),A=s.amplitude*g;
            // Locate the loop crossing u=Q*sin(u). Retain its upper lobe and
            // union it with the supported sheet; its lower returning branch
            // cannot leave a pinched, intersecting or detached water contour.
            double lo=fold,hi=Pi;
            for(int it=0;it<40;++it){const double u=(lo+hi)*.5;if(u<q*std::sin(u))lo=u;else hi=u;}
            const double crossing=(lo+hi)*.5;std::vector<V2> lobe;
            for(int j=0;j<=128;++j) {
                const double u=lerp(-crossing,crossing,j/128.);
                lobe.push_back({root.x+(u-q*std::sin(u))/k,root.y+A*(1-std::cos(u))});
            }
            out.silhouette=out.silhouette.united(polygon(lobe));
            std::vector<V2> inner;
            for(int j=0;j<=64;++j) {
                const double u=fold*lerp(-1.,1.28,j/64.);
                // Throw reaches its full displacement at the fold with zero
                // derivative. Beyond it the physical branch must turn inward;
                // mid energy cannot straighten a full curl into a bent flap.
                const double w=clamp01(u/fold),throwOffset=s.lipThrow*curl*sstep(0,1,w);
                const double throwSlope=u>0 && u<fold?s.lipThrow*curl*6*w*(1-w)/fold:0;
                const V2 v{root.x+(q*std::sin(u)-u)/k+throwOffset,root.y+A*(1-std::cos(u))};
                const V2 tangent{(q*std::cos(u)-1)/k+throwSlope,A*std::sin(u)};
                const V2 normal=unit({-tangent.y,tangent.x});
                const double thickness=(4+22*curl)*std::sin(Pi*(.10+.84*j/64.))*sstep(1,1.08,q);
                crest.outerLip.push_back(v);inner.push_back(v+normal*thickness);
            }
            auto lip=crest.outerLip;for(auto it=inner.rbegin();it!=inner.rend();++it)lip.push_back(*it);
            // The supported lobe overlaps the body. Boolean union resolves
            // the finite-thickness crown into a clean exterior contour.
            out.silhouette=out.silhouette.united(polygon(lip));
        }
        out.crests.push_back(std::move(crest));
    }
    out.silhouette=out.silhouette.simplified();
    // The physical hollow opens toward the following trough. Closed internal
    // voids are not a wave state: fill the subpixel slivers left by Boolean
    // intersections of the supported sheet and the finite-thickness crown.
    QPainterPath exterior;exterior.setFillRule(Qt::WindingFill);
    for(const auto& poly:out.silhouette.toSubpathPolygons()) {
        double area=0;for(int i=0;i+1<poly.size();++i)area+=poly[i].x()*poly[i+1].y()-poly[i].y()*poly[i+1].x();
        if(area>0){exterior.addPolygon(poly);exterior.closeSubpath();}
    }
    out.silhouette=exterior;
    for(const auto& poly:out.silhouette.toSubpathPolygons()) {
        std::vector<V2> points;for(const auto& q:poly)points.push_back({q.x(),q.y()});
        if(points.size()>1 && (points.front()-points.back()).len()<.0001)points.pop_back();
        out.boundaries.push_back(std::move(points));
    }
    return out;
}
void WaveTrainV2::paint(Canvas& cv,const WaveTrainProfileV2& f,const WaveTrainPoseV2& s,const WaveTrainParametersV2& p) {
    cv.linear(0,p.waterline-s.amplitude,0,p.waterline+p.depth,
        {{0,p.body,1},{.53f,mix(p.body,p.bottom,.55),1},{1,p.bottom,1}});fill(cv,f.silhouette);
    // Depth offset curves of the moving sheet, with secondary phase lag and
    // upward advection. Fade each contour before wrapping at either boundary.
    for(int row=0;row<21;++row) {
        const double d=wrap((row+.4)/21.-s.flow*.12,1.),fade=sstep(0,.035,d)*(1-sstep(.94,1,d));
        std::vector<V2> line;
        auto flush=[&]() {
            if(line.size()>1) {
                cv.polyline(line,row%4==0?7:3,p.underprint,.65*fade);
                cv.polyline(line,row%4==0?2:1.1,row%7==0?p.foam:p.lines,(row%7==0?.42:.55)*fade);
            }line.clear();
        };
        WaveTrainPoseV2 lag=s;lag.distance-=d*(8+12*s.energy);
        // Depth offsets follow gravity, with the orbit attenuated at depth.
        // Normal offsets beyond the local radius of curvature fold into
        // crosshatching at a peaked crest, so do not use them here.
        lag.amplitude*=std::exp(-.72*d);lag.q*=std::exp(-.85*d);
        for(double a=p.x0;a<=p.x1;a+=9) {
            const V2 v=surface(a,lag,p,false);
            const V2 q=v+V2(0,d*270+d*d*180);
            if(!f.silhouette.contains({q.x,q.y})){flush();continue;}line.push_back(q);
        }flush();
    }
    // W1 only: a continuous foam band, no band fingers or detached spray.
    std::vector<V2> rim;double rimOpacity=1;
    auto flush=[&]() {if(rim.size()>1) {
        cv.polyline(rim,5+8*s.q*s.q,p.underprint,.9*rimOpacity);
        cv.polyline(rim,3+6*s.q*s.q,p.foam,.9*rimOpacity);
        cv.polyline(rim,1.2,p.lines,.45*rimOpacity);
    }rim.clear();};
    for(std::size_t i=1;i+1<f.surface.size();++i) {
        const V2 v=f.surface[i],t=f.surface[i+1]-f.surface[i-1];
        const double slope=std::abs(t.y)/std::max(.01,std::abs(t.x));
        const double local=wrap(Tau/p.wavelength*(v.x-s.distance-380)+Pi,Tau)-Pi;
        const bool crest=v.y<p.waterline-s.amplitude*.60 && local<=.03;
        double opacity=1;
        for(const auto& cap:f.crests)if(std::abs(v.x-cap.a)<p.wavelength*.18)opacity=1-sstep(1,1.25,cap.q);
        if(std::abs(opacity-rimOpacity)>.0001){flush();rimOpacity=opacity;}
        if(crest && (slope>.62 || v.y<p.waterline-s.amplitude*.7))rim.push_back(v);else flush();
    }flush();
    for(const auto& crest:f.crests)if(crest.outerLip.size()>1) {
        cv.polyline(crest.outerLip,3+6*crest.q*crest.q,p.foam,.96*sstep(1,1.08,crest.q));
        cv.polyline(crest.outerLip,1.4,p.lines,.55*sstep(1,1.08,crest.q));
        // The same physical crown supplies the interior ink, not a separate
        // drawn curl. Its depth phase moves upward through the supported lobe.
        for(int row=0;row<5;++row) {
            const double d=wrap(row/5.-s.flow*.12,1.);
            std::vector<V2> ink;
            auto flushInk=[&](){if(ink.size()>1)cv.polyline(ink,1.2,p.lines,.42*sstep(1,1.08,crest.q)*window(d,0,.1,1,.1));ink.clear();};
            for(std::size_t i=1;i+1<crest.outerLip.size();++i) {
                const V2 t=crest.outerLip[i+1]-crest.outerLip[i-1];
                const V2 v=crest.outerLip[i]+unit({-t.y,t.x})*(6+35*d);
                if(f.silhouette.contains({v.x,v.y}))ink.push_back(v);else flushInk();
            }flushInk();
        }
    }
}
void WaveTrainV2::draw(Ctx& c,const WaveTrainPoseV2& s,const WaveTrainParametersV2& p) {
    GpuProfile::Group group(c.gpu.profile,name);auto f=profile(s,p);
    Canvas& cv=c.canvas();paint(cv,f,s,p);c.gpu.over(cv);
}
}
