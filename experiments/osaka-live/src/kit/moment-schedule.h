#pragma once
#include "../art.h"
#include "../score.h"
#include <array>
namespace Journey::Kit {
// Independent opt-in life clock. Osaka's authored schedule never reads it.
enum class PrintMoment { Fish, Cranes, FishingBoat, LanternBoat, Gust, Squall, SnowGlint, Star, Birds, Count };
struct PrintEventV1 { double start=-1000,duration=7,speed=1,height=0,direction=1; int count=0; unsigned cycle=0; };
struct WaveSetV1 { double start=0,duration=36,energy=0; unsigned cycle=0; };
struct MomentScheduleV1 {
    explicit MomentScheduleV1(int seed=1):seed(seed) { dragonNext=varied(91,0,180,300); }
    int seed=1;double next=4,last=0,heldBass=0,surgeStart=-1000,surgeReady=10,dragonNext=240;
    unsigned serial=0,surgeCycle=0,dragonCycle=0;std::uint64_t beatSerial=0;
    PrintEventV1 dragon,flock;
    double birdNext=6;
    double surgeFlow=0;
    WaveSetV1 wave,previousWave;
    std::array<PrintEventV1,int(PrintMoment::Count)> events{};
    double varied(int key,unsigned cycle,double lo,double hi)const{return lerp(lo,hi,hash2(seed*71.+key,cycle));}
    static const char* name(PrintMoment m){static constexpr const char* names[]={"fish","crane-pair","fishing-boat","lantern-boat","wind-gust","horizon-squall","snow-glint","shooting-star","bird-flock"};return names[int(m)];}
    double surge(double t)const{double age=t-surgeStart;return sstep(0,1.8,age)*(1-sstep(5,11,age));}
    void advance(double t,const Audio&a,const Score&s){
        const double dt=std::clamp(t-last,0.,.1);last=t;surgeFlow+=dt*surge(t);
        double e=0;for(double b:a.bands)e+=b*b;e=std::sqrt(e/6);
        if(a.bassLevel>=.25 && e>=.035)heldBass=std::min(2.4,heldBass+dt);else heldBass=std::max(0.,heldBass-dt*2);
        const Event* beat=Score::last(s.bassHits,t);if(!beat)beat=Score::last(s.onsets,t);
        if(beat && beat->serial>beatSerial && t-beat->t<.1){
            if(t>=surgeReady && heldBass>=1.8 && a.bassLevel>=.25 && e>=.035 && beat->strength>=.55){surgeStart=t;surgeReady=t+varied(90,++surgeCycle,45,90);}
            beatSerial=beat->serial;
        }
        // A set captures the causal low-band body while it forms, then carries
        // that size across the sea. The next set is invisible at the seam.
        if(t>=wave.start+wave.duration){
            previousWave=wave;wave.start+=wave.duration;++wave.cycle;
            wave.duration=varied(95,wave.cycle,24,38);wave.energy=0;
        }
        if(t-wave.start<.30*wave.duration)
            wave.energy=clamp01(2.4*(.65*s.bandBody[2][0]+.35*s.bandBody[2][1]));
        if(t>=next){
            // Seeded permutation per nine-event round guarantees every type
            // recurs, with a fresh cyclic rotation and stride each round.
            unsigned round=serial/9,index=serial%9;
            const int stride=varied(70,round,0,1)<.5?2:4;
            const int offset=int(varied(71,round,0,9));
            const int selected=(offset+stride*index)%9;
            auto&v=events[selected];v.start=t;++v.cycle;
            v.speed=v.cycle==1?1:varied(30+int(index),serial,.75,1.3);
            v.duration=8/v.speed;v.direction=varied(40,serial,0,1)<.5?-1:1;
            v.height=varied(41,serial,0,1);v.count=3+int(varied(42,serial,0,6));
            if(selected==int(PrintMoment::Birds) && t-flock.start>=flock.duration)flock=v;
            next=t+varied(10,++serial,5,10);
        }
        if(t>=birdNext && t-flock.start<flock.duration)birdNext=flock.start+flock.duration+1;
        if(t>=birdNext){flock.start=t;++flock.cycle;flock.duration=varied(80,flock.cycle,9,14);flock.direction=varied(81,flock.cycle,0,1)<.5?-1:1;
            flock.count=3+int(varied(82,flock.cycle,0,6));flock.height=varied(83,flock.cycle,0,1);birdNext=t+varied(84,flock.cycle,17,30);}
        // Music can bring a pending visit forward by up to four seconds,
        // without shortening the several-minute recurrence or popping in.
        if(t>=dragonNext-(a.bassLevel>.35?4:0)){
            dragon.start=t;dragon.duration=18;dragon.speed=varied(92,dragonCycle,.8,1.2);dragon.direction=varied(93,dragonCycle,0,1)<.5?-1:1;
            dragon.duration=18/dragon.speed;dragon.cycle=++dragonCycle;dragonNext=t+varied(94,dragonCycle,240,360);
        }
    }
};
}
