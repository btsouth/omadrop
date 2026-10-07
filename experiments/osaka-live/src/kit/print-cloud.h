#pragma once
#include "../canvas.h"
#include <vector>
namespace Journey::Kit {
// One tier of a woodblock print cloud: a flat underside, rounded ends and a
// row of round scallops along the top that rise toward the middle, meeting
// in crisp cusps. Lobe widths and heights are seeded, so a cloud keeps its
// silhouette while it drifts.
struct PrintCloudTierV1 {
    std::vector<V2> outline;
    struct Lobe { V2 centre; double rx, ry; };
    std::vector<Lobe> lobes;
    double top=0, base=0;
};
inline PrintCloudTierV1 printCloudTier(double cx,double base,double width,double body,double lift,int lobes,double seed){
    PrintCloudTierV1 tier;tier.base=base;
    const double r=body*.5,x0=cx-width/2+r,x1=cx+width/2-r,shoulder=base-body;
    auto& o=tier.outline;
    // Left end: a half round from the underside up to the shoulder.
    for(int k=0;k<=8;++k){const double a=Pi*.5+Pi*k/8;o.push_back({x0+r*std::cos(a),base-r+r*std::sin(a)});}
    std::vector<double> widths(lobes);double sum=0;
    // Fuller lobes gather toward the middle of the tier.
    for(int i=0;i<lobes;++i){widths[i]=(.7+.6*hash2(seed*13+i,7))*(.75+.5*std::sin(Pi*(i+.5)/lobes));sum+=widths[i];}
    double at=x0;tier.top=shoulder;
    for(int i=0;i<lobes;++i){
        const double w=(x1-x0)*widths[i]/sum,mid=(at+w/2-x0)/(x1-x0);
        const double rise=std::pow(std::sin(Pi*mid),.8);
        const double ry=std::min(lift*(.30+.70*rise)*(.8+.4*hash2(seed*17+i,3)),.95*w/2),rx=w/2;
        // Half-round scallops meet their neighbours in crisp cusps.
        const V2 centre{at+rx,shoulder};
        for(int k=1;k<=12;++k){const double a=Pi-Pi*k/12;o.push_back({centre.x+rx*std::cos(a),centre.y-ry*std::sin(a)});}
        tier.lobes.push_back({centre,rx,ry});tier.top=std::min(tier.top,centre.y-ry);
        at+=w;
    }
    // Right end down to the underside, which stays flat.
    for(int k=0;k<=8;++k){const double a=-Pi*.5+Pi*k/8;o.push_back({x1+r*std::cos(a),base-r+r*std::sin(a)});}
    return tier;
}
inline void tracePrintCloud(Canvas& cv,const PrintCloudTierV1& t,double dx=0,double dy=0){
    cv.moveTo(t.outline[0].x+dx,t.outline[0].y+dy);
    for(std::size_t i=1;i<t.outline.size();++i)cv.lineTo(t.outline[i].x+dx,t.outline[i].y+dy);
    cv.closePath();
}
// Nested lobe lines carved inside the larger scallops, the printed cloud
// motif that gives a flat fill its form.
inline void printCloudEchoes(Canvas& cv,const PrintCloudTierV1& t,double width,Col c,double alpha,double minimum){
    if(alpha<=.002)return;
    for(const auto& l:t.lobes){if(l.ry<minimum)continue;
        const double rx=l.rx*.62,ry=l.ry*.62,y=l.centre.y+.30*l.ry;
        std::vector<V2> arc;
        for(int k=1;k<=11;++k){const double a=Pi*(.08+.84*k/12.);arc.push_back({l.centre.x-rx*std::cos(a),y-ry*std::sin(a)});}
        cv.polyline(arc,width,c,alpha);}
}
}
