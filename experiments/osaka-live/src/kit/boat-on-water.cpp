#include "boat-on-water.h"
#include "../rig.h"
#include <QTransform>
namespace Journey::Kit {
double BoatOnWaterV1::surfaceY(const Ctx& c,const BoatOnWaterParametersV1& p,double x,double row) {
    row=std::clamp(row,0.,double(p.swell.rows-1));
    const int lo=std::min(p.swell.rows-2,int(row));
    const double sea=lerp(SwellLinesV1::field(c,lo,p.swell).y(x),SwellLinesV1::field(c,lo+1,p.swell).y(x),row-lo);
    const double depth=1-sstep(0,2,std::max(0.,p.wave.row-row));
    return p.ridesWave?lerp(sea,GreatWaveV1::field(c,p.wave).y(x,sea),depth):sea;
}
BoatOnWaterPoseV1 BoatOnWaterV1::pose(const Ctx& c,const BoatOnWaterParametersV1& p) {
    const double phase=Tau*hash2(p.seed,41),clock=c.t*p.driftSpeed;
    // Continuous bounded paths with incommensurate rates, no wrap or waypoint
    // seam. Seed changes both the lane and its slow rhythm over many minutes.
    auto drift=[&](double t){return .65*std::sin(t+phase)+.35*std::sin(t*std::sqrt(2.)+phase*.73);};
    BoatOnWaterPoseV1 s;s.at.x=p.x+p.driftX*(drift(clock)-drift(0))*.5;
    s.row=p.row+p.driftRows*(std::sin(clock*std::sqrt(3.)+phase)-std::sin(phase))*.5;
    const double half=p.length*p.scale*.45;
    const auto wave= p.ridesWave?GreatWaveV1::field(c,p.wave):GreatWaveFieldV1{};
    const int lo=std::min(p.swell.rows-2,int(std::clamp(s.row,0.,double(p.swell.rows-1))));
    const auto low=SwellLinesV1::field(c,lo,p.swell),high=SwellLinesV1::field(c,lo+1,p.swell);
    const double depth=1-sstep(0,2,std::max(0.,p.wave.row-s.row));
    auto support=[&](double x){const double sea=lerp(low.y(x),high.y(x),s.row-lo);return p.ridesWave?lerp(sea,wave.y(x,sea),depth):sea;};
    const double left=support(s.at.x-half),right=support(s.at.x+half);
    s.waterline=(left+2*support(s.at.x)+right)*.25;
    s.tilt=std::atan2(right-left,2*half);
    if(p.ridesWave)s.tilt=.55*std::tanh(s.tilt/.55);
    s.at.y=s.waterline-14*p.scale*std::cos(s.tilt);
    if(p.ridesWave){
        // The near hull sits entirely above the shared face. Fit the rigid hull
        // to its most demanding support point, not only its average centre.
        for(int j=0;j<=16;++j){double local=p.length*p.scale*(j/16.-.5)*1.18;
            const double x=s.at.x+local*std::cos(s.tilt)-26*p.scale*std::sin(s.tilt);
            s.at.y=std::min(s.at.y,support(x)-local*std::sin(s.tilt)-26*p.scale*std::cos(s.tilt));}
    }
    s.brace=clamp01(.9*c.kick(6)+.45*c.hit(6)+(p.surgeEnabled && c.schedule?.65*c.schedule->print.surge(c.t):0));
    // Integral makes tempo change continuously; never multiply time by the
    // current band/kick (that would jump the pose on every beat).
    s.stroke=c.t*p.rowingTempo+p.tempoGain*(c.score?c.score->bandIntegrals[p.band]:0)+hash2(p.seed,43);
    if(p.surgeEnabled && c.schedule)s.stroke+=.6*c.schedule->print.surgeFlow;
    s.splash=std::clamp(.10+p.splashGain*(.55*c.band(p.band)+.45*c.kick(5)),0.,.55);
    s.spray=std::clamp(p.kickGain*c.kick(6)+(p.surgeEnabled && c.schedule?.45*c.schedule->print.surge(c.t):0),0.,.8);
    return s;
}
QPainterPath BoatOnWaterV1::responsePath(const BoatOnWaterPoseV1& s,const BoatOnWaterParametersV1& p) {
    QPainterPath area;const double ln=p.length*p.scale,sc=p.scale;
    area.addRect(QRectF(-ln/2-35*sc,-60*sc,ln+100*sc,145*sc));
    QTransform transform;transform.translate(s.at.x,s.at.y);transform.rotateRadians(s.tilt);
    return transform.map(area);
}
void BoatOnWaterV1::paint(Canvas& cv,const Ctx& c,const BoatOnWaterParametersV1& p) {
    const auto s=pose(c,p);const double sc=p.scale,ln=p.length*sc;
    const auto wave=p.ridesWave?GreatWaveV1::field(c,p.wave):GreatWaveFieldV1{};
    const int lo=std::min(p.swell.rows-2,int(s.row));
    const auto low=SwellLinesV1::field(c,lo,p.swell),high=SwellLinesV1::field(c,lo+1,p.swell);
    const double depth=1-sstep(0,2,std::max(0.,p.wave.row-s.row));
    auto support=[&](double x){double sea=lerp(low.y(x),high.y(x),s.row-lo);return p.ridesWave?lerp(sea,wave.y(x,sea),depth):sea;};
    const double ca=std::cos(s.tilt),sa=std::sin(s.tilt);
    auto wp=[&](V2 q){return s.at+V2(q.x*ca-q.y*sa,q.x*sa+q.y*ca);};
    // Surface marks beneath the hull; the wake uses the actual contour too.
    for(int j=0;j<3;++j){std::vector<V2> points;
        for(int k=0;k<10;++k){const double x=s.at.x-ln*.52-(12+j*9)*sc+k*ln*.04;
            points.push_back({x,support(x)+(2+j*3)*sc});}
        cv.polyline(points,1.1*sc,p.foam,.17*(1-j*.22));}
    cv.save();cv.translate(s.at.x,s.at.y);cv.rotate(s.tilt);
    // Journey drawBoat: shallow raised ends, ochre side and distinct gunwale.
    cv.color(p.ink);cv.moveTo(-ln/2-26*sc,-16*sc);
    cv.curveTo(-ln/4,14*sc,ln/4,14*sc,ln/2+12*sc,-8*sc);
    cv.lineTo(ln/2-4*sc,10*sc);cv.curveTo(ln/4,26*sc,-ln/4,26*sc,-ln/2,6*sc);cv.closePath();cv.fill();
    cv.color(p.hull);cv.moveTo(-ln/2-20*sc,-13*sc);
    cv.curveTo(-ln/4,12*sc,ln/4,12*sc,ln/2+6*sc,-6*sc);
    cv.lineTo(ln/2-6*sc,6*sc);cv.curveTo(ln/4,20*sc,-ln/4,20*sc,-ln/2,3*sc);cv.closePath();cv.fill();
    std::vector<std::array<V2,2>> oars;
    for(int i=0;i<p.crewCount;++i){
        const double f=(i+.5)/p.crewCount,bx=-ln/2+ln*(.12+.72*f),by=2*sc+8*sc*std::sin(f*Pi);
        const double phase=s.stroke-i*.025,drive=std::sin(Tau*phase),dip=std::max(0.,std::sin(Tau*phase+.6));
        RigIn r;r.h=62*sc;r.facing=1;r.hair=Hair::Short;r.garment=Garment::Jacket;
        r.hip={bx,by-(13-5*s.brace)*sc};r.lean=.18+.30*drive+.32*s.brace;
        r.footF={bx+21*sc,by+3*sc};r.footB={bx+7*sc,by+5*sc};
        const V2 wanted(bx+(17+10*drive)*sc,by-(16-2*drive)*sc);
        const V2 shoulder=r.hip+V2(std::sin(r.lean),-std::cos(r.lean))*(.275*r.h),reach=wanted-shoulder;
        const V2 grip=shoulder+reach*std::min(1.,.29*r.h/std::max(1e-6,reach.len()));
        r.handF=grip;r.handB=grip+V2(-5*sc,sc);r.headTilt=.10*drive-.16*s.brace;
        const auto body=solve(r);drawBody(cv,body,p.ink);
        cv.line(body.shoulder.x,body.shoulder.y+2*sc,body.chest.x,body.chest.y+10*sc,4*sc,p.trim,.55);
        if(i<p.oarCount){
            V2 blade=grip+V2((23+18*drive+10*s.brace*std::sin(c.t*9+i))*sc,(34+18*dip)*sc);
            const V2 world=wp(blade);
            // In-water drive touches the sampled swell; recovery lifts the
            // blade a few pixels. No independent sine bob or detached rings.
            const double water=support(world.x)-6*sc*(1-dip);
            blade.y=(water-s.at.y-blade.x*sa)/ca;
            oars.push_back({grip,blade});
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
    for(std::size_t i=0;i<oars.size();++i){
        const V2 water=wp(oars[i][1]);const double phase=s.stroke-i*.025;
        const double ring=phase-std::floor(phase),dip=std::max(0.,std::sin(Tau*phase+.6));
        cv.color(p.foam,s.splash*(1-ring)*sstep(.05,.35,dip));
        cv.ellipse(water.x,support(water.x),(7+24*ring)*sc,(1.5+4*ring)*sc);cv.stroke(1.1*sc);
    }
    const double bow=s.at.x+ln*.48*ca;
    for(int j=0;j<12;++j){const double id=hash2(j,p.seed+61),x=bow+(3+j*3)*sc;
        const double y=support(x)-(3+38*id)*sc*(.4+2*s.spray);
        cv.color(p.foam,s.spray);cv.ellipse(x,y,(.8+.5*id)*sc,(1.2+.9*id)*sc);cv.fill();}
}
void BoatOnWaterV1::draw(Ctx& c,const BoatOnWaterParametersV1& p) {
    GpuProfile::Group group(c.gpu.profile,name);Canvas& cv=c.canvas();paint(cv,c,p);c.gpu.over(cv);
}
}
