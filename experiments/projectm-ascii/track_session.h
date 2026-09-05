#pragma once

#include "mpris_poller.h"
#include "mpris_state.h"

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>

struct TrackSessionUpdate {
    std::optional<MprisState> state;
    std::optional<PlaybackObservation> playback;
    std::optional<std::string> artworkCandidate;
    std::string error;
    bool clearArtwork = false;
    bool startupArtworkTimedOut = false;
};

class TrackPresentationState {
public:
    TrackPresentationState(bool artworkDisabled, std::uint64_t startedAtMs,
                           std::uint64_t startupArtworkWaitMs = 15000);

    TrackSessionUpdate advance(std::uint64_t nowMs);
    TrackSessionUpdate ingest(const MprisPollResult& poll,
                              std::uint64_t nowMs);
    void acceptArtwork(const std::string& path);
    void finishStartupWithoutArtwork();
    bool artworkAllowed() const { return !suppressStartupArtwork_; }

    bool artworkLookupComplete() const { return artworkLookupComplete_; }
    PlaybackClock& playbackClock() { return playbackClock_; }
    const PlaybackClock& playbackClock() const { return playbackClock_; }

private:
    PlaybackClock playbackClock_;
    std::string currentArtworkPath_;
    std::uint64_t startupArtworkDeadlineMs_ = 0;
    bool suppressStartupArtwork_ = false;
    bool artworkDisabled_ = false;
    bool artworkLookupComplete_ = false;
};

class TrackSession {
public:
    TrackSession(std::filesystem::path helper, bool artworkDisabled,
                 bool frequentPolling, std::uint64_t startedAtMs,
                 bool metadataDisabled = false);

    TrackSessionUpdate update(std::uint64_t nowMs);
    void acceptArtwork(const std::string& path) {
        presentation_.acceptArtwork(path);
    }
    void resume(std::uint64_t nowMs);
    void stop() { poller_.stop(); artworkPoller_.stop(); }

    bool artworkLookupComplete() const {
        return presentation_.artworkLookupComplete();
    }
    PlaybackClock& playbackClock() { return presentation_.playbackClock(); }
    const PlaybackClock& playbackClock() const {
        return presentation_.playbackClock();
    }

private:
    static void merge(TrackSessionUpdate& target,
                      TrackSessionUpdate incoming);

    MprisPoller poller_;
    MprisPoller artworkPoller_;
    TrackPresentationState presentation_;
    std::uint64_t nextPollAtMs_ = 0;
    bool artworkDisabled_ = false;
    bool frequentPolling_ = false;
    bool metadataDisabled_ = false;
    std::string artworkIdentity_;
    std::string artworkUrl_;
    std::string deliveredArtwork_;
    std::uint64_t nextArtworkRetryAtMs_ = 0;
};
