#include "track_session.h"

#include <cassert>
#include <iostream>
#include <string>
#include <utility>

namespace {
MprisPollResult result(std::string identity, std::string art,
                       std::uint64_t startedAtMs, double position = 12.0) {
    MprisPollResult poll;
    poll.state = MprisState{
        std::move(identity), "Playing", std::move(art), position, 240.0};
    poll.startedAtMs = startedAtMs;
    return poll;
}
}

int main() {
    TrackPresentationState presentation(false, 100);
    auto update = presentation.ingest(
        result("track-a", "/tmp/a.png", 100), 140);
    assert(update.state && update.playback && update.playback->first);
    assert(update.artworkCandidate == "/tmp/a.png");
    assert(!update.clearArtwork);
    presentation.acceptArtwork(*update.artworkCandidate);
    assert(presentation.artworkLookupComplete());

    update = presentation.ingest(
        result("track-a", "/tmp/a.png", 1100, 13.0), 1140);
    assert(update.playback && !update.playback->trackChanged);
    assert(!update.artworkCandidate);

    update = presentation.ingest(
        result("track-b", "/tmp/b.png", 2100), 2140);
    assert(update.playback && update.playback->trackChanged);
    assert(update.clearArtwork);
    assert(update.artworkCandidate == "/tmp/b.png");

    TrackPresentationState timeout(false, 100);
    assert(!timeout.advance(449).startupArtworkTimedOut);
    update = timeout.advance(450);
    assert(update.startupArtworkTimedOut);
    assert(timeout.artworkLookupComplete());
    update = timeout.ingest(
        result("slow-launch", "/tmp/late.png", 100), 4700);
    assert(update.playback && update.playback->first);
    assert(!update.artworkCandidate);
    update = timeout.ingest(
        result("next-track", "/tmp/next.png", 5000), 5040);
    assert(update.playback && update.playback->trackChanged);
    assert(update.artworkCandidate == "/tmp/next.png");

    TrackPresentationState noPlayer(false, 100);
    MprisPollResult empty;
    update = noPlayer.ingest(empty, 150);
    assert(noPlayer.artworkLookupComplete());
    update = noPlayer.ingest(
        result("late-player", "/tmp/late-player.png", 4700), 4740);
    assert(update.playback && update.playback->first);
    assert(update.artworkCandidate == "/tmp/late-player.png");

    TrackPresentationState failed(false, 100);
    MprisPollResult error;
    error.error = "helper failed";
    update = failed.ingest(error, 200);
    assert(update.error == "helper failed");
    assert(failed.artworkLookupComplete());

    TrackPresentationState disabled(true, 100);
    assert(disabled.artworkLookupComplete());
    update = disabled.ingest(
        result("track", "/tmp/ignored.png", 100), 140);
    assert(update.playback && update.playback->first);
    assert(!update.artworkCandidate);

    TrackSession metadataDisabled(
        "/definitely/missing/omadrop-mpris-helper", true, false, 100, true);
    update = metadataDisabled.update(140);
    assert(update.error.empty());
    assert(!update.state);
    assert(metadataDisabled.artworkLookupComplete());

    std::cout << "track session passed\n";
}
