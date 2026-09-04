#pragma once

#include <cerrno>
#include <chrono>
#include <csignal>
#include <sys/wait.h>
#include <thread>
#include <unistd.h>

// Only for children owned by this process. Helpers get their own process group
// so a shell waiting on curl, pactl, or an image decoder cannot strand shutdown.
inline void stopChildProcess(pid_t child) {
    if (child <= 0) return;
    auto reaped = [&] {
        const pid_t result = waitpid(child, nullptr, WNOHANG);
        return result == child || (result < 0 && errno == ECHILD);
    };
    if (reaped()) return;
    const bool ownsGroup = getpgid(child) == child;
    auto signal = [&](int value) {
        if (ownsGroup) kill(-child, value);
        kill(child, value);
    };
    signal(SIGTERM);
    for (int i = 0; i < 25; ++i) {
        if (reaped()) return;
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }
    signal(SIGKILL);
    for (int i = 0; i < 25; ++i) {
        if (reaped()) return;
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }
}
