#include "../src/audio.h"
#include "../src/score.h"
#include "../src/schedule.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <vector>

using namespace Journey;
void require(bool ok,const char* message) { if(!ok) { std::cerr<<message<<'\n'; std::exit(1); } }
std::vector<Audio> analyze(const std::vector<float>& pcm,int chunk) {
    StreamingAudio analyzer;
    std::vector<Audio> out;
    for(std::size_t i=0;i<pcm.size()/2;) {
        const auto n=std::min<std::size_t>(chunk,pcm.size()/2-i);
        analyzer.push(pcm.data()+2*i,n,[&](const Audio& a,double){out.push_back(a);}); i+=n;
    }
    return out;
}
int main() {
    constexpr int rate=44100;
    std::vector<float> pcm(rate*2*6);
    for(int i=0;i<rate*4;++i) {
        const double t=double(i)/rate;
        const double envelope=0.02+0.12*std::pow(std::max(0.0,std::sin(t*6.4)),8);
        pcm[2*i]=envelope*std::sin(t*2*Pi*110)+0.03*std::sin(t*2*Pi*700);
        pcm[2*i+1]=-pcm[2*i];
    }
    const auto small=analyze(pcm,37),large=analyze(pcm,4096);
    require(small.size()==large.size(),"chunking changed hop count");
    for(std::size_t k=0;k<small.size();++k) for(int b=0;b<6;++b)
        require(std::abs(small[k].bands[b]-large[k].bands[b])<1e-12,"chunking changed causal bands");
    require(small[180].bands[1]>0.05,"opposite-phase stereo erased music");
    for(double b:small.back().bands) require(b<0.001,"silence did not release bands");
    std::vector<float> prefix(pcm.begin(),pcm.begin()+rate*2*2);
    const auto before=analyze(prefix,4096);
    for(std::size_t k=0;k<before.size();++k) {
        require(before[k].accent==large[k].accent,"future PCM affected prior accent");
        require(before[k].bands==large[k].bands,"future PCM affected prior bands");
    }
    Score score;
    Schedule schedule(1);
    double previousFirework=-1;
    int fireworks=0, trains=0;
    double trainStart=-1;
    // Eight simulated hours tests memory, phase bounds, causal events, cooldown
    // and recurring quiet life without an eight-hour render archive.
    for(int k=0;k<8*3600*60;++k) {
        const double t=k/60.0;
        Audio a;
        a.preGainLevel=0.112;
        if(k<3600 || k>=7200) {
            for(int b=0;b<6;++b) a.bands[b]=0.15+0.12*std::sin(t*(1.5+b*0.17));
            a.bass=0.25+0.20*std::sin(t*6.5);
            a.accent=std::pow(std::max(0.0,std::sin(t*7.8)),4)*0.85;
        }
        score.advance(a,t,1.0/60); schedule.advance(t,a,score);
        for(const auto* list:{&score.onsets,&score.bassHits,&score.midPeaks,&score.surges}) {
            require(list->size()<=256,"event history grew without bound");
            for(const auto& e:*list) require(e.t<=t,"event was scheduled from future audio");
        }
        for(double phase:score.strandPhase) require(std::isfinite(phase)&&phase>=0&&phase<5,"strand phase drifted");
        if(schedule.fireworks!=previousFirework) {
            require(schedule.fireworks>=t-0.1,"firework was backdated");
            if(previousFirework>=0) require(schedule.fireworks-previousFirework>=45,"firework cooldown failed");
            require(schedule.fireworkReady-t>=45 && schedule.fireworkReady-t<=90,"seeded cooldown outside 45-90s");
            previousFirework=schedule.fireworks; ++fireworks;
            require(k<3600 || k>=7200,"silence triggered fireworks");
        }
        const double ts=schedule.moments[int(Moment::Train)].start;
        if(ts!=trainStart) { trainStart=ts; ++trains; }
    }
    require(fireworks>100 && trains>100,"moments failed to recur");
    // Strong measured events remain required, even for loud material.
    // Gymnopedie gets one small shell and cannot add follow-ups; Sneaky and
    // Volatile keep the full authored show. Test the latched show separately.
    for(double level:{0.066,0.112,0.375}) {
        Schedule shows(1); Score events; Audio a;
        a.bands.fill(0.2); a.preGainLevel=level;
        for(int k=0;k<300;++k) {
            const double t=10+k/60.0;
            events.onsets.push_back({t,0.8,std::uint64_t(k+1)});
            shows.advance(t,a,events);
            require(shows.fullFireworkShow==(level>=0.09),"pre-gain show gate failed");
            if(level<0.09) require(shows.finale.empty(),"quiet input produced a volley");
        }
        require(shows.fireworks==10,"show restarted inside cooldown");
        if(level>=0.09) require(!shows.finale.empty(),"loud show lost follow-ups");
    }
    for(bool initialFull:{false,true}) for(bool nextFull:{false,true}) {
        Schedule shows(1); Score events; Audio a;
        a.bands.fill(0.2); a.preGainLevel=initialFull?0.375:0.066;
        events.onsets.push_back({10,0.8,1}); shows.advance(10,a,events);
        require(shows.fireworkReady>=55 && shows.fireworkReady<=100,"quiet cooldown range failed");
        if(!initialFull) require(shows.fullFireworkReady>=28 && shows.fullFireworkReady<=35,"full cooldown range failed");
        a.preGainLevel=nextFull?0.375:0.066;
        events.onsets.push_back({40,0.8,2}); shows.advance(40,a,events);
        require(shows.fireworks==(!initialFull && nextFull?40:10),"separate show cooldown failed");
        if(!initialFull && nextFull) require(shows.fullFireworkShow,"quiet then loud lost full show");
    }
    // Shared gain must not enter the independent pre-gain level measurement.
    require(small[180].preGainLevel==large[180].preGainLevel,"chunking changed pre-gain level");
    Schedule one(1), same(1), two(2); Score noEvents;
    bool different=false;
    for(int k=0;k<36000;++k) {
        const double t=k/60.0;
        one.advance(t,{},noEvents); same.advance(t,{},noEvents); two.advance(t,{},noEvents);
        for(int i=0;i<int(Moment::Count);++i) {
            require(one.moments[i].start==same.moments[i].start && one.moments[i].next==same.moments[i].next,"seed not deterministic");
            different|=one.moments[i].start!=two.moments[i].start;
            const double speed=one.parameter(Moment(i),0,0.85,1.15,1);
            require(speed>=0.85 && speed<=1.15,"moment speed outside authored range");
            if(one.moments[i].cycle<=1) require(speed==1,"opening occurrence changed");
        }
        require(one.gesture(t,9,0.4,11,0.6)==same.gesture(t,9,0.4,11,0.6),"gesture not deterministic");
        require(one.pane(t,3)==same.pane(t,3),"pane not deterministic");
    }
    require(different,"different seeds produced identical timelines");
    require(one.combinations>=1 && one.combinations<=2,"rare combinations outside expected frequency");
    require(one.gesture(100,9,0.4,11,0.6)!=one.gesture(100+hash2(9*31.7+71,0)*25+28,9,0.4,11,0.6)
        || one.pane(100,3)!=one.pane(100+hash2(3*91+71,0)*36+31,3),"fixed gesture/pane loops survived");
    Schedule silent(7); Score quiet;
    for(int k=0;k<18000;++k) { quiet.advance({},k/60.0,1.0/60); silent.advance(k/60.0,{},quiet); }
    require(silent.fireworks<0,"timed fallback fireworks survived");
    require(silent.moments[int(Moment::Train)].cycle>3,"silent scene stopped living");
    std::cout<<"PASS: prefix causality, arbitrary chunks, stereo, silence, eight-hour bounds and recurring schedules\n";
}
