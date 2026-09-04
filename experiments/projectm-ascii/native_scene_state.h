#pragma once

#include "music_frame.h"
#include "native_scene_registry.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <deque>
#include <limits>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

struct NativeSceneState {
    float development = 0.0f;
    float drive = 0.0f;
    float peak = 0.0f;
    float release = 0.0f;
    float sceneBeats = 0.0f;
    NativeSceneKind currentScene = NativeSceneKind::DepthTunnel;
    NativeSceneKind incomingScene = NativeSceneKind::Centrifuge;
    NativeTransitionStyle transitionStyle = NativeTransitionStyle::FlowCarry;
    float transition = 0.0f;
    bool transitioning = false;
    bool motifRecalled = false;
};

// Turns musical measurements into a slow scene lifecycle. The renderer gets
// deliberate compositional state instead of deriving long-form behavior from
// momentary audio levels.
class NativeSceneDirector {
public:
    const NativeSceneState& update(const MusicFrame& music, float seconds,
                                   bool allowAutomaticTransitions = true) {
        const float dt = std::clamp(seconds, 1.0f / 240.0f, 0.1f);
        const bool sectionStarted = music.section > 0.72f && previousSection_ <= 0.72f;
        const bool phraseStarted = previousPhrasePhase_ > 0.78f
                                && music.phrasePhase < 0.22f;
        const bool barStarted = previousBarPhase_ > 0.72f
                             && music.barPhase < 0.28f;
        const bool musicActive = music.energyFast > 0.018f
                              || music.percussive > 0.035f
                              || music.harmonic > 0.035f;
        const bool sectionTransition = sectionStarted && dwellBeats_ >= 16.0f;
        const bool phraseFallback = phraseStarted && dwellBeats_ >= 31.5f;
        const bool barFallback = barStarted && dwellBeats_ >= 47.5f;
        const bool maximumDwell = dwellBeats_ >= 64.0f;
        const bool automaticTransition = allowAutomaticTransitions
                                      && musicActive
                                      && (sectionTransition || phraseFallback
                                          || barFallback || maximumDwell);
        state_.motifRecalled = false;
        bool hasRecalledScene = false;
        NativeSceneKind recalledScene = state_.currentScene;
        if (sectionStarted && music.motifIdentity >= 0) {
            const auto remembered = motifScenes_.find(music.motifIdentity);
            if (remembered != motifScenes_.end()
                && !sceneHidden(remembered->second)) {
                state_.motifRecalled = true;
                hasRecalledScene = true;
                recalledScene = remembered->second;
            }
        }
        const bool recallTransition = allowAutomaticTransitions && hasRecalledScene
                                   && recalledScene != state_.currentScene
                                   && !sceneHidden(recalledScene)
                                   && dwellBeats_ >= 8.0f;
        if (pendingScene_ && *pendingScene_ == state_.currentScene
            && !state_.transitioning) {
            pendingScene_.reset();
            pendingTransitionStyle_.reset();
        }
        std::optional<NativeSceneKind> automaticScene;
        if (automaticTransition) automaticScene = chooseAutomaticScene(music);
        if (!state_.transitioning
            && (pendingScene_ || pendingDirection_ != 0
                || recallTransition || automaticTransition)) {
            state_.incomingScene = pendingScene_ ? *pendingScene_
                : pendingDirection_ != 0
                ? nextVisibleScene(state_.currentScene, pendingDirection_)
                : recallTransition ? recalledScene
                : *automaticScene;
            const float transitionEnergy = std::clamp(
                0.58f * music.energyFast + 0.42f * music.energySlow,
                0.0f, 1.0f);
            const NativeTransitionContext transitionContext{
                .energy = transitionEnergy,
                .percussive = music.percussive,
                .harmonic = music.harmonic,
                .rhythmicDensity = music.rhythmicDensity,
                .harmonicChange = music.harmonicChange,
                .energySlope = music.energySlope,
            };
            state_.transitionStyle = pendingTransitionStyle_.value_or(
                nativeTransitionStyle(state_.currentScene,
                                      state_.incomingScene,
                                      transitionContext));
            state_.transitioning = true;
            transitionElapsed_ = 0.0f;
            state_.sceneBeats = 0.0f;
            dwellBeats_ = 0.0f;
            transitionDuration_ = transitionSecondsOverride_ > 0.0f
                ? transitionSecondsOverride_
                : std::clamp(
                    4.0f * 60.0f / std::clamp(music.bpm, 60.0f, 190.0f),
                    2.0f, 5.0f);
            pendingDirection_ = 0;
            pendingScene_.reset();
            pendingTransitionStyle_.reset();
            rememberSceneUse(state_.incomingScene);
        }
        if (sectionStarted && music.motifIdentity >= 0 && !hasRecalledScene) {
            motifScenes_[music.motifIdentity] = state_.transitioning
                ? state_.incomingScene : state_.currentScene;
        }
        const float elapsedBeats
            = dt * std::clamp(music.bpm, 60.0f, 190.0f) / 60.0f;
        state_.sceneBeats += elapsedBeats;
        if (!state_.transitioning) dwellBeats_ += elapsedBeats;

        const float developmentTarget = smoothstep(1.0f, 12.0f, state_.sceneBeats);
        const float driveTarget = smoothstep(0.28f, 0.62f,
            0.55f * music.energyFast + 0.45f * music.energySlow);
        const float peakTarget = smoothstep(0.58f, 0.84f, music.energyFast)
                               * smoothstep(0.18f, 0.62f, music.percussive);
        const float releaseTarget = smoothstep(0.025f, 0.22f, -music.energySlope);

        state_.development = smooth(state_.development, developmentTarget, 0.65f, dt);
        state_.drive = smooth(state_.drive, driveTarget, 1.2f, dt);
        state_.peak = smooth(state_.peak, peakTarget, 1.8f, dt);
        state_.release = smooth(state_.release, releaseTarget,
                                releaseTarget > state_.release ? 2.4f : 0.7f, dt);
        if (state_.transitioning) {
            transitionElapsed_ += dt;
            state_.transition = smoothstep(
                0.0f, 1.0f, transitionElapsed_ / transitionDuration_);
            if (transitionElapsed_ >= transitionDuration_) {
                state_.currentScene = state_.incomingScene;
                state_.incomingScene = nativeSceneOffset(state_.currentScene, 1);
                state_.transitioning = false;
                state_.transition = 0.0f;
                dwellBeats_ = 0.0f;
            }
        }
        previousSection_ = music.section;
        previousPhrasePhase_ = music.phrasePhase;
        previousBarPhase_ = music.barPhase;
        return state_;
    }

    void requestNext() { pendingDirection_ = 1; }
    void requestPrevious() { pendingDirection_ = -1; }
    void requestScene(NativeSceneKind scene) { pendingScene_ = scene; }
    void requestTransitionStyle(NativeTransitionStyle style) {
        pendingTransitionStyle_ = style;
    }
    void setTransitionDuration(float seconds) {
        transitionSecondsOverride_ = seconds > 0.0f
            ? std::clamp(seconds, 0.6f, 8.0f) : -1.0f;
    }
    void setProfile(NativeDirectorProfile profile) { profile_ = profile; }
    NativeDirectorProfile profile() const { return profile_; }
    void setScenePreferences(const std::vector<std::string>& favorites,
                             const std::vector<std::string>& hidden) {
        favoriteScenes_.fill(false);
        hiddenScenes_.fill(false);
        for (const std::string& slug : favorites) {
            NativeSceneKind scene;
            if (nativeSceneFromName(slug, scene)) {
                favoriteScenes_[static_cast<std::size_t>(scene)] = true;
            }
        }
        for (const std::string& slug : hidden) {
            NativeSceneKind scene;
            if (nativeSceneFromName(slug, scene)) {
                hiddenScenes_[static_cast<std::size_t>(scene)] = true;
            }
        }
        if (visibleSceneCount() < 2) hiddenScenes_.fill(false);
    }
    bool sceneFavorite(NativeSceneKind scene) const {
        return favoriteScenes_[static_cast<std::size_t>(scene)];
    }
    bool sceneHidden(NativeSceneKind scene) const {
        return hiddenScenes_[static_cast<std::size_t>(scene)];
    }
    std::size_t visibleSceneCount() const {
        return static_cast<std::size_t>(std::count(
            hiddenScenes_.begin(), hiddenScenes_.end(), false));
    }
    const NativeSceneState& state() const { return state_; }
    void selectScene(NativeSceneKind scene) {
        state_ = NativeSceneState{};
        state_.currentScene = scene;
        state_.incomingScene = nativeSceneOffset(scene, 1);
        recentScenes_.clear();
        sceneUseCount_.fill(0);
        rememberSceneUse(scene);
        pendingDirection_ = 0;
        pendingScene_.reset();
        pendingTransitionStyle_.reset();
        transitionElapsed_ = 0.0f;
        dwellBeats_ = 0.0f;
        previousSection_ = 0.0f;
        previousPhrasePhase_ = 0.0f;
        previousBarPhase_ = 0.0f;
    }
    void resetForTrack() {
        const NativeSceneKind retainedScene
            = state_.transitioning
                ? state_.incomingScene : state_.currentScene;
        const float retainedTransitionOverride = transitionSecondsOverride_;
        const NativeDirectorProfile retainedProfile = profile_;
        const auto retainedFavorites = favoriteScenes_;
        const auto retainedHidden = hiddenScenes_;
        *this = NativeSceneDirector{};
        transitionSecondsOverride_ = retainedTransitionOverride;
        profile_ = retainedProfile;
        favoriteScenes_ = retainedFavorites;
        hiddenScenes_ = retainedHidden;
        selectScene(retainedScene);
    }
    void reset() { *this = NativeSceneDirector{}; }

private:
    NativeSceneKind chooseAutomaticScene(const MusicFrame& music) const {
        const float energy = std::clamp(
            0.58f * music.energyFast + 0.42f * music.energySlow, 0.0f, 1.0f);
        const float percussive = std::clamp(music.percussive, 0.0f, 1.0f);
        const float harmonic = std::clamp(music.harmonic, 0.0f, 1.0f);
        const float centroid = std::clamp(music.spectralCentroid, 0.0f, 1.0f);
        const float stereo = std::clamp(music.stereoWidth, 0.0f, 1.0f);
        NativeSceneKind best = nextVisibleScene(state_.currentScene, 1);
        float bestScore = std::numeric_limits<float>::max();
        bool found = false;
        for (std::size_t index = 0; index < nativeSceneCount; ++index) {
            const NativeSceneKind candidate = static_cast<NativeSceneKind>(index);
            if (candidate == state_.currentScene) continue;
            if (sceneHidden(candidate)) continue;
            if (nativeSceneDefinition(candidate).visualFamily
                == nativeSceneDefinition(state_.currentScene).visualFamily) {
                continue;
            }
            const NativeSceneSelectionTraits& traits
                = nativeSceneDefinition(candidate).selection;
            const auto square = [](float value) { return value * value; };
            float score = 1.55f * square(energy - traits.energy)
                        + 1.30f * square(percussive - traits.percussive)
                        + 1.10f * square(harmonic - traits.harmonic)
                        + 0.72f * square(centroid - traits.centroid)
                        + 0.55f * square(stereo - traits.stereo)
                        + 0.055f * sceneUseCount_[index];
            if (sceneFavorite(candidate)) score -= 0.20f;
            const NativeSceneDefinition& currentDefinition
                = nativeSceneDefinition(state_.currentScene);
            const NativeSceneDefinition& candidateDefinition
                = nativeSceneDefinition(candidate);
            if (profile_ == NativeDirectorProfile::Kinetic) {
                score -= 0.16f * traits.percussive + 0.10f * traits.energy;
                if (candidateDefinition.motionGrammar
                    == NativeMotionGrammar::Flow) score -= 0.10f;
                if (candidateDefinition.motionGrammar
                    == NativeMotionGrammar::Sparse) score += 0.16f;
            } else if (profile_ == NativeDirectorProfile::Restrained) {
                score += 0.22f * traits.energy + 0.12f * traits.percussive;
                if (candidateDefinition.motionGrammar
                    == NativeMotionGrammar::Flow) score += 0.48f;
                if (candidateDefinition.motionGrammar
                    == NativeMotionGrammar::Sparse) score -= 0.12f;
            } else if (profile_ == NativeDirectorProfile::HighContrast) {
                if (candidateDefinition.motionGrammar
                    == currentDefinition.motionGrammar) score += 0.32f;
                else score -= 0.08f;
                if (candidateDefinition.transitionAnchor
                    == currentDefinition.transitionAnchor) score += 0.14f;
            }
            for (std::size_t age = 0; age < recentScenes_.size(); ++age) {
                const NativeSceneKind recent
                    = recentScenes_[recentScenes_.size() - 1 - age];
                if (recent == candidate) {
                    score += age == 0 ? 4.0f : age == 1 ? 1.5f : 0.55f;
                    break;
                }
                if (nativeSceneDefinition(recent).visualFamily
                    == nativeSceneDefinition(candidate).visualFamily) {
                    score += age == 0 ? 1.65f : age == 1 ? 0.62f : 0.24f;
                    break;
                }
            }
            if (score < bestScore) {
                bestScore = score;
                best = candidate;
                found = true;
            }
        }
        return found ? best : nextVisibleScene(state_.currentScene, 1);
    }

    NativeSceneKind nextVisibleScene(NativeSceneKind current,
                                     int direction) const {
        NativeSceneKind candidate = current;
        for (std::size_t step = 0; step < nativeSceneCount; ++step) {
            candidate = nativeSceneOffset(candidate, direction);
            if (!sceneHidden(candidate)) return candidate;
        }
        return current;
    }

    void rememberSceneUse(NativeSceneKind scene) {
        recentScenes_.push_back(scene);
        while (recentScenes_.size() > 4) recentScenes_.pop_front();
        ++sceneUseCount_[static_cast<std::size_t>(scene)];
    }

    static float smooth(float current, float target, float speed, float seconds) {
        return current + (target - current) * (1.0f - std::exp(-speed * seconds));
    }

    static float smoothstep(float lower, float upper, float value) {
        const float position = std::clamp((value - lower) / (upper - lower), 0.0f, 1.0f);
        return position * position * (3.0f - 2.0f * position);
    }

    NativeSceneState state_{};
    float previousSection_ = 0.0f;
    float previousPhrasePhase_ = 0.0f;
    float previousBarPhase_ = 0.0f;
    float dwellBeats_ = 0.0f;
    int pendingDirection_ = 0;
    std::optional<NativeSceneKind> pendingScene_;
    std::optional<NativeTransitionStyle> pendingTransitionStyle_;
    float transitionElapsed_ = 0.0f;
    float transitionDuration_ = 4.0f;
    float transitionSecondsOverride_ = -1.0f;
    NativeDirectorProfile profile_ = NativeDirectorProfile::Balanced;
    std::array<bool, nativeSceneCount> favoriteScenes_{};
    std::array<bool, nativeSceneCount> hiddenScenes_{};
    std::unordered_map<int, NativeSceneKind> motifScenes_;
    std::deque<NativeSceneKind> recentScenes_;
    std::array<unsigned int, nativeSceneCount> sceneUseCount_{};
};
