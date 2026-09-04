#include "scripted_scene_sequence.h"

#include <algorithm>
#include <utility>

ScriptedSceneSequence::ScriptedSceneSequence(
        ScriptedSceneSequenceConfig config)
    : config_(std::move(config)) {
    config_.minimumDwellSeconds = std::clamp(
        config_.minimumDwellSeconds, 1.0f, 30.0f);
    config_.maximumDwellSeconds = std::clamp(
        config_.maximumDwellSeconds,
        config_.minimumDwellSeconds, 32.0f);
    config_.transitionSeconds = std::clamp(
        config_.transitionSeconds, 0.6f, 8.0f);
}

ScriptedSceneCue ScriptedSceneSequence::update(
        std::uint64_t nowMs, const MusicFrame& music,
        bool presentationReady, bool sceneTransitioning) {
    ScriptedSceneCue cue;
    const bool barBoundary = music.clockConfidence >= 0.35f
                          && previousBarPhase_ > 0.72f
                          && music.barPhase < 0.28f;
    previousBarPhase_ = music.barPhase;
    if (!active() || completed_ || !presentationReady
        || sceneTransitioning) {
        return cue;
    }
    if (sceneSettledAtMs_ == 0) {
        sceneSettledAtMs_ = nowMs;
        return cue;
    }
    const float dwellSeconds = nowMs >= sceneSettledAtMs_
        ? (nowMs - sceneSettledAtMs_) / 1000.0f : 0.0f;
    const bool cueDue = dwellSeconds >= config_.maximumDwellSeconds
        || (dwellSeconds >= config_.minimumDwellSeconds && barBoundary);
    if (!cueDue) return cue;

    if (sceneIndex_ + 1 < config_.scenes.size()) {
        ++sceneIndex_;
        cue.scene = config_.scenes[sceneIndex_];
        sceneSettledAtMs_ = 0;
    } else if (config_.playOnce) {
        completed_ = true;
        cue.complete = true;
    } else {
        sceneIndex_ = 0;
        cue.scene = config_.scenes.front();
        sceneSettledAtMs_ = 0;
    }
    return cue;
}

void ScriptedSceneSequence::markSceneSettled(std::uint64_t nowMs) {
    if (active() && !completed_) sceneSettledAtMs_ = nowMs;
}
