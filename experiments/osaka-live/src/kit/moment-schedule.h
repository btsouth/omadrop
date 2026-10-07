#pragma once
#include "../art.h"
#include "../score.h"
#include <array>
#include <algorithm>
namespace Journey::Kit {
// Independent opt-in life clock. Osaka's authored schedule never reads it.
enum class PrintMoment { Fish, Cranes, FishingBoat, LanternBoat, Gust, Squall, SnowGlint, Star, Birds, Count };
struct PrintEventV1 { double start=-1000,duration=7,speed=1,height=0,direction=1; int count=0; unsigned cycle=0; };
struct WaveSetV1 { double start=0,duration=36,energy=0; unsigned cycle=0; };
struct MomentScheduleV1 {
    explicit MomentScheduleV1(int seed=1):seed(seed) { dragonNext=varied(91,0,40,80); }
    int seed=1;double next=4,last=0,heldBass=0,surgeStart=-1000,surgeReady=10,dragonNext=240;
    unsigned serial=0,surgeCycle=0,dragonCycle=0;std::uint64_t beatSerial=0;
    PrintEventV1 dragon,flock,foregroundSwell,gull;
    double swellNext=12,gullNext=8,gullTakeoff=-1000;
    double birdNext=6;
    double surgeFlow=0;
    // Shared rowing clock. Catches fall on whole cycles and a detected beat
    // pulls the phase toward the nearest catch (a half stroke on fast songs)
    // by easing its rate, so crews never rewind or jump between poses.
    double rowPhase=0,rowRate=.32,rowPull=0,rowTempo=90,rowBeat=-1000;
    std::uint64_t rowSerial=0,cueSerial=0;
    double fishCue=-1000,craneCue=-1000,boatCue=-1000;
    WaveSetV1 wave,previousWave;
    std::array<PrintEventV1,int(PrintMoment::Count)> events{};
    double varied(int key,unsigned cycle,double lo,double hi)const{return lerp(lo,hi,hash2(seed*71.+key,cycle));}
    static const char* name(PrintMoment m){static constexpr const char* names[]={"fish","crane-pair","fishing-boat","lantern-boat","wind-gust","horizon-squall","snow-glint","shooting-star","bird-flock"};return names[int(m)];}
    double surge(double t)const{double age=t-surgeStart;return sstep(0,1.8,age)*(1-sstep(5,11,age));}
    double eventLength(int m,double speed)const{
        return (m==int(PrintMoment::Squall)?12:m==int(PrintMoment::Cranes)?14:
            m==int(PrintMoment::FishingBoat) || m==int(PrintMoment::LanternBoat)?20:8)/speed;}
    bool active(PrintMoment m,double t)const{const auto&v=events[int(m)];return t>=v.start && t<v.start+v.duration;}
    // Starts a moment from the music when it is not already on screen.
    void cue(PrintMoment m,double t,unsigned key){if(active(m,t))return;auto&v=events[int(m)];
        v.start=t;++v.cycle;v.speed=varied(60+int(m),v.cycle+key,.85,1.2);v.duration=eventLength(int(m),v.speed);
        v.direction=varied(61+int(m),v.cycle,0,1)<.5?-1:1;v.height=varied(62+int(m),v.cycle,0,1);v.count=3+int(varied(63+int(m),v.cycle,0,6));}
    void row(double t,double dt,const Score&s){
        const bool kicks=s.bassHits.size()>=3 && s.bassHits.back().t>t-4;
        const auto& source=kicks?s.bassHits:s.onsets;
        std::array<double,32> intervals{};int n=0;double previous=-1;
        for(const auto& e:source)if(e.t<=t && e.t>t-8 && (kicks || e.strength>=.35)){
            if(previous>=0 && e.t-previous>.25 && n<32)intervals[n++]=e.t-previous;previous=e.t;}
        if(n>=3){std::sort(intervals.begin(),intervals.begin()+n);double tempo=60/intervals[n/2];
            while(tempo<60)tempo*=2;while(tempo>180)tempo*=.5;rowTempo+=(tempo-rowTempo)*-std::expm1(-dt/1.5);}
        const Event* beat=Score::last(source,t);
        const bool playing=beat && t-beat->t<3;
        // One stroke per beat; half time once the beat is too quick to pull.
        // A towering surge makes every beat a stroke again.
        const double perBeat=rowTempo>112 && surge(t)<.4?.5:1;
        const double target=playing?perBeat*rowTempo/60:.30;
        rowRate+=(target-rowRate)*-std::expm1(-dt/.5);
        if(beat && beat->serial>rowSerial && t-beat->t<.1 && (kicks || beat->strength>=.35)){
            rowSerial=beat->serial;rowBeat=beat->t;
            const double grid=perBeat<1?.5:1,ahead=rowPhase+rowRate*.04;
            rowPull=std::clamp(std::round(ahead/grid)*grid-rowPhase,-.22,.45);
        }
        double pull=rowPull*-std::expm1(-dt/.08);pull=std::max(pull,-.6*rowRate*dt);
        rowPhase+=rowRate*dt+pull;rowPull-=pull;
    }
    // One set at a time ties the sea together. Loud playing charges it; a
    // strong hit (or a surge) launches a swell at the horizon that rolls
    // toward the viewer and breaks the great wave when it reaches its row.
    static constexpr double setTravel=4.2,setWaveRow=.65,crashLength=5.5;
    double setCharge=0,setFull=-1000,setLaunch=-1000,setStrength=0,crashStart=-1000,crashStrength=0;
    unsigned setCycle=0;
    double setArrival()const{return setLaunch+setTravel*setWaveRow;}
    bool setRolling()const{return setLaunch>crashStart;}
    // The set passing depth z (0 horizon, 1 nearest row): a smooth rise and fall.
    double setRoll(double z,double t)const{const double age=t-setLaunch-setTravel*z;
        return age<=0||age>4?0:setStrength*(age/.5)*std::exp(1-age/.5);}
    double crash(double t)const{const double age=t-crashStart;
        return age<0?0:crashStrength*sstep(0,.3,age)*(1-sstep(1.2,crashLength,age));}
    // Crash foam spreads from the wave's row to the rows around it, then fades.
    double crashFoam(double z,double t)const{const double age=t-crashStart;if(age<0||age>8)return 0;
        const double reach=.05+.4*sstep(0,3,age);
        return crashStrength*(1-sstep(.5*reach,reach,std::abs(z-setWaveRow)))*sstep(0,.4,age)*(1-sstep(3,8,age));}
    void set(double t,double dt,const Score&s){
        const double loud=clamp01(2.4*(.65*s.bandBody[2][0]+.35*s.bandBody[2][1]));
        double body=0;for(double b:s.bandBody[1])body+=b*b;body=clamp01(3.1*std::sqrt(body/6));
        if(setRolling() && t>=setArrival()){crashStart=t;crashStrength=setStrength;setCharge=0;setFull=-1000;}
        if(setRolling())return;
        // Only playing music charges the next set; a quiet passage holds it.
        setCharge=std::min(1.,setCharge+dt*(.5*sstep(.15,.6,loud)+.5*sstep(.35,.8,body))/12);
        if(setCharge>=1 && setFull<0)setFull=t;
        // A set only leaves while the music is playing fully.
        if(t-crashStart<12 || body<.55)return;
        const Event* hit=Score::last(s.bassHits,t);const Event* onset=Score::last(s.onsets,t);
        const bool kick=hit && t-hit->t<.1 && hit->strength>=.4;
        const bool surging=t-surgeStart<.1 && setCharge>=.45;
        const bool late=setFull>0 && t-setFull>6 && onset && t-onset->t<.1;
        if((setCharge>=1 && kick)||surging||late){
            setLaunch=t;++setCycle;setStrength=std::clamp(.7+.4*loud+(surging||surge(t)>.3?.3:0),.6,1.3);}
    }
    void advance(double t,const Audio&a,const Score&s){
        const double dt=std::clamp(t-last,0.,.1);last=t;surgeFlow+=dt*surge(t);
        row(t,dt,s);
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
            auto&v=events[selected];
            // A moment the music already cued keeps its crossing.
            if(!active(PrintMoment(selected),t)){v.start=t;++v.cycle;
            v.speed=v.cycle==1?1:varied(30+int(index),serial,.75,1.3);
            v.duration=eventLength(selected,v.speed);v.direction=varied(40,serial,0,1)<.5?-1:1;
            v.height=varied(41,serial,0,1);v.count=3+int(varied(42,serial,0,6));
            if(selected==int(PrintMoment::Birds) && t-flock.start>=flock.duration)flock=v;}
            next=t+varied(10,++serial,5,10);
        }
        if(t>=gullNext){gull.start=t;gull.duration=26;++gull.cycle;gullTakeoff=-1000;
            gullNext=t+varied(99,gull.cycle,36,52);}
        if(gullTakeoff<gull.start && t>=gull.start+9){
            if(beat && t-beat->t<.1 && beat->strength>=.6 && a.bassLevel>.25)gullTakeoff=t;
            else if(t>=gull.start+22)gullTakeoff=t;
        }
        // A surge brings the foreground swell forward when it is not rolling.
        if(t-surgeStart<.1 && t>=foregroundSwell.start+foregroundSwell.duration)swellNext=t;
        if(t>=swellNext){foregroundSwell.start=t;foregroundSwell.duration=13;foregroundSwell.cycle++;
            foregroundSwell.direction=varied(97,foregroundSwell.cycle,0,1)<.5?-1:1;
            swellNext=t+varied(98,foregroundSwell.cycle,16,24);}
        if(t>=birdNext && t-flock.start<flock.duration)birdNext=flock.start+flock.duration+1;
        if(t>=birdNext){flock.start=t;++flock.cycle;flock.duration=varied(80,flock.cycle,9,14);flock.direction=varied(81,flock.cycle,0,1)<.5?-1:1;
            flock.count=3+int(varied(82,flock.cycle,0,6));flock.height=varied(83,flock.cycle,0,1);birdNext=t+varied(84,flock.cycle,17,30);}
        // Strong kicks throw fish beside the near boat, a measured rise sends
        // cranes over and a held loud passage brings a working boat across.
        // Each cue has its own cooldown and never restarts a visible moment.
        {const Event* kick=Score::last(s.bassHits,t);
         if(kick && kick->serial>cueSerial && t-kick->t<.1){cueSerial=kick->serial;
            if(kick->strength>=.62 && a.bassLevel>=.22 && t-fishCue>=7){cue(PrintMoment::Fish,t,kick->serial&255);fishCue=t;}}
         const Event* rise=Score::last(s.surges,t);
         if((rise && t-rise->t<.1 && t-craneCue>=20) || (t-surgeStart<.1 && t-craneCue>=20)){cue(PrintMoment::Cranes,t,serial);cue(PrintMoment::Gust,t,serial);craneCue=t;}
         if(heldBass>=2.2 && t-boatCue>=30){cue(boatCue<0?PrintMoment::LanternBoat:(int(t)%2?PrintMoment::FishingBoat:PrintMoment::LanternBoat),t,serial);boatCue=t;}}
        // Music can bring a pending visit forward by up to fifteen seconds,
        // without shortening the minute-scale recurrence or popping in.
        if(t>=dragonNext-(a.bassLevel>.35?15:0)){
            dragon.start=t;dragon.duration=18;dragon.speed=varied(92,dragonCycle,.8,1.2);dragon.direction=varied(93,dragonCycle,0,1)<.5?-1:1;
            dragon.duration=18/dragon.speed;dragon.cycle=++dragonCycle;dragonNext=t+varied(94,dragonCycle,70,120);
        }
        set(t,dt,s);
    }
};
}
