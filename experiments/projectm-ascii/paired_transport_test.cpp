#include "paired_transport.h"

#include <cassert>
#include <filesystem>
#include <iostream>
#include <limits>
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
        .flashLimitMode = 1,
        .colorVisionSafeMode = 1,
    }));
    const auto first = displayFollower.consume(follower.readDisplay(), 8, 10);
    assert(first && first->nativeScene == 4 && first->asciiMode == 0);
    assert(first->fullscreenMode == 1 && first->syncDelayMs == 74);
    assert(first->intensityPercent == 125 && first->brightnessPercent == 80);
    assert(first->motionPercent == 65 && first->reducedMotionMode == 1);
    assert(first->highContrastMode == 0 && first->directorProfile == 2);
    assert(first->manualSceneCue == 1);
    assert(first->flashLimitMode == 1);
    assert(first->colorVisionSafeMode == 1);
    assert(!displayFollower.consume(follower.readDisplay(), 8, 10));
    const auto* snapshotData = follower.readDisplay().data();
    assert(follower.readDisplay().data() == snapshotData);

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
        .flashLimitMode = 0,
        .colorVisionSafeMode = 0,
    }));
    const auto second = displayFollower.consume(follower.readDisplay(), 8, 10);
    assert(second && second->nativeScene == 7);
    assert(second->nativeSourceScene == 4 && second->asciiMode == 1);
    assert(second->closeMode == 1);
    assert(second->intensityPercent == 75 && second->brightnessPercent == 115);
    assert(second->motionPercent == 35 && second->highContrastMode == 1);
    assert(second->flashLimitMode == 0);
    assert(second->colorVisionSafeMode == 0);
    std::filesystem::remove(leader.statePath());
    assert(follower.readDisplay().empty());
    assert(leader.publishDisplay(*second));
    assert(!follower.readDisplay().empty());

    // Original-preset dissolves must reach the other monitor too.
    auto originalState=*second;
    originalState.serial=3; originalState.presetIndex=5;
    originalState.durationMs=5000; originalState.transitionMode=11;
    originalState.nativeScene=-1; originalState.nativeSourceScene=-1;
    originalState.closeMode=0;
    assert(leader.publishDisplay(originalState));
    const auto original=displayFollower.consume(follower.readDisplay(),8,10);
    assert(original && original->presetIndex==5 && original->transitionMode==11);
    assert(original->durationMs==5000 && original->nativeScene==-1);
    const std::string legacyDisplay = "3 2 1000 7 0 4 2 1 1 35 0\n";
    const auto legacy = decodePairedDisplayState(legacyDisplay, 8, 10);
    assert(legacy && legacy->nativeScene == 4);
    assert(legacy->intensityPercent == -1 && legacy->motionPercent == -1);
    const std::string previousDisplay
        = "4 2 1000 7 0 4 2 1 1 35 0 100 100 100 0 0 0 0 1\n";
    const auto previous = decodePairedDisplayState(previousDisplay, 8, 10);
    assert(previous && previous->flashLimitMode == 1);
    assert(previous->colorVisionSafeMode == -1);
    assert(!decodePairedDisplayState(
        "4 2 1000 7 0 4 2 1 1 35 0 151 100 100 0 0 0\n", 8, 10));
    assert(!decodePairedDisplayState(
        "5 2 1000 7 0 4 2 1 1 35 0 100 100 100 0 0 0 0 2\n",
        8, 10));
    assert(!decodePairedDisplayState(
        "6 2 1000 7 0 4 2 1 1 35 0 100 100 100 0 0 0 0 1 2\n",
        8, 10));

    MusicFrame frame;
    frame.audioTimeSeconds = 9.25;
    frame.bpm = 128.0f;
    frame.kick = 0.82f;
    assert(leader.publishMusic({.serial = 1, .flowTime = 4.75f, .frame = frame}));
    PairedMusicFollower musicFollower;
    const auto music = musicFollower.consume(follower.readMusic());
    assert(music && music->frame.audioTimeSeconds == 9.25);
    assert(music->frame.bpm == 128.0f && music->frame.kick == 0.82f);
    assert(music->flowTime == 4.75f);
    assert(!musicFollower.consume(follower.readMusic()));

    PairedMusicState corrupt{
        .serial = 2,
        .flowTime = 4.8f,
        .frame = frame,
    };
    corrupt.frame.spectrumFlux[7] = std::numeric_limits<float>::quiet_NaN();
    assert(!decodePairedMusicState(encodePairedMusicState(corrupt)));
    corrupt.frame.spectrumFlux[7] = 0.0f;
    corrupt.frame.arrangementConfidence = 1.1f;
    assert(!decodePairedMusicState(encodePairedMusicState(corrupt)));
    corrupt.frame.arrangementConfidence = 0.0f;
    corrupt.frame.arrangementRole = static_cast<ArrangementRole>(255);
    assert(!decodePairedMusicState(encodePairedMusicState(corrupt)));
    corrupt.frame.arrangementRole = ArrangementRole::Unknown;
    corrupt.flowTime = std::numeric_limits<float>::infinity();
    assert(!decodePairedMusicState(encodePairedMusicState(corrupt)));

    corrupt.flowTime = 0.0f;
    corrupt.collectionControls = {.5f, .2f, .3f, .4f, 0, .1f, .2f, .3f};
    assert(decodePairedMusicState(encodePairedMusicState(corrupt)));
    corrupt.collectionControls[3] = std::numeric_limits<float>::quiet_NaN();
    assert(!decodePairedMusicState(encodePairedMusicState(corrupt)));
    corrupt.collectionControls[3] = 1.1f;
    assert(!decodePairedMusicState(encodePairedMusicState(corrupt)));
    corrupt.collectionControls[3] = 0;
    corrupt.collectionResponseEnabled = 2;
    assert(!decodePairedMusicState(encodePairedMusicState(corrupt)));

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
