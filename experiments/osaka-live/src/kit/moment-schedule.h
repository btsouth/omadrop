#pragma once
#include "../art.h"
#include "../score.h"
#include <array>
namespace Journey::Kit {
// Independent opt-in life clock. Osaka's authored schedule never reads it.
enum class PrintMoment { Fish, Cranes, FishingBoat, LanternBoat, Gust, Squall, SnowGlint, Star, Birds, Count };
struct PrintEventV1 { double start=-1000,duration=7,speed=1,height=0,direction=1; int count=0; unsigned cycle=0; };
struct MomentScheduleV1 {
    explicit MomentScheduleV1(int seed=1):seed(seed) { dragonNext=varied(91,0,180,300); }
    int seed=1;double next=4,last=0,heldBass=0,surgeStart=-1000,surgeReady=10,dragonNext=240;
    unsigned serial=0,surgeCycle=0,dragonCycle=0;std::uint64_t beatSerial=0;
    PrintEventV1 dragon;
    std::array<PrintEventV1,int(PrintMoment::Count)> events{};
    double varied(int key,unsigned cycle,double lo,double hi)const{return lerp(lo,hi,hash2(seed*71.+key,cycle));}
    static const char* name(PrintMoment m){static constexpr const char* names[]={"fish","crane-pair","fishing-boat","lantern-boat","wind-gust","horizon-squall","snow-glint","shooting-star","bird-flock"};return names[int(m)];}
    double surge(double t)const{double age=t-surgeStart;return sstep(0,1.8,age)*(1-sstep(5,11,age));}
    void advance(double t,const Audio&a,const Score&s){
        const double dt=std::clamp(t-last,0.,.1);last=t;
        double e=0;for(double b:a.bands)e+=b*b;e=std::sqrt(e/6);
        if(a.bassLevel>=.25 && e>=.035)heldBass+=dt;else heldBass=std::max(0.,heldBass-dt*2);
        const Event* beat=Score::last(s.bassHits,t);if(!beat)beat=Score::last(s.onsets,t);
        if(beat && beat->serial>beatSerial && t-beat->t<.1){
            if(t>=surgeReady && heldBass>=1.8 && beat->strength>=.55){surgeStart=t;surgeReady=t+varied(90,++surgeCycle,45,90);}
            beatSerial=beat->serial;
        }
        if(t>=next){
            // Seeded permutation per nine-event round guarantees every type
            // recurs, with a fresh cyclic rotation and stride each round.
            unsigned round=serial/9,index=serial%9;
            const int stride=varied(70,round,0,1)<.5?2:4;
            const int offset=int(varied(71,round,0,9));
            auto&v=events[(offset+stride*index)%9];v.start=t;++v.cycle;
            v.speed=v.cycle==1?1:varied(30+int(index),serial,.75,1.3);
            v.duration=8/v.speed;v.direction=varied(40,serial,0,1)<.5?-1:1;
            v.height=varied(41,serial,0,1);v.count=3+int(varied(42,serial,0,6));
            next=t+varied(10,++serial,5,10);
        }
        // Music can bring a pending visit forward by up to four seconds,
        // without shortening the several-minute recurrence or popping in.
        if(t>=dragonNext-(a.bassLevel>.35?4:0)){
            dragon.start=t;dragon.duration=18;dragon.speed=varied(92,dragonCycle,.8,1.2);dragon.direction=varied(93,dragonCycle,0,1)<.5?-1:1;
            dragon.cycle=++dragonCycle;dragonNext=t+varied(94,dragonCycle,240,360);
        }
    }
};
}
