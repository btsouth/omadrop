#include "display_session.h"
#include "cover_presentation.h"
#include "track_session.h"

#include <cassert>
#include <iostream>
#include <string>
#include <utility>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <thread>
#include <unistd.h>

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

    TrackPresentationState timeout(false, 100, 350);
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
    assert(!failed.artworkLookupComplete());
    assert(failed.advance(15100).startupArtworkTimedOut);

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

    // A slow cover must not block metadata or leak into a later song.
    char directory[] = "/tmp/omadrop-track-test-XXXXXX";
    assert(mkdtemp(directory));
    const std::filesystem::path fixture(directory);
    auto write = [&](const char* name, const std::string& content, bool executable=false) {
        const auto path=fixture/name;
        std::ofstream(path) << content;
        if(executable) std::filesystem::permissions(path,
            std::filesystem::perms::owner_all);
    };
    write("track", "a");
    write("mpris-state", "#!/bin/sh\n[ \"$1\" = --no-art ] || exit 9\n"
        "cd \"$(dirname \"$0\")\"\nt=$(cat track)\n"
        "printf '{\"identity\":\"%s\",\"playback_status\":\"Playing\","
        "\"art_path\":\"\",\"art_url\":\"%s\",\"position_seconds\":0,"
        "\"duration_seconds\":200}\\n' \"$t\" \"$t\"\n", true);
    write("art-fetch", "#!/bin/sh\n[ \"$1\" = a ] && sleep 3\n"
        "printf '/tmp/%s.png\\n' \"$1\"\n", true);
    {
        TrackSession session(fixture/"mpris-state",false,false,0);
        const auto began=std::chrono::steady_clock::now();
        bool sawA=false,sawB=false,receivedB=false,switched=false;
        while(std::chrono::steady_clock::now()-began<std::chrono::seconds(2)) {
            const auto ms=std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now()-began).count();
            if(ms>=400 && !switched) { write("track","b");switched=true; }
            auto value=session.update(ms);
            if(value.state && value.state->identity=="a") { sawA=true;assert(ms<1000); }
            if(value.state && value.state->identity=="b") sawB=true;
            if(value.artworkCandidate) {
                assert(*value.artworkCandidate=="/tmp/b.png");
                session.acceptArtwork(*value.artworkCandidate);receivedB=true;break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
        assert(sawA && sawB && receivedB);
    }
    // Startup regression: a slow cover must be the first visible content.
    write("track", "a");
    write("art-fetch", "#!/bin/sh\nsleep 1\nprintf '/tmp/%s.png\\n' \"$1\"\n", true);
    {
        TrackSession session(fixture/"mpris-state",false,false,0);
        DisplaySession display({});
        CoverPresentation cover(3.5f,1.75f);
        const auto began=std::chrono::steady_clock::now();
        bool shown=false;
        while(std::chrono::steady_clock::now()-began<std::chrono::seconds(3)) {
            const auto ms=std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now()-began).count();
            auto value=session.update(ms);
            if(value.artworkCandidate) {
                session.acceptArtwork(*value.artworkCandidate);
                cover.show(ms);
            }
            if(display.afterFramePresented(ms,session.artworkLookupComplete(),true)) {
                assert(ms>=1000);
                assert(cover.hasArtwork()); // No visual-first frame is allowed.
                cover.restart(ms);
                assert(cover.frame(ms+3499).coverMix==1.0f);
                assert(cover.frame(ms+4375).coverMix>0.49f);
                assert(cover.frame(ms+5250).complete);
                shown=true;break;
            }
            assert(!display.windowShown());
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
        assert(shown);
    }
    TrackPresentationState noCover(false,0);
    noCover.ingest(result("no-cover","",0),50);
    assert(noCover.artworkLookupComplete());
    assert(!noCover.ingest(result("no-cover","/tmp/late.png",0),2000).artworkCandidate);
    assert(noCover.ingest(result("new-song","/tmp/new.png",0),2100).artworkCandidate);
    TrackSession withoutMetadata(fixture/"mpris-state",false,false,0,true);
    assert(withoutMetadata.artworkLookupComplete());
    std::filesystem::remove_all(fixture);

    std::cout << "track session passed\n";
}
