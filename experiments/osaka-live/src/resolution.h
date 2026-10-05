#pragma once
#include <algorithm>
#include <cmath>

namespace Journey {
// Native pixels first: give a sustained 60 Hz overload a 30 Hz tier before
// changing scale. The 36 ms overload boundary tolerates 32-35 ms iGPU samples;
// promotion requires 29 ms at the next larger scale. Neither changes scene time.
class Resolution {
public:
    void fixed(double scale) { automaticScale_=false; scale_=scale; }
    void fixedFps(int fps) { automaticFps_=false; fps_=fps; reset(); }
    double scale() const { return scale_; }
    int fps() const { return fps_; }
    void sample(double ms,double dt) {
        if(!std::isfinite(ms) || ms<=0 || !std::isfinite(dt)) return;
        dt=std::clamp(dt,0.0,0.25);
        average_=average_==0 ? ms : average_+(ms-average_)*(1-std::exp(-dt/0.7));
        const double overload=fps_==60 ? 14 : 36;
        high_=average_>overload ? high_+dt : 0;
        const double next=scale_==0.5 ? 0.75 : 1;
        const double promotion=scale_<1 ? average_*next*next/(scale_*scale_) : average_;
        low_=promotion<(scale_<1 ? 29 : 9) ? low_+dt : 0;
        if(fps_==60 && automaticFps_ && high_>=2) {
            fps_=30;reset();
        } else if(automaticScale_ && high_>=(fps_==60 ? 2 : 4) && scale_>0.5) {
            scale_=scale_==1 ? 0.75 : 0.5;reset();
        } else if(automaticScale_ && scale_<1 && low_>=15) {
            scale_=next;reset();
        } else if(automaticFps_ && fps_==30 && scale_==1 && low_>=15) {
            fps_=60;reset();
        }
    }
private:
    void reset() { average_=high_=low_=0; }
    bool automaticScale_=true,automaticFps_=true;
    int fps_=60;
    double scale_=1,average_=0,high_=0,low_=0;
};

// Called only on presentation ticks. If interval 2 is honored, each 30 Hz tick
// renders; otherwise alternate 60 Hz ticks render. Absolute deadlines avoid
// accumulating render cost or changing animation speed.
class FramePacer {
public:
    bool tick(double now,int fps) {
        if(fps!=fps_) {fps_=fps;next_=now;}
        const double period=1.0/fps;
        if(now+period*0.2<next_) return false;
        next_+=period;
        if(next_<now-period*0.5) next_=now+period;
        return true;
    }
private:
    int fps_=0;
    double next_=0;
};
}
