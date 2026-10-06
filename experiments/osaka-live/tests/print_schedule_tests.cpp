#include "kit/moment-schedule.h"
#include <iostream>
#include <stdexcept>
using namespace Journey;using namespace Journey::Kit;
void need(bool b,const char*s){if(!b)throw std::runtime_error(s);}
int main(){try{
 MomentScheduleV1 quiet(1),a(1),b(1),other(9);Audio loud;loud.bands.fill(.45);loud.bassLevel=.5;Score score;double prior=0,lastSurge=-1000;unsigned seen=0;
 for(int i=1;i<=36000;++i){double t=i/60.;score.bassHits={{std::floor(t*2)/2,.9,(uint64_t)(t*2)+1}};
 a.advance(t,loud,score);b.advance(t,loud,score);quiet.advance(t,{},{});other.advance(t,{},{});
 need(a.serial==b.serial && a.surgeStart==b.surgeStart && a.dragon.start==b.dragon.start,"seed nondeterminism");
 need(quiet.surgeCycle==0,"silence starts surge");
 if(a.serial!=seen){if(seen)need(t-prior>=5 && t-prior<=10.02,"event gap out of bounds");seen=a.serial;prior=t;}
 if(a.surgeStart!=lastSurge){if(lastSurge>0)need(a.surgeStart-lastSurge>=45 && a.surgeStart-lastSurge<=91,"surge cooldown");lastSurge=a.surgeStart;need(a.heldBass>=1.8,"unsustained gate");}
 need(a.surge(t)>=0 && a.surge(t)<=1,"surge envelope bound");}
 for(const auto&v:a.events)need(v.cycle>=5,"moment omitted from recurring rounds");
 need(a.surgeCycle>=5 && a.dragonCycle>=1,"missing recurring surge or dragon");
 need(a.events[0].count!=other.events[0].count || a.next!=other.next,"seed has no variety");
 MomentScheduleV1 transient;for(int i=1;i<60;++i)transient.advance(i/60.,loud,score);need(transient.surgeCycle==0,"single transient triggers surge");
 MomentScheduleV1 reset;reset.surgeReady=1000;
 for(int i=1;i<600;++i)reset.advance(i/60.,loud,score);
 for(int i=600;i<900;++i)reset.advance(i/60.,{},{});
 need(reset.heldBass==0,"sustained gate remembers old loud passage");reset.surgeReady=0;
 Score hit;hit.bassHits={{15,.9,999}};reset.advance(15,loud,hit);need(reset.surgeCycle==0,"new transient inherits old bass gate");
 // The shared row clock locks catches to a steady kick, half time when fast,
 // and never rewinds the crews while it pulls into phase.
 for(double period:{.667,.5}){MomentScheduleV1 r(3);Score beats;double previous=0,worst=0,nextBeat=1.03;std::uint64_t id=0;
  for(int i=1;i<=60*40;++i){const double t=i/60.;bool onBeat=false;if(t>=nextBeat){beats.bassHits.push_back({t,.8,++id});nextBeat+=period;onBeat=true;}
   r.advance(t,loud,beats);need(r.rowPhase>=previous-1e-9,"rowers rewind");previous=r.rowPhase;
   const double grid=period<.6?.5:1;
   if(onBeat && t>20)worst=std::max(worst,std::abs(r.rowPhase/grid-std::round(r.rowPhase/grid))*grid);}
  need(worst<.1,"catches drift off the beat");}
 // Music cues moments without restarting one already on screen.
 {MomentScheduleV1 m(2);Score s;m.last=29.99;s.bassHits={{30,.9,1}};m.advance(30,loud,s);
  need(m.events[int(PrintMoment::Fish)].start==30,"strong kick does not cue fish");
  s.bassHits.push_back({32,.9,2});m.last=31.99;m.advance(32,loud,s);need(m.events[int(PrintMoment::Fish)].start==30,"fish cue restarts a visible leap");
  s.surges={{33,.6,3}};m.last=32.99;m.advance(33,loud,s);need(m.active(PrintMoment::Cranes,33),"measured rise does not cue cranes");}
 std::cout<<"PASS beat-locked row clock, music cues, seeded ten-minute bounds, sustained bass gate, 45-90s cooldown, quiet suppression and rare creature\n";
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
