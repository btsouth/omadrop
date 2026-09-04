#pragma once

#include <cstdint>
#include <ctime>

inline std::uint64_t bootTimeMilliseconds() {
#if defined(CLOCK_BOOTTIME)
    timespec now{};
    if (clock_gettime(CLOCK_BOOTTIME, &now) == 0) {
        return static_cast<std::uint64_t>(now.tv_sec) * 1000u
             + static_cast<std::uint64_t>(now.tv_nsec) / 1000000u;
    }
#endif
    return 0;
}

// CLOCK_BOOTTIME advances while Linux is suspended; the normal render clock
// does not. Comparing their interval deltas distinguishes a real suspend from
// an ordinary slow frame, debugger stop, or overloaded process.
class SessionResumeDetector {
public:
    bool observe(std::uint64_t bootMilliseconds,
                 std::uint64_t activeMilliseconds) {
        if (bootMilliseconds == 0) return false;
        if (!initialized_ || bootMilliseconds < previousBootMilliseconds_
            || activeMilliseconds < previousActiveMilliseconds_) {
            initialized_ = true;
            previousBootMilliseconds_ = bootMilliseconds;
            previousActiveMilliseconds_ = activeMilliseconds;
            return false;
        }
        const std::uint64_t bootElapsed
            = bootMilliseconds - previousBootMilliseconds_;
        const std::uint64_t activeElapsed
            = activeMilliseconds - previousActiveMilliseconds_;
        previousBootMilliseconds_ = bootMilliseconds;
        previousActiveMilliseconds_ = activeMilliseconds;
        return bootElapsed > activeElapsed + minimumSuspendMilliseconds;
    }

private:
    static constexpr std::uint64_t minimumSuspendMilliseconds = 1500;
    bool initialized_ = false;
    std::uint64_t previousBootMilliseconds_ = 0;
    std::uint64_t previousActiveMilliseconds_ = 0;
};
