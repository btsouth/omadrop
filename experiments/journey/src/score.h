#pragma once

// Discrete musical events measured once from the analysed timeline, so the
// worlds remain a pure function of movie time (scrubbable, deterministic).
// These are energy events, not instrument or section recognition.
#include "audio.h"
#include <array>
#include <vector>

namespace Journey {
struct Event { double t; double strength; };

struct Score {
    std::vector<Event> onsets;   // accent peaks
    std::vector<Event> bassHits; // strong bass attacks
    std::vector<Event> midPeaks; // 400..1500 Hz peaks
    std::vector<Event> surges;   // musical surge starts

    // Running integrals of each band (band-seconds), for travel that speeds
    // up with the music without jumping when the level changes.
    std::vector<std::array<float, 6>> bandIntegral; // 120 Hz
    double integral(int band, double t) const;
    // Slow running mean of each band (about 1.5 s), so objects can answer how
    // far a band stands above its recent level rather than its absolute
    // loudness (dense, compressed mixes keep every band near the same level).
    std::vector<std::array<float, 6>> bandMean; // 120 Hz
    double mean(int band, double t) const;
    // Decaying envelope of the most recent event: strength * exp(-age * decay).
    static double envelope(const std::vector<Event>& list, double t, double decay, double maxAge = 1.5);

    void build(const AudioTimeline& audio, double duration, bool hasAudio);
    // Events in [from, to).
    static int count(const std::vector<Event>& list, double from, double to);
    // Most recent event at or before t (t - age), or nullptr.
    static const Event* last(const std::vector<Event>& list, double t);
    static const Event* first(const std::vector<Event>& list, double from, double to);
};
}
