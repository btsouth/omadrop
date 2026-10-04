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
            if(previousFirework>=0) require(schedule.fireworks-previousFirework>=18,"firework cooldown failed");
            previousFirework=schedule.fireworks; ++fireworks;
            require(k<3600 || k>=7200,"silence triggered fireworks");
        }
        const double ts=schedule.moments[int(Moment::Train)].start;
        if(ts!=trainStart) { trainStart=ts; ++trains; }
    }
    require(fireworks>100 && trains>100,"moments failed to recur");
    Schedule silent(7); Score quiet;
    for(int k=0;k<18000;++k) { quiet.advance({},k/60.0,1.0/60); silent.advance(k/60.0,{},quiet); }
    require(silent.fireworks<0,"timed fallback fireworks survived");
    require(silent.moments[int(Moment::Train)].cycle>3,"silent scene stopped living");
    std::cout<<"PASS: prefix causality, arbitrary chunks, stereo, silence, eight-hour bounds and recurring schedules\n";
}
