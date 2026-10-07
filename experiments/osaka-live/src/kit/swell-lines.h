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
    // Scales the band-integral share of drift; with a lower driftSpeed the
    // sea runs with the music instead of a constant clock.
    double flowGain=1;
    // Share of the gap to the neighbouring row a crest may travel. Above
    // about .5 a nearer swell rises over the farther one and hides it.
    double rowFreedom=.42;
    // Each bass hit lifts the sea as a swell that rolls from the horizon to
    // the viewer over rollDelay seconds. Zero keeps the uniform kick heave.
    double rollGain=0,rollDelay=.55;
    bool surgeEnabled=false;
    bool orderedRows=true; // printed contours; background WaterSurface retains its authored field
    double amplitudeGain=0;
    double opacity=.34, bandGain=.32, liftGain=.25, kickGain=.10;
    Col color=hex(0x7397a4), highlight=hex(0xdcd7ba);
};
struct SwellRowV1 {
    double z, base, amplitude, scale, phase, detail, brightness, lift, highlight;
    double displacementLimit=0;
    double roll=0; // the rolling bass swells and the set passing this row, 0..2.7
    double foam=0; // foam a breaking set spreads across this row, 0..1.3
    double set=0; // the set swell passing this row, 0..1.3
    double ringX=0,ringR=-1,ringAmp=0; // the great wave's landing kicks a raised ring outward
    double y(double x,double offset=0) const;
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
