#pragma once
#include "scene.h"
#include <array>
#include <deque>
#include <cstdint>

namespace Journey {
struct Event { double t, strength; std::uint64_t serial; };
struct Score {
    std::deque<Event> onsets, bassHits, midPeaks, surges;
    // Current means and bounded strand phases, never an entire-file timeline.
    std::array<double, 6> means{}, strandPhase{};
    // Fixed-size causal water controls. Integrals never store a PCM timeline;
    // three envelopes give a flowing surface inertia without frame history.
    std::array<double, 6> bandIntegrals{};
    std::array<std::array<double, 6>, 3> bandBody{};
    void advance(const Audio& a, double now, double dt);
    double mean(int band, double) const { return means[band]; }
    static const Event* last(const std::deque<Event>& events, double t);
    static double envelope(const std::deque<Event>& events, double t, double decay, double maxAge=1.5);
private:
    struct Sample { double t; Audio a; };
    std::deque<Sample> recent_;
    Audio previous_;
    double bassMean_=0, midMean_=0;
    double lastOnset_=-1, lastBass_=-1, lastMid_=-1;
    bool surgeArmed_=true;
    std::uint64_t serial_=0;
};
}
