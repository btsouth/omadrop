#pragma once
#include "../art.h"
#include "../score.h"
#include <array>
#include <algorithm>
namespace Journey::Kit {
// Independent opt-in life clock. Osaka's authored schedule never reads it.
enum class PrintMoment { Fish, Cranes, FishingBoat, LanternBoat, Gust, Squall, SnowGlint, Star, Birds, Count };
struct PrintEventV1 { double start=-1000,duration=7,speed=1,height=0,direction=1; int count=0; unsigned cycle=0; };
// Each set has its own character: most build and break, some tower with
// two feints, some fizzle back into the sea and some break early and small.
enum class SetKind { Normal, Towering, Fizzle, EarlyBreak };
// What the set does to the near boat: it outruns it, is swamped, or rides
// the landing and is pitched hard but survives.
enum class BoatFate { Escape, Swamped, Rides };
struct SetPlanV1 {
    SetKind kind=SetKind::Normal;BoatFate near=BoatFate::Escape;
    double scale=.95,spot=-40,build=1,breakAt=2,fizzleAt=2,travel=1,splash=1;
    std::array<double,2> feintAt{{2,2}}; // build levels of the lip's feints
    int nearDirection=1;bool nearOffscreen=true,farStays=false;
};
// A boat lane's race against the set. frac runs from home (0) to the lane's
// goal (an edge, or the landing for a caught lane); facing turns the hull.
struct RaceLaneV1 {
    enum State { Home, Fleeing, Caught, Returning, Entering } state=Home;
    BoatFate fate=BoatFate::Escape;
    double frac=0,velocity=0,facing=1,since=-1000,landing=1100;
    int direction=1;bool offscreen=true;
};
struct WaveSetV1 { double start=0,duration=36,energy=0; unsigned cycle=0; };
struct MomentScheduleV1 {
    explicit MomentScheduleV1(int seed=1):seed(seed) { dragonNext=varied(91,0,40,80);plan=choosePlan(0,0,BoatFate::Escape); }
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
    // After a landing the sea stays calm for a while before the next set can
    // build, so crashes stay rare and the lulls have room for life.
    static constexpr double setTravel=4.2,setWaveRow=.65,crashLength=5.5,setBuild=26,crashGap=24;
    double riseLevel=0,setCharge=0,setFull=-1000,setLaunch=-1000,setStrength=0,crashStart=-1000,crashStrength=0;
    double rollTravel=setTravel,calmUntil=0,fizzleStart=-1000,fizzleEnd=-1000,dropFor=0,calmCue=-1000,landingX=1100;
    unsigned setCycle=0,planCycle=0,fateCycle=0,calmSerial=0;
    SetPlanV1 plan,lastPlan;
    std::array<RaceLaneV1,2> lanes{}; // near, far
    double travel()const{return rollTravel;}
    double setArrival()const{return setLaunch+travel()*setWaveRow;}
    bool setRolling()const{return setLaunch>crashStart;}
    bool fizzling()const{return fizzleStart>fizzleEnd;}
    // The set passing depth z (0 horizon, 1 nearest row): a smooth rise and fall.
    double setRoll(double z,double t)const{const double age=t-setLaunch-travel()*z;
        return age<=0||age>4?0:setStrength*(age/.5)*std::exp(1-age/.5);}
    double crash(double t)const{const double age=t-crashStart;
        return age<0?0:crashStrength*sstep(0,.3,age)*(1-sstep(1.2,crashLength,age));}
    // Crash foam spreads from the wave's row to the rows around it, then fades.
    double crashFoam(double z,double t)const{const double age=t-crashStart;if(age<0||age>8)return 0;
        const double reach=.05+.4*sstep(0,3,age);
        return crashStrength*(1-sstep(.5*reach,reach,std::abs(z-setWaveRow)))*sstep(0,.4,age)*(1-sstep(3,8,age));}
    // Each set gets its own character from a seeded bag, so every kind and
    // every boat outcome recurs and no two neighbours repeat the same story.
    SetPlanV1 choosePlan(unsigned cycle,unsigned fateCycle,BoatFate previous)const{
        static constexpr SetKind kinds[]={SetKind::Normal,SetKind::Towering,SetKind::Normal,SetKind::Fizzle,SetKind::EarlyBreak,SetKind::Normal};
        static constexpr BoatFate fates[]={BoatFate::Escape,BoatFate::Swamped,BoatFate::Escape,BoatFate::Rides,BoatFate::Escape};
        auto pick=[&](int key,unsigned n,unsigned index){const unsigned round=index/n,at=index%n;
            const unsigned stride=n==6?(varied(key,round,0,1)<.5?1:5):(varied(key,round,0,1)<.5?2:3);
            return (unsigned(varied(key+1,round,0,n))+stride*at)%n;};
        SetPlanV1 p;
        // The opening set is a full ordinary one.
        p.kind=cycle==0?SetKind::Normal:kinds[pick(300,6,cycle-1)];
        p.near=fates[pick(302,5,fateCycle)];
        const double r=varied(304,cycle,0,1);
        if(p.kind==SetKind::Towering && p.near==BoatFate::Escape && r<.4)p.near=BoatFate::Swamped;
        if(p.kind==SetKind::EarlyBreak && p.near==BoatFate::Swamped)p.near=BoatFate::Rides;
        if(p.near==BoatFate::Swamped && previous==BoatFate::Swamped)p.near=r<.5?BoatFate::Escape:BoatFate::Rides;
        if(p.kind==SetKind::Fizzle)p.near=BoatFate::Escape;
        p.nearDirection=varied(305,cycle,0,1)<.5?-1:1;p.nearOffscreen=varied(306,cycle,0,1)<.6;
        // Tall sets stay left of the mountain so Fuji and the sun stay in view.
        p.spot=varied(307,cycle,-220,60);
        switch(p.kind){
        case SetKind::Normal:p.scale=varied(308,cycle,.88,1.02);
            if(varied(309,cycle,0,1)<.65)p.feintAt[0]=varied(310,cycle,.55,.85);break;
        case SetKind::Towering:p.scale=varied(308,cycle,1.04,1.1);p.build=1.15;p.splash=1.2;p.spot=std::min(p.spot,-30.);
            p.feintAt={{varied(310,cycle,.48,.6),varied(311,cycle,.74,.88)}};break;
        case SetKind::Fizzle:p.scale=varied(308,cycle,.86,1);p.fizzleAt=varied(312,cycle,.72,.9);
            if(varied(309,cycle,0,1)<.5)p.feintAt[0]=varied(310,cycle,.5,.65);break;
        case SetKind::EarlyBreak:p.scale=varied(308,cycle,.82,.9);p.breakAt=varied(313,cycle,.42,.58);p.travel=.65;p.splash=.65;break;
        }
        p.farStays=p.kind==SetKind::EarlyBreak || p.kind==SetKind::Fizzle || (p.kind==SetKind::Normal && p.scale<.93);
        return p;
    }
    // A fizzle never reaches the boats, so it does not use up an outcome.
    void nextPlan(){lastPlan=plan;if(plan.kind!=SetKind::Fizzle)++fateCycle;plan=choosePlan(++planCycle,fateCycle,lastPlan.near);}
    void set(double t,double dt,const Score&s){
        const double loud=clamp01(2.4*(.65*s.bandBody[2][0]+.35*s.bandBody[2][1]));
        double body=0;for(double b:s.bandBody[1])body+=b*b;body=clamp01(3.1*std::sqrt(body/6));
        if(setRolling() && t>=setArrival()){crashStart=t;crashStrength=setStrength;setCharge=0;setFull=-1000;
            calmUntil=t+varied(320,setCycle,8,20);calmCue=t+varied(321,setCycle,3.5,6.5);nextPlan();}
        if(setRolling())return;
        riseLevel+=(loud-riseLevel)*-std::expm1(-dt/4);
        dropFor=loud<.12?dropFor+dt:0;
        // A fizzling set gives up its crest and sinks back into the sea; the
        // next one may start soon after.
        if(fizzling()){setCharge=std::max(0.,setCharge-dt/7);
            if(setCharge<=0){fizzleEnd=t;calmUntil=t+varied(322,planCycle,3,6);calmCue=t+2;nextPlan();}
            return;}
        if(t<calmUntil)return;
        // The next set builds while the music plays, about twenty-six seconds
        // of full playing; quiet passages hold it and silence lets it ebb.
        // Rising music drives it hardest: a swell in level counts double.
        const double drive=(.5*sstep(.15,.6,loud)+.5*sstep(.35,.8,body))*(1+1.2*clamp01(4*(loud-riseLevel)));
        setCharge=std::clamp(setCharge+dt*(drive/(setBuild*plan.build)-(drive<.08?1./150:0)),0.,1.);
        if(setCharge>=1 && setFull<0)setFull=t;
        if(setCharge<.97)setFull=-1000;
        // The music turns a set: a surge makes an ordinary one tower, and a
        // real drop in the music lets a grown one fizzle out.
        if(plan.kind==SetKind::Normal && lastPlan.kind!=SetKind::Towering && t-surgeStart<.1 && setCharge>.3 && varied(314,planCycle,0,1)<.35){plan.kind=SetKind::Towering;plan.scale=std::max(plan.scale,1.04);plan.splash=1.2;plan.spot=std::min(plan.spot,-30.);
            if(plan.feintAt[1]>1)plan.feintAt[1]=std::max(plan.feintAt[0]>1?.6:plan.feintAt[0]+.15,setCharge+.12);}
        if((plan.kind==SetKind::Fizzle && setCharge>=plan.fizzleAt)||(dropFor>3.5 && setCharge>.5)){fizzleStart=t;plan.kind=SetKind::Fizzle;return;}
        // A built set leaves on a strong hit while the music plays fully, or
        // on the next onset once it has stood ready for six seconds. An early
        // set breaks on the first strong hit once it is half grown.
        if(t-crashStart<crashGap)return;
        const Event* hit=Score::last(s.bassHits,t);const Event* onset=Score::last(s.onsets,t);
        const bool kick=hit && t-hit->t<.1 && hit->strength>=.4;
        const bool surging=t-surgeStart<.1 && setCharge>=.9;
        const bool late=setFull>0 && t-setFull>6 && onset && t-onset->t<.1;
        const bool early=plan.kind==SetKind::EarlyBreak && setCharge>=plan.breakAt && kick && body>=.35;
        if((setCharge>=1 && kick && body>=.55)||surging||late||early){
            setLaunch=t;++setCycle;rollTravel=setTravel*plan.travel;setStrength=std::min(1.4,std::clamp(.7+.4*loud+(surging||surge(t)>.3?.3:0),.6,1.3)*plan.splash);}
    }
    // Boat lanes race the set. A fleeing lane runs for an edge as the set
    // builds and rows back once it has landed or fizzled; a caught lane is
    // dragged toward the landing. Lanes move on a damped spring with a speed
    // limit and turn the hull before rowing the other way.
    double raceBuild(double t)const{return std::max(.55*easeIn(sstep(.35,1.,setCharge))+.45*(setRolling()?sstep(setLaunch,setArrival(),t):0.),0.);}
    void race(double t,double dt){
        const double roll=setRolling()?sstep(setLaunch,setArrival(),t):0.,age=t-crashStart;
        for(int i=0;i<2;++i){auto& L=lanes[i];double target=0,limit=.2,facing=1;
            const bool landed=crashStart>L.since,released=landed || fizzleEnd>L.since || (fizzling() && fizzleStart>L.since && setCharge<.45);
            switch(L.state){
            case RaceLaneV1::Home:
                if(!fizzling() && t>=calmUntil && setCharge>.3 && !(i==1 && plan.farStays)){
                    L.since=t;L.fate=i==0?plan.near:BoatFate::Escape;L.direction=i==0?plan.nearDirection:1;
                    L.offscreen=i==0?plan.nearOffscreen:true;L.state=L.fate==BoatFate::Escape?RaceLaneV1::Fleeing:RaceLaneV1::Caught;
                    if(L.state==RaceLaneV1::Caught)L.landing=landingX;}
                break;
            case RaceLaneV1::Fleeing:
                target=std::max(raceBuild(t),.9*roll);facing=L.direction;
                if(landed){target=std::max(L.frac,target);if(age>=1){L.state=RaceLaneV1::Returning;L.since=t;}}
                else if(released){L.state=RaceLaneV1::Returning;L.since=t;}
                break;
            case RaceLaneV1::Caught:
                target=.6*easeIn(sstep(.35,1.,setCharge))+.4*roll;
                if(!landed)L.landing=landingX;
                if(landed){target=1.25;limit=.35;
                    if(L.fate==BoatFate::Swamped && age>=3){L.state=RaceLaneV1::Entering;L.since=crashStart+3;L.frac=0;L.velocity=0;}
                    else if(L.fate!=BoatFate::Swamped && age>=3.5){L.state=RaceLaneV1::Returning;L.since=t;}}
                else if(released){L.state=RaceLaneV1::Returning;L.since=t;}
                break;
            case RaceLaneV1::Returning:
                // Turn first, then row home.
                facing=L.fate==BoatFate::Escape?-L.direction:-1;limit=.075;
                target=t-L.since<1.2?L.frac:0;
                if(L.frac<.004 && std::abs(L.velocity)<.01){L.state=RaceLaneV1::Home;L.frac=0;L.velocity=0;}
                break;
            case RaceLaneV1::Entering:
                if(t-L.since>=13)L.state=RaceLaneV1::Home;
                break;
            }
            if(L.state!=RaceLaneV1::Entering){
                L.velocity+=dt*(1.6*1.6*(target-L.frac)-2*1.6*L.velocity);
                L.velocity=std::clamp(L.velocity,-limit,limit);L.frac+=L.velocity*dt;}
            L.facing+=std::clamp(facing-L.facing,-dt/.8,dt/.8);
        }
    }
    // Life returns to the sea in the calm after a landing or a fizzle.
    void calm(double t){
        if(calmCue<0 || t<calmCue)return;calmCue=-1000;
        const unsigned n=calmSerial++;
        static constexpr int order[]={0,1,2,3,4,5};
        const int start=int(varied(330,n/6,0,6));
        for(int k=0;k<6;++k){const int m=order[(start+n+k)%6];
            if(m==0 && !active(PrintMoment::Cranes,t)){cue(PrintMoment::Cranes,t,n);cue(PrintMoment::Gust,t,n);craneCue=t;return;}
            if(m==1 && !active(PrintMoment::LanternBoat,t) && !active(PrintMoment::FishingBoat,t)){cue(PrintMoment::LanternBoat,t,n);boatCue=t;return;}
            if(m==2 && lanes[0].state!=RaceLaneV1::Entering && !active(PrintMoment::Fish,t)){cue(PrintMoment::Fish,t,n);fishCue=t;return;}
            if(m==3 && t-flock.start>=flock.duration){birdNext=t;return;}
            if(m==4 && !active(PrintMoment::FishingBoat,t) && !active(PrintMoment::LanternBoat,t)){cue(PrintMoment::FishingBoat,t,n);boatCue=t;return;}
            if(m==5 && t-gull.start>gull.duration){gullNext=t;return;}
        }
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
        set(t,dt,s);race(t,dt);calm(t);
    }
};
}
