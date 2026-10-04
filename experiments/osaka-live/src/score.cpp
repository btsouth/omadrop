#include "score.h"
#include <algorithm>
#include <cmath>

namespace Journey {
void Score::advance(const Audio& a, double t, double dt) {
    if (!std::isfinite(t) || !std::isfinite(dt) || dt <= 0) return;
    dt = std::min(dt, 0.1);
    for (int i=0; i<6; ++i) {
        means[i] += (a.bands[i]-means[i]) * -std::expm1(-dt/1.5);
        const double speed = 0.05 + 0.035*i;
        // Five cycles also repeat the quay's original 1.4 multiplier.
        strandPhase[i] = std::fmod(strandPhase[i] + speed*(0.55+3.4*a.bands[i])*dt, 5.0);
    }
    bassMean_ += (a.bass-bassMean_)*-std::expm1(-dt/2.1);
    midMean_ += (a.bands[3]-midMean_)*-std::expm1(-dt/0.83);
    auto append=[&](auto& list, double strength) { list.push_back({t,std::clamp(strength,0.0,1.0),++serial_}); };
    double trough=previous_.accent, oldBass=previous_.bass;
    for (const auto& s : recent_) {
        trough=std::min(trough,s.a.accent);
        if (s.t <= t-0.05) oldBass=s.a.bass;
    }
    // Confirm a peak with the sample just received, stamp discovery time.
    // This costs one hop, never reads or backdates an event into the future.
    if (t>1 && previous_.accent>a.accent && previous_.accent-trough>0.045 && t-lastOnset_>0.16) {
        append(onsets,(previous_.accent-trough)*7.0); lastOnset_=t;
    }
    if (t>1 && a.bass-oldBass>0.07 && a.bass>bassMean_*1.12 && a.bass>0.25 && t-lastBass_>0.3) {
        append(bassHits,a.bass); lastBass_=t;
    }
    if (t>1 && previous_.bands[3]>a.bands[3] && previous_.bands[3]>midMean_*1.08
        && previous_.bands[3]>0.07 && t-lastMid_>0.22) {
        append(midPeaks,previous_.bands[3]); lastMid_=t;
    }
    if (a.surge<0.12) surgeArmed_=true;
    if (surgeArmed_ && a.surge>0.25) { append(surges,a.surge); surgeArmed_=false; }
    recent_.push_back({t,a});
    while (!recent_.empty() && recent_.front().t<t-0.2) recent_.pop_front();
    for (auto* list : {&onsets,&bassHits,&midPeaks,&surges}) {
        while (!list->empty() && list->front().t<t-20) list->pop_front();
        while (list->size()>256) list->pop_front();
    }
    previous_=a;
}
const Event* Score::last(const std::deque<Event>& list, double t) {
    for (auto it=list.rbegin(); it!=list.rend(); ++it) if (it->t<=t) return &*it;
    return nullptr;
}
double Score::envelope(const std::deque<Event>& list,double t,double decay,double maxAge) {
    const Event* e=last(list,t);
    return e && t-e->t<=maxAge ? e->strength*std::exp(-(t-e->t)*decay) : 0;
}
}
