#include "kit/moment-schedule.h"
#include <iostream>
#include <vector>
#include <cmath>
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
 // The great wave crashes rarely with a calm after each landing; every set
 // kind and boat outcome recurs and no crew is swamped twice running.
 {MomentScheduleV1 m(1);Score s;for(auto& band:s.bandBody)band.fill(.4);double last=-1000,calm=0;std::vector<double> crashes;
  for(int i=1;i<=60*600;++i){double t=i/60.;s.bassHits={{std::floor(t*2)/2,.9,(uint64_t)(t*2)+1}};s.onsets=s.bassHits;
   m.advance(t,loud,s);
   if(m.crashStart!=last){last=m.crashStart;crashes.push_back(t);calm=m.calmUntil;need(calm-t>=8 && calm-t<=20,"calm after a landing out of bounds");}
   if(t<calm && t>last+.1)need(m.setCharge==0,"set charges during the calm");}
  need(crashes.size()>=8 && crashes.size()<=16,"crash count over ten minutes");
  for(size_t i=1;i<crashes.size();++i)need(crashes[i]-crashes[i-1]>=24,"crashes closer than 24 s");}
 {MomentScheduleV1 m(1);BoatFate previous=BoatFate::Escape;int kinds[4]{},fates[3]{};
  for(unsigned c=0,f=0;c<60;++c){const auto p=m.choosePlan(c,f,previous);++kinds[int(p.kind)];
   if(p.kind!=SetKind::Fizzle){++fates[int(p.near)];++f;}
   need(!(p.near==BoatFate::Swamped && previous==BoatFate::Swamped),"crew swamped twice running");
   need(p.kind!=SetKind::Towering || p.spot<=-30,"towering set hides the mountain");previous=p.near;}
  for(int k:kinds)need(k>=6,"set kind missing");for(int f:fates)need(f>=5,"boat outcome missing");}
 std::cout<<"PASS beat-locked row clock, music cues, seeded ten-minute bounds, sustained bass gate, 45-90s cooldown, quiet suppression, rare creature and varied great-wave sets\n";
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
