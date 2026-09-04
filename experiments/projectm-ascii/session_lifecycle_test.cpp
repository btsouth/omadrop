#include "session_lifecycle.h"

#include <cassert>
#include <iostream>

int main() {
    SessionResumeDetector detector;
    assert(!detector.observe(100000, 90000));
    assert(!detector.observe(100016, 90016));

    // A slow or stopped render loop advances both clocks equally and must not
    // be mistaken for suspend.
    assert(!detector.observe(108016, 98016));

    // Five seconds pass in CLOCK_BOOTTIME but not in the active clock.
    assert(detector.observe(113032, 98032));
    assert(!detector.observe(113048, 98048));

    // Clock discontinuities rebase safely instead of emitting a false resume.
    assert(!detector.observe(20, 10));
    assert(!detector.observe(36, 26));
    assert(!detector.observe(0, 42));

    std::cout << "session lifecycle passed\n";
}
