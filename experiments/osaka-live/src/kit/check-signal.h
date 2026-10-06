#pragma once
// Built-in test music for the world check, used when no fixture file is
// given. Stereo float32 at 44100 Hz, identical on every run.
#include <vector>

namespace Journey::Kit::Check {
enum class SignalKind {
    Music,   // sixty seconds of drums, bass, pads and a lead, with a break and a drop
    Strobe   // a hard drum hit five times a second, for testing flash detection
};
std::vector<float> makeCheckSignal(SignalKind kind, double seconds = 60.0);
}
