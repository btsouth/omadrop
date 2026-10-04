#pragma once
#include "gpu.h"
#include "score.h"
#include "schedule.h"
#include <QString>
#include <array>

namespace Journey {
struct Ctx {
    Gpu& gpu;
    double t;
    Audio a;
    const Score* score;
    int seed;
    std::array<Canvas,6>* canvases;
    const Schedule* schedule;
    int next=0;
    double cameraX=0;
    Canvas& canvas() { auto& c=(*canvases)[next++%canvases->size()]; c.reset(gpu.pixelScale()); c.translate(-cameraX,0); return c; }
    double band(int i) const { return a.bands[std::clamp(i,0,5)]; }
    double lift(int i) const {
        const double m=score?score->mean(std::clamp(i,0,5),t):0;
        return std::clamp((band(i)-m)/(m+0.035),0.0,1.6);
    }
    double kick(double d=6) const { return score?Score::envelope(score->bassHits,t,d):0; }
    double hit(double d=8) const { return score?Score::envelope(score->onsets,t,d):0; }
    double jit(double k) const { return hash2(k,seed*7.13+0.5)*2-1; }
    double gesture(double a,double in,double b,double out) const { return schedule->gesture(t,a,in,b,out); }
};
struct OsakaState {
    double cam=5, drift=0, land=1, fog=0, fogTop=600;
    double moonDx=0, moonDy=0, moonWarm=0,wisteria=1;
    bool chapter=true;
    double outAlpha=1,harbour=1,reflection=1,harbourFeather=0;
};
struct Surge { double t=-1,strength=0; bool fallback=false; };
void drawOsaka(Ctx&,const OsakaState&);
void drawSignGlyph(Canvas&,int,double,double,double,Col,double alpha=1);
class World {
public:
    explicit World(int seed):seed_(seed) {}
    bool init(QString& error) { return gpu_.init(error); }
    void render(int width,int height,double time,const Audio&,const Score&,const Schedule&);
    Gpu& gpu() { return gpu_; }
private:
    Gpu gpu_;
    int seed_;
    std::array<Canvas,6> canvases_;
};
}
