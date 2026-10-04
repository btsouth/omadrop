#pragma once
#include "audio.h"
#include "schedule.h"
#include "../../projectm-ascii/pipewire_capture.h"
#include <atomic>
#include <chrono>
#include <mutex>
#include <thread>

namespace Journey {
struct LiveFrame { double seconds=0, gain=1; Audio audio; Score score; Schedule schedule; };
class LiveSession {
public:
    explicit LiveSession(int seed);
    ~LiveSession();
    LiveFrame snapshot();
private:
    void run();
    std::chrono::steady_clock::time_point start_;
    std::atomic_bool stop_{false};
    std::mutex mutex_;
    LiveFrame frame_;
    std::thread worker_;
};
}
