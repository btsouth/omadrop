#include "scripted_scene_sequence.h"

#include <cassert>
#include <iostream>

namespace {
MusicFrame music(float barPhase, float confidence = 0.9f) {
    MusicFrame frame;
    frame.barPhase = barPhase;
    frame.clockConfidence = confidence;
    return frame;
}
}

int main() {
    ScriptedSceneSequence once({
        .scenes = {
            NativeSceneKind::DepthTunnel,
            NativeSceneKind::InkCurrent,
            NativeSceneKind::PaperHorizon,
        },
        .minimumDwellSeconds = 5.0f,
        .maximumDwellSeconds = 7.0f,
        .transitionSeconds = 1.2f,
        .playOnce = true,
    });
    assert(once.active());
    assert(once.firstScene() == NativeSceneKind::DepthTunnel);
    assert(once.transitionSeconds() == 1.2f);
    assert(!once.update(100, music(0.8f), false, false).scene);
    assert(!once.update(200, music(0.1f), true, false).scene);
    assert(!once.update(5000, music(0.8f), true, false).scene);
    const ScriptedSceneCue second
        = once.update(5300, music(0.1f), true, false);
    assert(second.scene == NativeSceneKind::InkCurrent);
    assert(!second.complete);
    assert(!once.update(5400, music(0.8f), true, true).scene);
    once.markSceneSettled(6500);
    assert(!once.update(11400, music(0.8f), true, false).scene);
    const ScriptedSceneCue third
        = once.update(11600, music(0.1f), true, false);
    assert(third.scene == NativeSceneKind::PaperHorizon);
    once.markSceneSettled(13000);
    const ScriptedSceneCue completed
        = once.update(20000, music(0.5f, 0.0f), true, false);
    assert(completed.complete);
    assert(!completed.scene);
    assert(!once.update(21000, music(0.1f), true, false).complete);

    ScriptedSceneSequence loop({
        .scenes = {
            NativeSceneKind::GlassChoir,
            NativeSceneKind::ShadowArchitecture,
        },
        .minimumDwellSeconds = 2.0f,
        .maximumDwellSeconds = 3.0f,
    });
    loop.update(100, music(0.5f), true, false);
    assert(loop.update(3100, music(0.5f, 0.0f), true, false).scene
        == NativeSceneKind::ShadowArchitecture);
    loop.markSceneSettled(4000);
    assert(loop.update(7000, music(0.5f, 0.0f), true, false).scene
        == NativeSceneKind::GlassChoir);

    ScriptedSceneSequence inactive({});
    assert(!inactive.active());
    assert(!inactive.update(10000, music(0.1f), true, false).scene);
    std::cout << "scripted scene sequence passed\n";
}
