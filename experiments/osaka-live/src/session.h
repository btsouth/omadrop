#pragma once
#include "audio.h"
#include "schedule.h"
#include "../../projectm-ascii/pipewire_capture.h"
#include <atomic>
#include <chrono>
#include <mutex>
#include <thread>
#include <vector>

namespace Journey {
struct LiveFrame { double seconds=0, gain=1; Audio audio; Score score; Schedule schedule; };
class LiveSession {
public:
    // With a fixture (interleaved stereo float32, 44100 Hz) the session loops it in real
    // time instead of capturing system audio.
    explicit LiveSession(int seed, std::vector<float> fixture={});
    ~LiveSession();
    LiveFrame snapshot();
private:
    void run();
    std::chrono::steady_clock::time_point start_;
    std::atomic_bool stop_{false};
    std::mutex mutex_;
    LiveFrame frame_;
    std::vector<float> fixture_;
    std::thread worker_;
};
}
