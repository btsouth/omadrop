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
        artworkLookupComplete_ = true;
        suppressLateInitialArtwork_ = true;
        update.startupArtworkTimedOut = true;
    }
    return update;
}

TrackSessionUpdate TrackPresentationState::ingest(
        const MprisPollResult& poll, std::uint64_t nowMs) {
    TrackSessionUpdate update = advance(nowMs);
    if (!poll.error.empty()) {
        update.error = poll.error;
        if (!artworkLookupComplete_) {
            artworkLookupComplete_ = true;
            suppressLateInitialArtwork_ = true;
        }
        return update;
    }
    if (!poll.state) {
        if (!artworkLookupComplete_) {
            artworkLookupComplete_ = true;
            suppressLateInitialArtwork_ = true;
        }
        return update;
    }

    const MprisState& state = *poll.state;
    const PlaybackObservation observation
        = playbackClock_.observe(state, nowMs / 1000.0);
    if (observation.first && suppressLateInitialArtwork_
        && poll.startedAtMs >= startupArtworkDeadlineMs_) {
        suppressLateInitialArtwork_ = false;
    }
    if (observation.trackChanged) suppressLateInitialArtwork_ = false;
    if (observation.trackChanged) {
        currentArtworkPath_.clear();
        update.clearArtwork = true;
    }
    update.state = state;
    update.playback = observation;
    if (!artworkDisabled_ && !suppressLateInitialArtwork_
        && !state.artPath.empty()
        && state.artPath != currentArtworkPath_) {
        update.artworkCandidate = state.artPath;
    }
    return update;
}

void TrackPresentationState::acceptArtwork(const std::string& path) {
    if (artworkDisabled_ || path.empty()) return;
    currentArtworkPath_ = path;
    artworkLookupComplete_ = true;
}

TrackSession::TrackSession(std::filesystem::path helper, bool artworkDisabled,
                           bool frequentPolling, std::uint64_t startedAtMs,
                           bool metadataDisabled)
    : poller_(std::move(helper)),
      presentation_(artworkDisabled, startedAtMs),
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
        if (!poller_.start(artworkDisabled_, nowMs)) {
            update.error = "could not start MPRIS helper";
            nextPollAtMs_ = nowMs + 1000;
        }
    }
    if (const auto poll = poller_.update()) {
        nextPollAtMs_ = nowMs + (!presentation_.artworkLookupComplete()
            ? 250 : frequentPolling_ ? 500 : 1000);
        merge(update, presentation_.ingest(*poll, nowMs));
    }
    return update;
}

void TrackSession::resume(std::uint64_t nowMs) {
    poller_.stop();
    nextPollAtMs_ = nowMs;
}
