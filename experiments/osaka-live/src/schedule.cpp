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
    combinationAt=varied(200,0,240,360);
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
    if(strong && t>=(a.preGainLevel>=0.09 ? fullFireworkReady : fireworkReady)) {
        fireworks=t; fireworkStrength=trigger->strength;
        // Fixture RMS means: Gymnopedie .066, Sneaky .112, Volatile .375.
        // .09 separates quiet music from a full show; latch for the volley.
        fullFireworkShow=a.preGainLevel>=0.09;
        fireworkReady=t+varied(90,++fireworksCount_,45,90);
        fullFireworkReady=fullFireworkShow ? fireworkReady : t+varied(91,fireworksCount_,18,25);
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
    if(t>=combinationAt) {
        // Keep each participant's full exit before the next pass. These are
        // existing actions only; music still independently gates fireworks.
        const int partner=varied(201,combinations,0,1)<0.5 ? int(Moment::Gust) : int(Moment::Cyclist);
        const auto idle=[&](int i) { return t-moments[i].start>durations[i]/0.85+4; };
        if(idle(int(Moment::Train)) && idle(partner)) {
            for(int i:{int(Moment::Train),partner}) {
                auto& m=moments[i]; m.start=t; ++m.cycle;
                m.next=t+gaps[i]+varied(10+i,m.cycle,0,gaps[i]*0.65);
            }
            combinationAt=t+varied(200,++combinations,240,360);
        }
    }
    for(int i=0;i<15;++i) if(birdReturn[i]>=0 && t>=birdReturn[i]) {
        // Start the approach now, not in the past of a newly detected beat.
        birdLand[i]=t+1.15; birdReturn[i]=-1;
    }
}
double Schedule::parameter(Moment m,double key,double lo,double hi,double original) const {
    const auto cycle=moments[int(m)].cycle;
    return cycle<=1 ? original : varied(300+int(m)*19+key,cycle,lo,hi);
}
Schedule::Cycle Schedule::cycleAt(double age,double key,double lo,double hi) const {
    // Periods come in pairs that sum to lo+hi, so the current cycle is found
    // in constant time however long the session has been running.
    const double pair=lo+hi, n=std::floor(age/pair);
    age-=n*pair;
    const auto index=std::uint64_t(n)*2+1;
    const double period=varied(key,index,lo,hi);
    if(age<period) return {index,age,period};
    return {index+1,age-period,pair-period};
}
double Schedule::cyclePhase(double now,double key,double phaseKey,double lo,double hi,bool isPane) const {
    const double first=varied(key,0,lo,hi);
    const double offset=isPane ? first-phaseKey : varied(phaseKey,0,0,first);
    // Retain the complete original opening, including its initial phase.
    const double opening=2*first-offset;
    if(now<opening) return std::fmod(now+offset,first);
    const Cycle cycle=cycleAt(now-opening,key,lo,hi);
    // Gestures begin from rest, after a seeded idle interval, and finish
    // before the next boundary. Panes restart their flicker.
    const double delay=isPane ? 0 : varied(phaseKey,cycle.index,0,cycle.period-12);
    return cycle.age-delay;
}
double Schedule::gesture(double now,double a,double in,double b,double out) const {
    return window(cyclePhase(now,a*31.7,a*17.0,28,53,false),0,in,b-a,out);
}
double Schedule::pane(double now,double key) const {
    const double first=varied(key*91.0,0,31,67);
    const double opening=first+key;
    const double period=now<opening ? first : cycleAt(now-opening,key*91.0,31,67).period;
    const double age=cyclePhase(now,key*91.0,key,31,67,true);
    if(age<0.6) return age<0.05?0.55:age<0.11?0.08:age<0.17?0.8:age<0.22?0.35:0.75+0.25*sstep(0.22,0.6,age);
    return 1-0.8*window(age,period-4,1.2,period,0.7);
}
}
