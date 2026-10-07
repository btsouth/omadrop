#include "boat-on-water.h"
#include "../rig.h"
#include "../schedule.h"
#include <QTransform>
namespace Journey::Kit {
namespace {
// Horizontal scale of a hull turning between facing right (1) and left (-1):
// the projection of a steady turn, kept off edge-on.
double turned(double facing){const double v=std::sin(Pi/2*std::clamp(facing,-1.,1.));
    return (v<0?-1:1)*std::max(.28,std::abs(v));}
}
V2 BoatOnWaterV1::keel(const BoatOnWaterParametersV1&p,double u){
    const double sc=p.scale,ln=p.length*sc,v=1-u;
    // The actual lower hull Bezier, not a rectangle beneath the raised ends.
    return V2(ln/2-4*sc,10*sc)*(v*v*v)+V2(ln/4,26*sc)*(3*v*v*u)
        +V2(-ln/4,26*sc)*(3*v*u*u)+V2(-ln/2,6*sc)*(u*u*u);
}
double BoatOnWaterV1::surfaceY(const Ctx& c,const BoatOnWaterParametersV1& p,double x,double row) {
    row=std::clamp(row,0.,double(p.swell.rows-1));
    const int lo=std::min(p.swell.rows-2,int(row));
    const double sea=lerp(SwellLinesV1::field(c,lo,p.swell).y(x),SwellLinesV1::field(c,lo+1,p.swell).y(x),row-lo);
    const double depth=1-sstep(0,2,std::max(0.,p.wave.row-row));
    if(p.ridesWaveTrain && !p.waveTrain.setCycle)return std::min(sea,lerp(sea,WaveTrainV2::surfaceY(WaveTrainV2::worldProfile(c,p.waveTrain),x,p.waveTrain.waterline),depth));
    return p.ridesWave?lerp(sea,GreatWaveV1::field(c,p.wave).y(x,sea),depth):sea;
}
BoatOnWaterPoseV1 BoatOnWaterV1::pose(const Ctx& c,const BoatOnWaterParametersV1& p) {
    const double phase=Tau*hash2(p.seed,41),clock=c.t*p.driftSpeed;
    // Continuous bounded paths with incommensurate rates, no wrap or waypoint
    // seam. Seed changes both the lane and its slow rhythm over many minutes.
    auto drift=[&](double t){return .65*std::sin(t+phase)+.35*std::sin(t*std::sqrt(2.)+phase*.73);};
    BoatOnWaterPoseV1 s;s.at.x=p.x+p.driftX*(drift(clock)-drift(0))*.5;
    s.row=p.row+p.driftRows*(std::sin(clock*std::sqrt(3.)+phase)-std::sin(phase))*.5;
    // The race: each set the schedule decides whether the crews outrun it,
    // and the lanes carry them toward an edge or the landing and back. A
    // swamped hull is thrown over into the foam and a fresh crew rows in;
    // a boat that rides the landing is pitched hard and carried, then rows home.
    double race=0,sunk=0,thrown=0,pitched=0,braced=0,lifted=0;
    if(p.race && c.schedule && p.waveTrain.authored) {
        const auto& k=c.schedule->print;const auto& L=k.lanes[p.race==1?0:1];
        const double home=s.at.x,age=c.t-k.crashStart;
        const bool caught=L.fate!=BoatFate::Escape,landed=k.crashStart>L.since;
        if(L.state==RaceLaneV1::Entering)s.at.x=lerp(-260.,home,easeOut((c.t-L.since)/13));
        else {const double goal=caught?L.landing-80:L.direction>0?(L.offscreen?2250.:1760.):(L.offscreen?-330.:150.);
            s.at.x=home+L.frac*(goal-home);}
        s.facing=L.facing;
        if(L.state==RaceLaneV1::Fleeing || L.state==RaceLaneV1::Caught)race=k.raceBuild(c.t);
        if(L.state==RaceLaneV1::Caught && !landed)braced=sstep(.5,1.,k.setRolling()?sstep(k.setLaunch,k.setArrival(),c.t):0.);
        if(L.state==RaceLaneV1::Caught && landed && age>=0) {
            const double u=clamp01((age-.2)/1.);
            if(L.fate==BoatFate::Swamped){thrown=u>0 && u<1?u:0;sunk=u>=1?1:0;}
            else {lifted=80*std::sin(Pi*clamp01((age-.15)/1.1));
                pitched=.42*sstep(0,.5,age)*std::exp(-std::max(0.,age-.5)/1.2)*std::cos((age-.5)*3.6);braced=1-sstep(1.5,3,age);}
            race=0;
        }
        // The far crew stays for a small set and braces as it stands.
        if(p.race==2 && L.state==RaceLaneV1::Home && k.plan.farStays)braced=sstep(.45,1.,k.setCharge)+(k.setRolling()?1.:0.);
    }
    const double half=p.length*p.scale*.45;
    const auto wave= p.ridesWave && !p.ridesWaveTrain?GreatWaveV1::field(c,p.wave):GreatWaveFieldV1{};
    const auto* train=p.ridesWaveTrain?&WaveTrainV2::worldProfile(c,p.waveTrain):nullptr;
    // In a set cycle the boats keep their own sea: the near lane passes in
    // front of the breaking crest and the far lane behind it.
    const auto* ride=p.waveTrain.setCycle?nullptr:train;
    const int lo=std::min(p.swell.rows-2,int(std::clamp(s.row,0.,double(p.swell.rows-1))));
    const auto low=SwellLinesV1::field(c,lo,p.swell),high=SwellLinesV1::field(c,lo+1,p.swell);
    const double depth=1-sstep(0,2,std::max(0.,p.wave.row-s.row));
    auto support=[&](double x){const double sea=lerp(low.y(x),high.y(x),s.row-lo);return ride?std::min(sea,lerp(sea,WaveTrainV2::surfaceY(*train,x,p.waveTrain.waterline),depth)):(p.ridesWave?lerp(sea,wave.y(x,sea),depth):sea);};
    // The overhang is not navigable water. Move the full hull outward before
    // a growing set reaches its lane, then keep to the lower descending face.
    // This uses the drawn lip bounds, including its forward travel and curl.
    double escape=0;
    if(ride && depth>0) {
        const double original=s.at.x;
        // Navigate the trough to the right of each arriving set. Exclusion
        // covers the full hull, cream hooks, blue peaks and barrel tendrils.
        // Leave room for the lower face to meet the travelling swell, so a
        // rigid hull does not bridge the face/sea junction.
        // The next crest is one group period away, leaving a navigable lower face.
        for(const auto& crest:train->crests) {
            // A risen or sunk set lies flat in the sea; the hull rows over it.
            if(crest.envelope<.2)continue;
            double left=1e9;for(auto q:crest.boundary)left=std::min(left,q.x);
            // Navigate from the slower body lip, with a conservative detail
            // reserve. Following individual claw flicks would jerk the hull.
            double lip=-1e9;for(auto q:crest.outerLip)lip=std::max(lip,q.x);
            lip+=120;
            if(s.at.x+half*1.4+60<left)continue;
            double safe=std::max(s.at.x,lip+half*1.4+260);
            auto lowEnough=[&](double x){return WaveTrainV2::surfaceY(*train,x-half*1.3,p.waveTrain.waterline)>=p.waveTrain.waterline-145;};
            if(!lowEnough(safe)){double a=safe,b=safe+p.waveTrain.width;
                for(int j=0;j<24;++j){const double m=(a+b)*.5;if(lowEnough(m))b=m;else a=m;}safe=b;}
            s.at.x=safe;
        }
        escape=clamp01((s.at.x-original)/320);
    } else if(p.ridesWave && !p.ridesWaveTrain && depth>0){
        const auto set=GreatWaveV1::pose(c,p.wave);
        const double side=p.wave.anchorRight?-1.:1.;
        double lip=-1e9;for(int j=16;j<48;++j)lip=std::max(lip,side*wave.face[j].x);
        const double original=side*s.at.x;
        double safe=std::max(original,lip+half*1.4+60);
        auto lowEnough=[&](double outward){const double x=side*(outward-half*1.3);
            const double sea=lerp(low.y(x),high.y(x),s.row-lo);
            return wave.y(x,sea)>=sea-15;};
        if(!lowEnough(safe)){double a=safe,b=safe+p.wave.width;
            for(int j=0;j<24;++j){const double m=(a+b)*.5;if(lowEnough(m))b=m;else a=m;}safe=b;}
        escape=sstep(0,.14,set.phase)*(1-sstep(.94,1,set.phase));
        s.at.x=side*lerp(original,safe,escape);
    }
    // A tall crest hanging close behind the hull makes the crew brace.
    double towering=0;
    if(train && depth>0)for(const auto& crest:train->crests){
        const double gap=s.at.x-half-WaveTrainV2::exclusionRight(crest);
        if(gap>-80)towering=std::max(towering,sstep(2.6,4.6,crest.stage)*(1-sstep(140,520,gap)));
    }
    // The shared perspective row remains the local sea in the safe lane.
    // A hero face can lift it, but must not replace it with a travelling
    // reference swell whose slope the escaping boat would follow forever.
    // Reserve the complete hull at either pitch, including the raised bow.
    if(ride) {
        const double hullRadius=std::hypot((p.length*.5+26)*p.scale,26*p.scale);
        const double bounded=std::clamp(s.at.x,hullRadius+16,1920-hullRadius-16);
        bool clear=true;
        for(const auto& crest:train->crests) {
            if(crest.envelope<.2)continue;
            double left=1e9;for(auto q:crest.boundary)left=std::min(left,q.x);
            if(bounded+half*1.4+20>=left && bounded-half*1.4<=WaveTrainV2::exclusionRight(crest)+20)clear=false;
        }
        // Never trade lip/barrel clearance for screen placement when a later
        // set leaves no onscreen lane. The 90 s world lane has room for both.
        if(clear)s.at.x=bounded;
    }
    const double left=support(s.at.x-half),right=support(s.at.x+half);
    s.waterline=(left+2*support(s.at.x)+right)*.25;
    s.tilt=std::atan2(right-left,2*half);
    if(c.schedule && train && p.waveTrain.setCycle) {
        // The set lifts the bow as it arrives and drops it as it passes;
        // the break rocks the boats nearest its row hardest.
        const auto& k=c.schedule->print;const double z=s.row/double(p.swell.rows-1),age=c.t-k.crashStart;
        const double slope=(k.setRoll(z,c.t+.2)-k.setRoll(z,c.t-.2))/.4;
        const double rock=age>0 && age<7?k.crashStrength*std::exp(-age/1.8)*std::sin(age*3.1)*(1-sstep(.08,.3,std::abs(z-k.setWaveRow))):0;
        s.tilt-=.07*slope*p.pitchGain+.12*rock*p.pitchGain;
    }
    // Exaggerated pitch reads at print scale; the keel fit below keeps the
    // rigid hull on its most demanding support point.
    if(p.ridesWave)s.tilt=.55*std::tanh(p.pitchGain*s.tilt/.55);
    s.at.y=s.waterline-14*p.scale*std::cos(s.tilt);
    if(thrown>0){s.at.y-=170*std::sin(Pi*thrown)-120*thrown*thrown;s.tilt-=2.6*easeIn(thrown)+.4*std::sin(Pi*thrown);}
    s.at.y-=lifted;s.tilt-=pitched*s.facing;
    if(sunk>0)s.hidden=true;
    if(p.ridesWave){
        // Fit the rigid hull to the lower face after excluding the overhang.
        // The near hull sits entirely above the shared face. Fit the rigid hull
        // to its most demanding support point, not only its average centre.
        for(int j=0;j<=32;++j){const auto q=keel(p,j/32.);
            const double x=s.at.x+q.x*std::cos(s.tilt)-q.y*std::sin(s.tilt);
            s.at.y=std::min(s.at.y,support(x)-q.x*std::sin(s.tilt)-q.y*std::cos(s.tilt));}
    }
    const double surge=p.surgeEnabled && c.schedule?c.schedule->print.surge(c.t):0;
    double body=0;if(c.score){for(double b:c.score->bandBody[0])body+=b*b;body=std::sqrt(body/6);}else body=c.band(p.band);
    s.effort=clamp01(3.2*body+.45*c.kick(4));
    s.urgency=clamp01(1.4*escape+.6*towering+.8*surge);
    s.brace=clamp01(std::max(1.2*towering+.35*surge,braced));
    if(c.schedule){
        // Catches land on the shared beat clock; each lane keeps a slight lag.
        s.stroke=c.schedule->print.rowPhase-.03*hash2(p.seed,43);s.rate=c.schedule->print.rowRate;
    }else{
        // Integral makes tempo change continuously; never multiply time by the
        // current band/kick (that would jump the pose on every beat).
        s.stroke=c.t*p.rowingTempo+p.tempoGain*(c.score?c.score->bandIntegrals[p.band]:0)+hash2(p.seed,43);s.rate=p.rowingTempo;
    }
    s.urgency=std::max(s.urgency,race);
    s.splash=std::clamp(.10+p.splashGain*(.55*c.band(p.band)+.45*c.kick(5))+.35*s.urgency,0.,.8);
    s.spray=std::clamp(p.kickGain*c.kick(6)+.45*surge+.3*s.urgency*s.effort,0.,.8);
    return s;
}
BoatGullPoseV1 BoatOnWaterV1::gullPose(const Ctx& c,const BoatOnWaterParametersV1&p){
    BoatGullPoseV1 out;if(!p.gullVisits || !c.schedule)return out;
    const auto& clock=c.schedule->print;const double age=c.t-clock.gull.start;
    if(age<0 || age>clock.gull.duration)return out;
    auto bow=[&](const Ctx& at){const auto b=pose(at,p);const V2 q(turned(b.facing)*p.length*p.scale*.48,-8*p.scale);
        return b.at+V2(q.x*std::cos(b.tilt)-q.y*std::sin(b.tilt),q.x*std::sin(b.tilt)+q.y*std::cos(b.tilt));};
    out.at=bow(c);out.alpha=sstep(0,1,age);out.flap=std::sin(c.t*5.2);
    if(clock.gullTakeoff>=clock.gull.start && c.t>=clock.gullTakeoff){
        const double fly=c.t-clock.gullTakeoff;Ctx born=c;born.t=clock.gullTakeoff;
        out.at=bow(born)+V2(95*fly,-75*fly-13*fly*fly);
        out.alpha=1-sstep(3,4,fly);out.flap=std::sin(fly*8);
    }else if(age<8){
        const double approach=1-sstep(4,8,age),angle=age*1.12;
        out.at=out.at+V2(140*approach*std::cos(angle),-24*approach-85*approach*(.65+.35*std::sin(angle)));
    }else{out.landed=true;out.flap=0;}
    return out;
}
QPainterPath BoatOnWaterV1::responsePath(const BoatOnWaterPoseV1& s,const BoatOnWaterParametersV1& p) {
    QPainterPath area;const double ln=p.length*p.scale,sc=p.scale;
    area.addRect(QRectF(-ln/2-35*sc,-60*sc,ln+100*sc,145*sc));
    QTransform transform;transform.translate(s.at.x,s.at.y);transform.rotateRadians(s.tilt);
    return transform.map(area);
}
void BoatOnWaterV1::paint(Canvas& cv,const Ctx& c,const BoatOnWaterParametersV1& p) {
    const auto s=pose(c,p);const double sc=p.scale,ln=p.length*sc;
    if(s.hidden)return;
    const auto wave=p.ridesWave && !p.ridesWaveTrain?GreatWaveV1::field(c,p.wave):GreatWaveFieldV1{};
    const auto* train=p.ridesWaveTrain && !p.waveTrain.setCycle?&WaveTrainV2::worldProfile(c,p.waveTrain):nullptr;
    const int lo=std::min(p.swell.rows-2,int(s.row));
    const auto low=SwellLinesV1::field(c,lo,p.swell),high=SwellLinesV1::field(c,lo+1,p.swell);
    const double depth=1-sstep(0,2,std::max(0.,p.wave.row-s.row));
    auto support=[&](double x){double sea=lerp(low.y(x),high.y(x),s.row-lo);return train?std::min(sea,lerp(sea,WaveTrainV2::surfaceY(*train,x,p.waveTrain.waterline),depth)):(p.ridesWave?lerp(sea,wave.y(x,sea),depth):sea);};
    const double ca=std::cos(s.tilt),sa=std::sin(s.tilt);
    // A turning hull is drawn foreshortened, never edge-on; dir is its bow side.
    const double fx=turned(s.facing),dir=fx<0?-1:1,afx=std::abs(fx);
    auto wp=[&](V2 q){return s.at+V2(fx*q.x*ca-q.y*sa,fx*q.x*sa+q.y*ca);};
    // A narrow waterline seam and two broken trailing ribbons bind the
    // ochre hull to its sampled swell. Rowing and the travelling set enlarge
    // them; none of these marks changes the clearance or rocking solution.
    const double phase=Tau*hash2(p.seed,41),clock=c.t*p.driftSpeed;
    const double driftSpeed=std::abs(p.driftX*p.driftSpeed*.5*
        (.65*std::cos(clock+phase)+.35*std::sqrt(2.)*std::cos(clock*std::sqrt(2.)+phase*.73)));
    const double escapeSpeed=train && c.schedule?
        sstep(40,320,std::abs(s.at.x-p.x))*c.schedule->waveTrain.pose().phaseSpeed*p.waveTrain.travelScale:0;
    const double pace=clamp01(.12+.02*(driftSpeed+escapeSpeed)+s.splash+s.spray*.5+std::abs(s.tilt)*1.2+.45*s.urgency);
    for(int j=0;j<2;++j){
        const double span=(24+45*pace)*(1-.28*j)*sc;
        const double x0=dir>0?s.at.x-ln*.46*afx-span:s.at.x+ln*.46*afx;
        const int n=12;
        cv.color(p.foam,(.36+.30*pace)*(1-.28*j));
        cv.moveTo(x0,support(x0)+(2+j*5)*sc);
        for(int k=1;k<=n;++k){const double u=k/double(n),x=x0+span*u;
            cv.lineTo(x,support(x)+(2+j*5)*sc);}
        for(int k=n;k>=0;--k){const double u=k/double(n),x=x0+span*u;
            cv.lineTo(x,support(x)+(2+j*5)*sc+(1.2+3*pace)*sc*std::sin(Pi*u));}
        cv.closePath();cv.fill();
    }
    // The cream seam follows the keel itself, so steep pitch cannot detach
    // the wake from either raised end.
    std::vector<V2> seam;
    for(int k=3;k<=29;++k)seam.push_back(wp(keel(p,k/32.)));
    cv.polyline(seam,(1.1+pace)*sc,p.foam,.45+.3*pace);
    const double nose=s.at.x+dir*ln*.48*ca*afx;
    const double bowSpan=(8+20*pace)*sc;
    auto ahead=[&](double d){return nose+dir*d;};
    cv.color(p.foam,.65+.2*pace);cv.moveTo(ahead(-5*sc),support(ahead(-5*sc)));
    cv.curveTo(ahead(6*sc),support(ahead(6*sc))-5*sc,
        ahead(bowSpan),support(ahead(bowSpan))-3*sc,
        ahead(bowSpan+3*sc),support(ahead(bowSpan+3*sc))+2*sc);
    cv.curveTo(ahead(bowSpan*.6),support(ahead(bowSpan*.6))+sc,
        ahead(6*sc),support(ahead(6*sc))+4*sc,ahead(-5*sc),support(ahead(-5*sc))+sc);
    cv.closePath();cv.fill();
    cv.save();cv.translate(s.at.x,s.at.y);cv.rotate(s.tilt);cv.scale(fx,1);
    // Journey drawBoat: shallow raised ends, ochre side and distinct gunwale.
    cv.color(p.ink);cv.moveTo(-ln/2-26*sc,-16*sc);
    cv.curveTo(-ln/4,14*sc,ln/4,14*sc,ln/2+12*sc,-8*sc);
    cv.lineTo(ln/2-4*sc,10*sc);cv.curveTo(ln/4,26*sc,-ln/4,26*sc,-ln/2,6*sc);cv.closePath();cv.fill();
    cv.color(p.hull);cv.moveTo(-ln/2-20*sc,-13*sc);
    cv.curveTo(-ln/4,12*sc,ln/4,12*sc,ln/2+6*sc,-6*sc);
    cv.lineTo(ln/2-6*sc,6*sc);cv.curveTo(ln/4,20*sc,-ln/4,20*sc,-ln/2,3*sc);cv.closePath();cv.fill();
    // A paddling stroke facing the bow: reach and plunge on the catch (the
    // beat), a fast pull back, then a slower airborne return. Louder music
    // lengthens the reach; a towering crest makes the crew lean back with
    // blades raised, rowing again as it passes.
    struct Stroke {double body=0,depth=0,lift=0,age=0;};
    auto strokeAt=[&](double phase){const double u=phase-std::floor(phase);Stroke k;
        if(u<.42){const double d=u/.42;k.body=-1+2*(1-(1-d)*(1-d));k.depth=sstep(0,.06,u)*(1-sstep(.34,.42,u));}
        else{const double d=(u-.42)/.58;k.body=1-2*d*d*(3-2*d);k.lift=std::sin(Pi*d);}
        k.age=u/std::max(.25,s.rate);return k;};
    const double amp=.5+.5*std::max(s.effort,s.urgency),row=1-s.brace;
    std::vector<std::array<V2,2>> oars;std::vector<Stroke> strokes;
    for(int i=0;i<p.crewCount;++i){
        const double f=(i+.5)/p.crewCount,bx=-ln/2+ln*(.12+.72*f),by=2*sc+8*sc*std::sin(f*Pi);
        const auto k=strokeAt(s.stroke-i*.018);const double b=k.body*amp*row;
        RigIn r;r.h=62*sc;r.facing=1;r.hair=Hair::Short;r.garment=Garment::Jacket;
        r.hip={bx,by-(13-5*s.brace)*sc};r.lean=.12-(.16+.16*amp)*b-.30*s.brace;
        r.footF={bx+21*sc,by+3*sc};r.footB={bx+7*sc,by+5*sc};
        const V2 wanted(bx+(18-13*b-4*s.brace)*sc,by-(15+3*b+12*s.brace)*sc);
        const V2 shoulder=r.hip+V2(std::sin(r.lean),-std::cos(r.lean))*(.275*r.h),reach=wanted-shoulder;
        const V2 grip=shoulder+reach*std::min(1.,.29*r.h/std::max(1e-6,reach.len()));
        r.handF=grip;r.handB=grip+V2(-5*sc,sc);r.headTilt=-.10*b-.16*s.brace;
        const auto body=solve(r);drawBody(cv,body,p.ink);
        cv.line(body.shoulder.x,body.shoulder.y+2*sc,body.chest.x,body.chest.y+10*sc,4*sc,p.trim,.55);
        if(i<p.oarCount){
            V2 blade=grip+V2((24-16*b+6*s.brace)*sc,(34+6*k.depth)*sc);
            const V2 world=wp(blade);
            // In-water drive touches the sampled swell; the return lifts the
            // blade clear, and bracing holds it high. No independent bob.
            const double raised=(1-k.depth)*(5+(10+14*amp)*k.lift*row)+44*s.brace;
            const double water=support(world.x)+1.5*sc*k.depth-raised*sc;
            blade.y=(water-s.at.y-fx*blade.x*sa)/ca;
            // A hull riding high on a face cannot reach the trough: the oar
            // keeps its length and the blade stays dry, with no splash.
            const double reach=70*sc;const V2 shaft=blade-grip;
            auto stroke=k;if(shaft.len()>reach){blade=grip+shaft*(reach/shaft.len());stroke.age=1e9;}
            oars.push_back({grip,blade});strokes.push_back(stroke);
        }
    }
    // Journey's front face hides seated knees, then shafts are redrawn on top.
    cv.color(p.ink);cv.moveTo(-ln/2-20*sc,-10*sc);
    cv.curveTo(-ln/4,13*sc,ln/4,13*sc,ln/2+8*sc,-5*sc);
    cv.lineTo(ln/2-4*sc,10*sc);cv.curveTo(ln/4,26*sc,-ln/4,26*sc,-ln/2,6*sc);cv.closePath();cv.fill();
    cv.color(p.hull);cv.moveTo(-ln/2-18*sc,-7*sc);
    cv.curveTo(-ln/4,14*sc,ln/4,14*sc,ln/2+4*sc,-3*sc);
    cv.lineTo(ln/2-6*sc,6*sc);cv.curveTo(ln/4,20*sc,-ln/4,20*sc,-ln/2,3*sc);cv.closePath();cv.fill();
    cv.color(p.trim,.85);cv.moveTo(-ln/2-20*sc,-13*sc);
    cv.curveTo(-ln/4,12*sc,ln/4,12*sc,ln/2+6*sc,-6*sc);cv.stroke(2.6*sc);
    for(const auto& shaft:oars){const V2 grip=shaft[0],blade=shaft[1];
        cv.line(grip.x-8*sc,grip.y-9*sc,blade.x,blade.y,1.8*sc,p.trim);
        cv.line(blade.x-3*sc,blade.y-7*sc,blade.x+3*sc,blade.y+7*sc,4*sc,p.hull);}
    cv.restore();
    // Each catch throws a crisp cream crown and droplets back from the blade.
    for(std::size_t i=0;i<oars.size();++i){
        const auto& k=strokes[i];if(k.age>.55 || s.brace>.7)continue;
        const V2 entry=wp(oars[i][1]);const double base=support(entry.x),a=k.age;
        // Splash keeps a legible size on the distant hull.
        const double force=(.55+.45*amp)*(1-s.brace),ss=.35+.5*sc;
        const double crown=1-sstep(.18,.40,a);
        if(crown>0){cv.color(p.foam,.92);const double w=(7+20*sstep(0,.25,a))*ss*force,h=(3+8*force)*ss*crown;
            cv.moveTo(entry.x-w,base);cv.curveTo(entry.x-w*.6,base-h,entry.x-w*.2,base-h*1.3,entry.x,base-h*.2);
            cv.curveTo(entry.x+w*.2,base-h*1.3,entry.x+w*.6,base-h,entry.x+w,base);cv.closePath();cv.fill();}
        const int drops=3+int(4*force);
        for(int j=0;j<drops;++j){const double id=hash2(i*7+j,p.seed+71),id2=hash2(i*7+j,p.seed+73);
            const double vx=-dir*(35+110*id)*ss,vy=(120+150*id2)*ss*(.55+.7*force),g=900*ss;
            const double x=entry.x+vx*a,y=base-vy*a+.5*g*a*a;if(y>support(x)-.5)continue;
            const double r=(2+2.2*id2)*ss*(1-sstep(.30,.55,a));
            cv.color(p.foam,.95);cv.ellipse(x,y,r*.75,r*1.25);cv.fill();}
    }
    const auto gull=gullPose(c,p);
    if(gull.alpha>0){cv.save();cv.translate(gull.at.x,gull.at.y);cv.scale(dir*sc*1.8,sc*1.8);
        if(gull.landed){
            cv.line(-3,0,-2,-4,.8,p.ink,gull.alpha);cv.line(1,0,2,-4,.8,p.ink,gull.alpha);
            cv.color(p.foam,gull.alpha);cv.ellipse(-1,-7,6,3.6);cv.fill();
            cv.color(p.ink,.8*gull.alpha);cv.moveTo(-6,-8);cv.curveTo(-3,-10,0,-8,2,-6);cv.stroke(1);
            cv.disc(4,-11,2.7,p.foam,gull.alpha);cv.disc(5,-11, .55,p.ink,gull.alpha);
            cv.tri({6,-11},{10,-10},{6,-9},p.trim,gull.alpha);
        }else{
            cv.color(p.foam,gull.alpha);cv.moveTo(-1,-4);cv.curveTo(-5,-7,-11,-6-8*gull.flap,-17,-4-10*gull.flap);
            cv.curveTo(-10,-1-3*gull.flap,-4,0,0,-2);cv.curveTo(5,0,11,-1-3*gull.flap,17,-4-10*gull.flap);
            cv.curveTo(11,-6-8*gull.flap,5,-7,1,-4);cv.closePath();cv.fill();
            cv.line(-4,-3,4,-3,2,p.ink,.6*gull.alpha);cv.disc(5,-4,2,p.foam,gull.alpha);
        }cv.restore();}
    const double bow=nose;
    for(int j=0;j<12;++j){const double id=hash2(j,p.seed+61),x=bow+dir*(3+j*3)*sc;
        const double y=support(x)-(3+38*id)*sc*(.4+2*s.spray);
        cv.color(p.foam,s.spray);cv.ellipse(x,y,(.8+.5*id)*sc,(1.2+.9*id)*sc);cv.fill();}
}
void BoatOnWaterV1::draw(Ctx& c,const BoatOnWaterParametersV1& p) {
    GpuProfile::Group group(c.gpu.profile,name);Canvas& cv=c.canvas();paint(cv,c,p);c.gpu.over(cv);
}
}
