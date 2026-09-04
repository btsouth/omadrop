#pragma once

#include "music_frame.h"
#include "native_scene_registry.h"

#include <cstdint>
#include <optional>
#include <vector>

struct ScriptedSceneSequenceConfig {
    std::vector<NativeSceneKind> scenes;
    float minimumDwellSeconds = 5.0f;
    float maximumDwellSeconds = 7.0f;
    float transitionSeconds = 2.0f;
    bool playOnce = false;
};

struct ScriptedSceneCue {
    std::optional<NativeSceneKind> scene;
    bool complete = false;
};

class ScriptedSceneSequence {
public:
    explicit ScriptedSceneSequence(ScriptedSceneSequenceConfig config);

    ScriptedSceneCue update(std::uint64_t nowMs, const MusicFrame& music,
                            bool presentationReady,
                            bool sceneTransitioning);
    void markSceneSettled(std::uint64_t nowMs);

    bool active() const { return config_.scenes.size() >= 2; }
    NativeSceneKind firstScene() const { return config_.scenes.front(); }
    float transitionSeconds() const { return config_.transitionSeconds; }

private:
    ScriptedSceneSequenceConfig config_;
    std::size_t sceneIndex_ = 0;
    std::uint64_t sceneSettledAtMs_ = 0;
    float previousBarPhase_ = 0.0f;
    bool completed_ = false;
};
