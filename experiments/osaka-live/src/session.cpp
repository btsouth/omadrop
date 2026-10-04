#include "session.h"
#include <array>
#include <cstdio>
#include <cstdlib>
#include <deque>
#include <algorithm>

namespace Journey {
LiveSession::LiveSession(int seed):start_(std::chrono::steady_clock::now()) {
    frame_.schedule=Schedule(seed);
    worker_=std::thread([this] { run(); });
}
LiveSession::~LiveSession() { stop_=true; if(worker_.joinable()) worker_.join(); }
LiveFrame LiveSession::snapshot() {
    std::lock_guard<std::mutex> lock(mutex_);
    LiveFrame f=frame_;
    f.seconds=std::chrono::duration<double>(std::chrono::steady_clock::now()-start_).count();
    return f;
}
void LiveSession::run() {
    PipeWireCapture capture;
    StreamingAudio analyzer;
    std::array<float,8192> input{};
    std::array<float,1470> hop{};
    std::deque<float> pending;
    double lastPcm=0, lastAdvance=0, retry=0;
    auto time=[&] { return std::chrono::duration<double>(std::chrono::steady_clock::now()-start_).count(); };
    auto process=[&](double stamp) {
        analyzer.push(hop.data(),735,[&](const Audio& a,double dt) {
            std::lock_guard<std::mutex> lock(mutex_);
            frame_.audio=a; frame_.gain=analyzer.gain();
            frame_.score.advance(a,stamp,dt);
            frame_.schedule.advance(stamp,a,frame_.score);
        });
        lastAdvance=stamp;
    };
    while(!stop_) {
        const double now=time();
        if(!capture.running() && now>=retry) {
            const char* sink=std::getenv("OMADROP_AUDIO_SINK");
            const bool started=capture.start(sink?sink:"");
            std::fprintf(stderr,"capture: %s\n",started?"started existing PipeWireCapture":"unavailable; calm scene");
            retry=now+2;
        }
        // Bound reads and backlog, keeping the newest stereo-aligned hops.
        for(int i=0;i<8;++i) {
            const std::size_t n=capture.read(input.data(),input.size());
            if(!n) break;
            pending.insert(pending.end(),input.begin(),input.begin()+n); lastPcm=now;
        }
        if(pending.size()>1470*8) {
            const auto drop=(pending.size()-1470*8)/1470*1470;
            pending.erase(pending.begin(),pending.begin()+drop);
            std::fprintf(stderr,"capture: discarded %zu stale stereo frames\n",drop/2);
        }
        while(pending.size()>=hop.size()) {
            std::copy_n(pending.begin(),hop.size(),hop.begin());
            pending.erase(pending.begin(),pending.begin()+hop.size());
            // Events cannot be in the future, even if a helper bursts data.
            const double stamp=std::max(lastAdvance,now-double(pending.size())/88200);
            process(stamp);
        }
        // Missing/disconnected input also releases the response to silence.
        if(now-lastPcm>0.12 && now-lastAdvance>=1.0/60) { hop.fill(0); process(now); }
        std::this_thread::sleep_for(std::chrono::milliseconds(4));
    }
}
}
