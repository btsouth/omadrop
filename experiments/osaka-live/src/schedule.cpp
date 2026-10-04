#include "schedule.h"
#include <algorithm>
#include <cmath>

namespace Journey {
namespace {
constexpr std::array<double,7> durations{9,7,15,5,8,1.2,0.9};
constexpr std::array<double,7> gaps{30,28,34,15,32,12,34};
}
double Schedule::varied(double key,std::uint64_t cycle,double lo,double hi) const {
    return lo+(hi-lo)*hash2(key+seed_*71.0,double(cycle));
}
Schedule::Schedule(int seed):seed_(seed) {
    for(int i=0;i<int(Moment::Count);++i) {
        auto& m=moments[i]; m.duration=durations[i];
        m.start=-1000; m.next=varied(i,0,2,14);
    }
    for(int i=0;i<15;++i) {
        birdLand[i]=1+0.8*i+varied(i+50,0,0,1);
        birdReturn[i]=-1;
    }
}
void Schedule::advance(double t,const Audio& a,const Score& score) {
    double energy=0;
    for(double b:a.bands) energy+=b*b;
    energy=std::sqrt(energy/6);
    const Event* onset=Score::last(score.onsets,t);
    const Event* bass=Score::last(score.bassHits,t);
    const Event* surge=Score::last(score.surges,t);
    const Event* trigger=nullptr;
    for(const Event* e:{onset,bass,surge}) {
        if(e && e->serial>eventSerial_ && t-e->t<0.1) {
            if(!trigger || e->strength>trigger->strength) trigger=e;
        }
    }
    const bool strong=trigger && trigger->strength>=0.55 && energy>=0.035;
    if(strong && t>=fireworkReady) {
        fireworks=t; fireworkStrength=trigger->strength;
        // Fixture RMS means: Gymnopedie .066, Sneaky .112, Volatile .375.
        // .09 separates quiet music from a full show; latch for the volley.
        fullFireworkShow=a.preGainLevel>=0.09;
        fireworkReady=t+varied(90,++fireworksCount_,45,90);
        finale.clear();
        // Birds have time to scatter and leave before returning independently.
        for(int i=0;i<15;++i) birdReturn[i]=t+varied(100+i,fireworksCount_,13,24);
    } else if(fullFireworkShow && strong && fireworks>=0 && t>fireworks+1.3 && t<fireworks+4.8
        && finale.size()<4 && (finale.empty() || t-finale.back().t>0.5)) {
        finale.push_back({t,trigger->strength,trigger->serial});
    }
    if(trigger) eventSerial_=std::max(eventSerial_,trigger->serial);
    for(int i=0;i<int(Moment::Count);++i) {
        auto& m=moments[i];
        if(t<m.next) continue;
        // Toasts/stars may follow an onset once their own quiet schedule opens.
        // Silence still gets these small existing actions, never fireworks.
        if((i==int(Moment::Toast)||i==int(Moment::Star)) && energy>0.035
            && !trigger && t<m.next+3) continue;
        m.start=t;
        m.next=t+gaps[i]+varied(10+i,++m.cycle,0,gaps[i]*0.65);
    }
    for(int i=0;i<15;++i) if(birdReturn[i]>=0 && t>=birdReturn[i]) {
        // Start the approach now, not in the past of a newly detected beat.
        birdLand[i]=t+1.15; birdReturn[i]=-1;
    }
}
double Schedule::gesture(double now,double a,double in,double b,double out) const {
    // Independent seeded periods for small authored gestures. No whole-scene
    // rewind, and every gesture returns to its resting pose before recurrence.
    const double period=varied(a*31.7,0,28,53);
    const double phase=std::fmod(now+varied(a*17.0,0,0,period),period);
    return window(phase,0,in,b-a,out);
}
double Schedule::pane(double now,double key) const {
    const double period=varied(key*91.0,0,31,67);
    const double age=std::fmod(now+period-key,period);
    // Most of each window's life is lit; a soft settling interval and the
    // original flicker-on can recur without resetting the entire street.
    if(age<0.6) return age<0.05?0.55:age<0.11?0.08:age<0.17?0.8:age<0.22?0.35:0.75+0.25*sstep(0.22,0.6,age);
    return 1-0.8*window(age,period-4,1.2,period,0.7);
}
}
