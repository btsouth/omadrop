#include "swell-lines.h"
#include <cmath>
#include <QPainterPathStroker>
namespace Journey::Kit {
double SwellRowV1::y(double x) const {
    const double a=x/scale-phase;
    // Printed sea's long curl plus its second harmonic, with calm-sea texture.
    return base+amplitude*(std::sin(a)+.23*std::sin(a*2.1+.6)
        +.075*std::sin(x/(scale*.43)+detail))
        -lift*(.65+.35*std::sin(a*.47+detail*.31));
}
int SwellLinesV1::band(int row,int rows) {
    return std::clamp(int(std::round((1-row/double(rows-1))*5)),0,5);
}
SwellRowV1 SwellLinesV1::field(const Ctx& c,int row,const SwellLinesParametersV1& p) {
    const double z=row/double(p.rows-1),depth=std::pow(z,p.depthFalloff);
    const int b=band(row,p.rows);
    const double e=c.score ? .20*c.score->bandBody[0][b]+.50*c.score->bandBody[1][b]+.30*c.score->bandBody[2][b] : c.band(b);
    const double role=(1-z)*5; const int lo=std::min(4,int(role));
    const double flow=c.score ? .95*lerp(c.score->bandIntegrals[lo],c.score->bandIntegrals[lo+1],role-lo) : 0;
    const double id=hash2(row,p.seed+13),lift=c.lift(b),kick=c.kick(5);
    return {z,p.region.top()+2+(p.region.height()-4)*depth,
        (7+83*std::pow(depth,1.15))*p.amplitude*(1+.22*e+.16*p.liftGain*lift),
        (55+115*depth)*(.86+.28*id),
        c.t*p.driftSpeed*(.81+.37*id)+flow-depth*6+Tau*hash2(row,p.seed+19),
        row*.82-c.t*(.17+.11*hash2(row,p.seed+23)),
        std::min(.85,p.opacity+p.bandGain*e+p.liftGain*.35*lift+p.kickGain*kick),
        (8+8*depth)*p.bandGain*c.band(b)+(10+10*depth)*p.liftGain*lift+(2+8*depth)*p.kickGain*kick,
        clamp01(p.bandGain*c.band(b)+p.liftGain*lift)};
}
QRectF SwellLinesV1::responseArea(int row,const SwellLinesParametersV1& p) {
    const double z=row/double(p.rows-1),depth=std::pow(z,p.depthFalloff);
    const double y=p.region.top()+2+(p.region.height()-4)*depth;
    const double spread=(7+83*std::pow(depth,1.15))*p.amplitude*1.8+40;
    return QRectF(p.region.left(),std::max(p.region.top(),y-spread),p.region.width(),2*spread).intersected(p.region);
}
QPainterPath SwellLinesV1::responsePath(const Ctx& c,int row,const SwellLinesParametersV1& p,double width) {
    const auto f=field(c,row,p);QPainterPath edge;
    edge.moveTo(p.region.left(),f.y(p.region.left()));
    for(double x=p.region.left()+16;x<p.region.right();x+=16)edge.lineTo(x,f.y(x));
    edge.lineTo(p.region.right(),f.y(p.region.right()));
    QPainterPathStroker stroke;stroke.setWidth(width);QPainterPath area=stroke.createStroke(edge);
    QPainterPath clip;clip.addRect(p.region);area=area.intersected(clip);
    for(const auto& box:p.exclusions){QPainterPath excluded;excluded.addRect(box);area=area.subtracted(excluded);}
    return area;
}
bool SwellLinesV1::allowed(const QRectF& bounds,const SwellLinesParametersV1& p) {
    if(!p.region.contains(bounds))return false;
    for(const auto& box:p.exclusions)if(box.intersects(bounds))return false;
    return true;
}
void SwellLinesV1::paint(Canvas& cv,const Ctx& c,const SwellLinesParametersV1& p) {
    std::vector<SwellRowV1> fields;
    for(int row=0;row<p.rows;++row)fields.push_back(field(c,row,p));
    std::vector<V2> points;points.reserve(64);
    for(int i=0;i<p.count;++i) {
        const int row=i%p.rows;const auto& f=fields[row];
        const double id=hash2(i,p.seed+31),depth=std::pow(f.z,p.depthFalloff);
        const double span=lerp(p.lengthMin,p.lengthMax,id)*(.38+.82*depth);
        // Offscreen wraps and individually seeded speeds have no short common
        // cycle; even silent sea keeps moving. Audio flow carries the contour.
        const double margin=p.lengthMax*1.4,period=p.region.width()+2*margin;
        const double travel=c.t*p.driftSpeed*(7+12*hash2(i,p.seed+32));
        const double x0=p.region.left()-margin+std::fmod(period*hash2(i,p.seed+33)+travel,period);
        const double offset=(hash2(i,p.seed+34)-.48)*(12+37*depth);
        const double surge=p.kickGain*c.kick(5)*(2+8*depth);
        const double width=lerp(p.widthMin,p.widthMax,depth)*(.65+.65*hash2(i,p.seed+35));
        points.clear();
        auto flush=[&]() {if(points.size()>1)cv.polyline(points,width,mix(p.color,p.highlight,f.highlight),f.brightness*(.35+.65*id));points.clear();};
        const int n=std::max(5,int(std::ceil(span/20)));
        for(int j=0;j<=n;++j) {
            const double u=j/double(n),x=x0+u*span;
            const double y=f.y(x)+offset+2*depth*std::sin(Pi*u)-surge;
            const QRectF bounds(x-width,y-width,2*width,2*width);
            if(!allowed(bounds,p)){flush();continue;}
            // Do not bridge a reserved area when sampling crosses its corner.
            if(!points.empty()) {
                const auto& last=points.back();
                const QRectF segment(QPointF(std::min(last.x,x)-width,std::min(last.y,y)-width),QPointF(std::max(last.x,x)+width,std::max(last.y,y)+width));
                if(!allowed(segment,p))flush();
            }
            points.push_back({x,y});
        }
        flush();
    }
}
void SwellLinesV1::draw(Ctx& c,const SwellLinesParametersV1& p) {
    GpuProfile::Group group(c.gpu.profile,name);
    Canvas& cv=c.canvas();paint(cv,c,p);c.gpu.over(cv);
}
}
