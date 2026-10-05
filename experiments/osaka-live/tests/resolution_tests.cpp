#include "resolution.h"
#include <iostream>
#include <limits>
#include <cstdlib>
using Journey::Resolution;
static void require(bool value,const char* message) {if(!value) {std::cerr<<message<<'\n';std::exit(1);}}
static void samples(Resolution& r,double nativeMs,int seconds) {
    for(int i=0;i<seconds*60;++i)r.sample(nativeMs*r.scale()*r.scale(),1.0/60);
}
int main() {
    Resolution fast;samples(fast,8,120);require(fast.scale()==1 && fast.fps()==60,"RTX must remain native 60");
    Resolution intel;
    for(int i=0;i<120*60;++i)intel.sample(32+i%4,1.0/60);
    require(intel.scale()==1 && intel.fps()==30,"32-35 ms iGPU must remain native 30");
    Resolution slow;samples(slow,70,120);require(slow.scale()==0.5 && slow.fps()==30,"70 ms GPU must step down after native 30");
    for(int i=0;i<120*60;++i) {slow.sample(70*slow.scale()*slow.scale(),1.0/60);require(slow.scale()==0.5,"slow GPU scale ping-pong");}
    samples(slow,8,60);require(slow.scale()==1 && slow.fps()==60,"fast recovery must promote scale then fps");
    Resolution border;samples(border,13.5,5);
    for(int i=0;i<120*60;++i)border.sample(i%2 ? 13.5 : 14.5,1.0/60);
    require(border.scale()==1 && border.fps()==60,"60 Hz boundary noise must not ping-pong");
    Resolution fixed;fixed.fixed(0.75);samples(fixed,100,120);require(fixed.scale()==0.75,"scale override changed");
    fixed.fixedFps(60);samples(fixed,100,120);require(fixed.scale()==0.75 && fixed.fps()==60,"fps override changed");
    Resolution forced;forced.fixedFps(30);samples(forced,8,60);require(forced.scale()==1 && forced.fps()==30,"forced 30 must stay native 30");
    forced.sample(std::numeric_limits<double>::quiet_NaN(),1);require(forced.scale()==1 && forced.fps()==30,"invalid sample changed tier");
    Journey::FramePacer paced;int frames=0;
    for(int i=0;i<600;++i)frames+=paced.tick(i/60.0,30);
    require(frames==300,"fallback must render every second 60 Hz tick");
    Journey::FramePacer honored;frames=0;
    for(int i=0;i<300;++i)frames+=honored.tick(i/30.0,30);
    require(frames==300,"honored interval 2 must not halve the rate again");
    std::cout<<"PASS RTX native60, iGPU native30, slow scale fallback, recovery, hysteresis, overrides, alternate-vsync pacing\n";
}
