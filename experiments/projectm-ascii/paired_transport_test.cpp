#include "paired_transport.h"

#include <cassert>
#include <filesystem>
#include <iostream>
#include <string>
#include <unistd.h>

int main() {
    const std::filesystem::path root = std::filesystem::temp_directory_path()
        / ("omadrop-paired-transport-test-" + std::to_string(getpid()));
    std::filesystem::create_directories(root);
    const PairedTransport leader(root / "pair-state");
    const PairedTransport follower(root / "pair-state");
    assert(leader.enabled() && follower.enabled());

    PairedDisplayFollower displayFollower;
    assert(leader.publishDisplay({
        .serial = 1,
        .presetIndex = 3,
        .transitionMode = 10,
        .hardSync = true,
        .nativeScene = 4,
        .nativeSourceScene = 4,
        .asciiMode = 0,
        .fullscreenMode = 1,
        .syncDelayMs = 74,
        .closeMode = 0,
        .intensityPercent = 125,
        .brightnessPercent = 80,
        .motionPercent = 65,
        .reducedMotionMode = 1,
        .highContrastMode = 0,
        .directorProfile = 2,
        .manualSceneCue = 1,
    }));
    const auto first = displayFollower.consume(follower.readDisplay(), 8, 10);
    assert(first && first->nativeScene == 4 && first->asciiMode == 0);
    assert(first->fullscreenMode == 1 && first->syncDelayMs == 74);
    assert(first->intensityPercent == 125 && first->brightnessPercent == 80);
    assert(first->motionPercent == 65 && first->reducedMotionMode == 1);
    assert(first->highContrastMode == 0 && first->directorProfile == 2);
    assert(first->manualSceneCue == 1);
    assert(!displayFollower.consume(follower.readDisplay(), 8, 10));

    assert(leader.publishDisplay({
        .serial = 2,
        .presetIndex = 3,
        .transitionMode = 10,
        .nativeScene = 7,
        .nativeSourceScene = 4,
        .asciiMode = 1,
        .fullscreenMode = 1,
        .syncDelayMs = 74,
        .closeMode = 1,
        .intensityPercent = 75,
        .brightnessPercent = 115,
        .motionPercent = 35,
        .reducedMotionMode = 0,
        .highContrastMode = 1,
        .directorProfile = 3,
    }));
    const auto second = displayFollower.consume(follower.readDisplay(), 8, 10);
    assert(second && second->nativeScene == 7);
    assert(second->nativeSourceScene == 4 && second->asciiMode == 1);
    assert(second->closeMode == 1);
    assert(second->intensityPercent == 75 && second->brightnessPercent == 115);
    assert(second->motionPercent == 35 && second->highContrastMode == 1);

    const std::string legacyDisplay = "3 2 1000 7 0 4 2 1 1 35 0\n";
    const auto legacy = decodePairedDisplayState(legacyDisplay, 8, 10);
    assert(legacy && legacy->nativeScene == 4);
    assert(legacy->intensityPercent == -1 && legacy->motionPercent == -1);
    assert(!decodePairedDisplayState(
        "4 2 1000 7 0 4 2 1 1 35 0 151 100 100 0 0 0\n", 8, 10));

    MusicFrame frame;
    frame.audioTimeSeconds = 9.25;
    frame.bpm = 128.0f;
    frame.kick = 0.82f;
    assert(leader.publishMusic({.serial = 1, .frame = frame}));
    PairedMusicFollower musicFollower;
    const auto music = musicFollower.consume(follower.readMusic());
    assert(music && music->audioTimeSeconds == 9.25);
    assert(music->bpm == 128.0f && music->kick == 0.82f);
    assert(!musicFollower.consume(follower.readMusic()));

    assert(follower.publishRequest("ascii"));
    const auto request = leader.consumeRequest();
    assert(request && *request == "ascii");
    assert(!leader.consumeRequest());
    assert(!std::filesystem::exists(leader.requestPath()));

    for (const auto& entry : std::filesystem::directory_iterator(root)) {
        assert(entry.path().extension() != ".tmp");
    }
    std::error_code error;
    std::filesystem::remove_all(root, error);
    std::cout << "paired transport passed\n";
}
