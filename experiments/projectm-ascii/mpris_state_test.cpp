#include "mpris_state.h"

#include <cassert>
#include <cmath>
#include <iostream>
#include <string>
#include <utility>

namespace {
MprisState state(std::string identity, std::string status, double position,
                 double duration = 240.0) {
    return {std::move(identity), std::move(status), "/tmp/cover.png",
            position, duration};
}

bool closeTo(double actual, double expected) {
    return std::abs(actual - expected) < 0.001;
}
}

int main() {
    std::string error;
    const auto parsed = parseMprisState(
        R"({"identity":"player:track-a","playback_status":"Playing","art_path":"/tmp/cover.png","position_seconds":42.0,"duration_seconds":240.0})",
        error);
    assert(parsed);
    assert(parsed->identity == "player:track-a");
    assert(parsed->playbackStatus == "Playing");
    assert(closeTo(parsed->positionSeconds, 42.0));
    assert(!parseMprisState(R"({"identity":"broken"})", error));

    PlaybackClock clock;
    PlaybackObservation observation = clock.observe(
        state("player:track-a", "Playing", 42.0), 100.0);
    assert(observation.first);
    assert(!observation.trackChanged);
    assert(!observation.seeked);
    assert(closeTo(clock.positionAt(102.0), 44.0));
    assert(closeTo(clock.progressAt(102.0), 44.0 / 240.0));

    observation = clock.observe(
        state("player:track-a", "Playing", 44.0), 102.0);
    assert(!observation.first && !observation.trackChanged
           && !observation.seeked);

    observation = clock.observe(
        state("player:track-a", "Paused", 45.0), 103.0);
    assert(!observation.seeked);
    assert(closeTo(clock.positionAt(180.0), 45.0));

    observation = clock.observe(
        state("player:track-a", "Playing", 45.0), 181.0);
    assert(!observation.seeked);
    assert(closeTo(clock.positionAt(182.5), 46.5));

    observation = clock.observe(
        state("player:track-a", "Playing", 80.0), 183.0);
    assert(observation.seeked);
    assert(!observation.trackChanged);

    observation = clock.observe(
        state("player:track-b", "Playing", 12.0, 90.0), 184.0);
    assert(observation.trackChanged);
    assert(!observation.seeked);
    assert(clock.identity() == "player:track-b");
    assert(closeTo(clock.positionAt(400.0), 90.0));
    assert(closeTo(clock.progressAt(400.0), 1.0));

    PlaybackClock unknownDuration;
    unknownDuration.observe(
        state("stream", "Playing", 12.0, 0.0), 500.0);
    assert(unknownDuration.progressAt(501.0) < 0.0);

    std::cout << "MPRIS state passed\n";
}
