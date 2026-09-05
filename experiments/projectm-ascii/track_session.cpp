#include "track_session.h"

#include <utility>

TrackPresentationState::TrackPresentationState(
        bool artworkDisabled, std::uint64_t startedAtMs,
        std::uint64_t startupArtworkWaitMs)
    : startupArtworkDeadlineMs_(startedAtMs + startupArtworkWaitMs),
      artworkDisabled_(artworkDisabled),
      artworkLookupComplete_(artworkDisabled) {}

TrackSessionUpdate TrackPresentationState::advance(std::uint64_t nowMs) {
    TrackSessionUpdate update;
    if (!artworkLookupComplete_ && nowMs >= startupArtworkDeadlineMs_) {
        finishStartupWithoutArtwork();
        update.startupArtworkTimedOut = true;
    }
    return update;
}

TrackSessionUpdate TrackPresentationState::ingest(
        const MprisPollResult& poll, std::uint64_t nowMs) {
    TrackSessionUpdate update = advance(nowMs);
    if (!poll.error.empty()) {
        update.error = poll.error;
        // Retry metadata before the bounded startup deadline.
        return update;
    }
    if (!poll.state) {
        if (!artworkLookupComplete_) {
            artworkLookupComplete_ = true;
        }
        return update;
    }

    const MprisState& state = *poll.state;
    const PlaybackObservation observation
        = playbackClock_.observe(state, nowMs / 1000.0);
    if (observation.trackChanged) {
        suppressStartupArtwork_ = false;
        currentArtworkPath_.clear();
        update.clearArtwork = true;
    }
    if (!artworkLookupComplete_ && state.artPath.empty() && state.artUrl.empty()) {
        finishStartupWithoutArtwork();
    }
    update.state = state;
    update.playback = observation;
    if (!artworkDisabled_ && artworkAllowed()
        && !state.artPath.empty()
        && state.artPath != currentArtworkPath_) {
        update.artworkCandidate = state.artPath;
    }
    return update;
}

void TrackPresentationState::finishStartupWithoutArtwork() {
    if (artworkLookupComplete_) return;
    artworkLookupComplete_ = true;
    // Once the initial scene is revealed, do not interrupt it with a late
    // startup cover. The next actual track change permits artwork again.
    suppressStartupArtwork_ = true;
}

void TrackPresentationState::acceptArtwork(const std::string& path) {
    if (artworkDisabled_ || path.empty()) return;
    currentArtworkPath_ = path;
    artworkLookupComplete_ = true;
}

TrackSession::TrackSession(std::filesystem::path helper, bool artworkDisabled,
                           bool frequentPolling, std::uint64_t startedAtMs,
                           bool metadataDisabled)
    : poller_(helper),
      artworkPoller_(helper.parent_path() / "art-fetch"),
      presentation_(artworkDisabled || metadataDisabled, startedAtMs),
      nextPollAtMs_(startedAtMs),
      artworkDisabled_(artworkDisabled),
      frequentPolling_(frequentPolling),
      metadataDisabled_(metadataDisabled) {}

void TrackSession::merge(TrackSessionUpdate& target,
                         TrackSessionUpdate incoming) {
    if (incoming.state) target.state = std::move(incoming.state);
    if (incoming.playback) target.playback = incoming.playback;
    if (incoming.artworkCandidate) {
        target.artworkCandidate = std::move(incoming.artworkCandidate);
    }
    if (!incoming.error.empty()) target.error = std::move(incoming.error);
    target.clearArtwork = target.clearArtwork || incoming.clearArtwork;
    target.startupArtworkTimedOut
        = target.startupArtworkTimedOut || incoming.startupArtworkTimedOut;
}

TrackSessionUpdate TrackSession::update(std::uint64_t nowMs) {
    TrackSessionUpdate update = presentation_.advance(nowMs);
    if (metadataDisabled_) return update;
    if (nowMs >= nextPollAtMs_ && !poller_.running()) {
        // Metadata must never wait for downloading or decoding an image.
        if (!poller_.start(true, nowMs)) {
            update.error = "could not start MPRIS helper";
            nextPollAtMs_ = nowMs + 1000;
        }
    }
    if (const auto poll = poller_.update()) {
        nextPollAtMs_ = nowMs + 250;
        merge(update, presentation_.ingest(*poll, nowMs));
    }
    if (!artworkDisabled_ && update.state) {
        const auto& state = *update.state;
        if (state.identity != artworkIdentity_ || state.artUrl != artworkUrl_) {
            artworkPoller_.stop();
            artworkIdentity_ = state.identity;
            artworkUrl_ = state.artUrl;
            deliveredArtwork_.clear();
            nextArtworkRetryAtMs_ = nowMs;
        }
        if (!state.artPath.empty()) deliveredArtwork_ = state.artPath;
    }
    if (!artworkDisabled_ && presentation_.artworkAllowed()
        && !artworkUrl_.empty() && deliveredArtwork_.empty()
        && !artworkPoller_.running() && nowMs >= nextArtworkRetryAtMs_) {
        if (!artworkPoller_.startArtwork(artworkUrl_, nowMs))
            nextArtworkRetryAtMs_ = nowMs + 1000;
    }
    if (const auto artwork = artworkPoller_.update()) {
        if (artwork->artworkPath && presentation_.artworkAllowed()
            && presentation_.playbackClock().identity() == artworkIdentity_) {
            deliveredArtwork_ = *artwork->artworkPath;
            update.artworkCandidate = deliveredArtwork_;
        } else {
            presentation_.finishStartupWithoutArtwork();
            nextArtworkRetryAtMs_ = nowMs + 2000;
            if (!artwork->error.empty()) update.error = artwork->error;
        }
    }
    return update;
}

void TrackSession::resume(std::uint64_t nowMs) {
    stop();
    nextPollAtMs_ = nowMs;
}
