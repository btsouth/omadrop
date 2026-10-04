#pragma once
#include <algorithm>
#include <cmath>

namespace Journey {
// Time-based hysteresis, independent of presentation cadence. Promotion uses
// the estimated cost at full size, so a fast reduced target cannot ping-pong.
class Resolution {
public:
    void fixed(double scale) { automatic_=false; scale_=scale; }
    double scale() const { return scale_; }
    void sample(double ms,double dt) {
        if(!automatic_ || !std::isfinite(ms) || ms<=0) return;
        dt=std::clamp(dt,0.0,0.25);
        average_=average_==0 ? ms : average_+(ms-average_)*(1-std::exp(-dt/0.7));
        high_=average_>14 ? high_+dt : 0;
        low_=average_/(scale_*scale_)<9 ? low_+dt : 0;
        if(high_>=2 && scale_>0.5) change(scale_==1 ? 0.75 : 0.5);
        else if(low_>=15 && scale_<1) change(scale_==0.5 ? 0.75 : 1);
    }
private:
    void change(double value) { scale_=value; average_=high_=low_=0; }
    bool automatic_=true;
    double scale_=1,average_=0,high_=0,low_=0;
};
}
