#pragma once
#include "../world.h"
#include <QPainterPath>
namespace Journey::Kit {
// Perspective and contour arithmetic ported from Journey's printed/calm sea.
// The same field is available to foam and future hull support.
struct SwellLinesParametersV1 {
    QRectF region{-30,612,2030,500};
    std::vector<QRectF> exclusions;
    int count=260, rows=12, seed=71;
    double depthFalloff=1.45, widthMin=.55, widthMax=2.0;
    double lengthMin=110, lengthMax=650, driftSpeed=.52, amplitude=1;
    double opacity=.34, bandGain=.32, liftGain=.25, kickGain=.10;
    Col color=hex(0x7397a4), highlight=hex(0xdcd7ba);
};
struct SwellRowV1 {
    double z, base, amplitude, scale, phase, detail, brightness, lift, highlight;
    double y(double x) const;
};
struct SwellLinesV1 {
    static constexpr const char* name="swell-lines-v1";
    static int band(int row,int rows);
    static SwellRowV1 field(const Ctx&,int row,const SwellLinesParametersV1&);
    static QRectF responseArea(int row,const SwellLinesParametersV1&);
    static QPainterPath responsePath(const Ctx&,int row,const SwellLinesParametersV1&,double width);
    static bool allowed(const QRectF&,const SwellLinesParametersV1&);
    static void paint(Canvas&,const Ctx&,const SwellLinesParametersV1&);
    static void draw(Ctx&,const SwellLinesParametersV1&);
};
}
