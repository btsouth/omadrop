#include "paired_display.h"
#include "paired_music_state.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <limits>
#include <string>

namespace {

constexpr double frameSeconds = 1.0 / 60.0;
constexpr std::uint64_t testFrames = 2u * 60u * 60u * 60u;

float phase(double seconds, double period) {
    return static_cast<float>(std::fmod(seconds, period) / period);
}

float circularDistance(float left, float right) {
    const float direct = std::abs(left - right);
    return std::min(direct, 1.0f - direct);
}

MusicFrame musicAt(double seconds) {
    MusicFrame frame;
    frame.audioTimeSeconds = seconds;
    frame.bpm = 120.0f;
    frame.beatPhase = phase(seconds, 0.5);
    frame.barPhase = phase(seconds, 2.0);
    frame.phrasePhase = phase(seconds, 16.0);
    frame.clockConfidence = 0.92f;
    frame.energyFast = 0.42f;
    frame.energySlow = 0.38f;
    frame.percussive = 0.36f;
    frame.harmonic = 0.48f;
    frame.trackProgress = static_cast<float>(seconds / (2.0 * 60.0 * 60.0));
    frame.arrangementRole = ArrangementRole::Verse;
    frame.arrangementConfidence = 0.82f;
    return frame;
}

} // namespace

int main() {
    PairedMusicFollower musicFollower;
    PairedDisplayFollower displayFollower;
    std::string wireMusic;
    std::string wireDisplay;
    std::uint64_t displaySerial = 0;
    std::uint64_t followerFrames = 0;
    std::uint64_t skippedReads = 0;
    double nextFollowerAt = 0.0;
    double lastFollowerAudioTime = -1.0;
    float lastFollowerFlowTime = 0.0f;
    int followerScene = -1;
    int leaderScene = 0;
    int leaderSourceScene = 0;
    int leaderTransitionMode = 6;
    double flowTime = 0.0;
    double maximumAudioDrift = 0.0;
    double maximumFlowDrift = 0.0;
    double currentSceneMismatch = 0.0;
    double maximumSceneMismatch = 0.0;
    float maximumBeatError = 0.0f;
    float maximumBarError = 0.0f;
    float maximumPhraseError = 0.0f;

    constexpr std::array<double, 6> followerIntervals{
        1.0 / 58.0, 1.0 / 62.0, 1.0 / 59.5,
        1.0 / 61.0, 1.0 / 60.0, 1.0 / 60.5,
    };

    for (std::uint64_t tick = 0; tick < testFrames; ++tick) {
        const double leaderTime = tick * frameSeconds;
        const MusicFrame leaderMusic = musicAt(leaderTime);
        flowTime += frameSeconds
            * (0.006 + 0.18 * std::sqrt(leaderMusic.energySlow));
        wireMusic = encodePairedMusicState({
            .serial = tick + 1,
            .flowTime = static_cast<float>(flowTime),
            .frame = leaderMusic,
        });

        // A scene change is followed by ordinary control snapshots. Every
        // replacement still carries the active source, target, and authored
        // transition, so no scene event can be erased before the next read.
        if (tick % 960u == 0u) {
            leaderSourceScene = leaderScene;
            leaderScene = static_cast<int>((tick / 960u) % 18u);
            leaderTransitionMode = 6 + static_cast<int>((tick / 960u) % 5u);
            wireDisplay = encodePairedDisplayState({
                .serial = ++displaySerial,
                .presetIndex = 0,
                .transitionMode = leaderTransitionMode,
                .hardSync = tick == 0,
                .nativeScene = leaderScene,
                .nativeSourceScene = leaderSourceScene,
                .asciiMode = 1,
                .fullscreenMode = 1,
                .syncDelayMs = 80,
                .closeMode = 0,
                .intensityPercent = 100,
                .brightnessPercent = 100,
                .motionPercent = 65,
                .reducedMotionMode = 0,
                .highContrastMode = 0,
                .directorProfile = 0,
                .flashLimitMode = 1,
                .colorVisionSafeMode = 1,
            });
        } else if (tick % 137u == 0u) {
            wireDisplay = encodePairedDisplayState({
                .serial = ++displaySerial,
                .presetIndex = 0,
                .transitionMode = leaderTransitionMode,
                .nativeScene = leaderScene,
                .nativeSourceScene = leaderSourceScene,
                .asciiMode = static_cast<int>((tick / 137u) % 2u),
                .fullscreenMode = 1,
                .syncDelayMs = 80,
                .closeMode = 0,
                .intensityPercent = 100,
                .brightnessPercent = 100,
                .motionPercent = 65,
                .reducedMotionMode = 0,
                .highContrastMode = 0,
                .directorProfile = 0,
                .flashLimitMode = 1,
                .colorVisionSafeMode = 1,
            });
        }

        while (nextFollowerAt <= leaderTime + 1e-9) {
            const std::uint64_t cycle = followerFrames % 211u;
            const bool readThisFrame = cycle != 17u && cycle != 18u;
            if (readThisFrame) {
                if (const auto synchronized = musicFollower.consume(wireMusic)) {
                    assert(synchronized->frame.audioTimeSeconds
                           > lastFollowerAudioTime);
                    lastFollowerAudioTime
                        = synchronized->frame.audioTimeSeconds;
                    lastFollowerFlowTime = synchronized->flowTime;
                    assert(synchronized->frame.arrangementRole
                           == ArrangementRole::Verse);
                    assert(synchronized->frame.arrangementConfidence == 0.82f);
                }
                if (const auto synchronized = displayFollower.consume(
                        wireDisplay, 1, 18)) {
                    followerScene = synchronized->nativeScene;
                    assert(synchronized->nativeSourceScene >= 0);
                    assert(synchronized->transitionMode >= 6
                           && synchronized->transitionMode <= 10);
                }
            } else {
                ++skippedReads;
            }
            nextFollowerAt += followerIntervals[
                followerFrames % followerIntervals.size()];
            ++followerFrames;
        }

        if (lastFollowerAudioTime >= 0.0) {
            const double audioDrift = leaderTime - lastFollowerAudioTime;
            assert(audioDrift >= -1e-6);
            maximumAudioDrift = std::max(maximumAudioDrift, audioDrift);
            maximumFlowDrift = std::max(maximumFlowDrift,
                flowTime - static_cast<double>(lastFollowerFlowTime));
            const MusicFrame followerMusic = musicAt(lastFollowerAudioTime);
            maximumBeatError = std::max(maximumBeatError,
                circularDistance(leaderMusic.beatPhase, followerMusic.beatPhase));
            maximumBarError = std::max(maximumBarError,
                circularDistance(leaderMusic.barPhase, followerMusic.barPhase));
            maximumPhraseError = std::max(maximumPhraseError,
                circularDistance(leaderMusic.phrasePhase,
                                 followerMusic.phrasePhase));
        }
        if (followerScene != leaderScene) {
            currentSceneMismatch += frameSeconds;
            maximumSceneMismatch = std::max(
                maximumSceneMismatch, currentSceneMismatch);
        } else {
            currentSceneMismatch = 0.0;
        }
    }

    // Two missed follower reads plus the deliberately uneven frame schedule
    // must never become a visible long-term divergence.
    assert(followerFrames > 400000u);
    assert(skippedReads > 3000u);
    assert(maximumAudioDrift <= 0.075);
    assert(maximumSceneMismatch <= 0.075);
    assert(maximumFlowDrift <= 0.012);
    assert(maximumBeatError <= 0.15f);
    assert(maximumBarError <= 0.038f);
    assert(maximumPhraseError <= 0.0048f);

    // Invalid data does not advance the serial. A valid replacement with the
    // same serial must still be accepted on the next poll.
    PairedMusicState corrupt{
        .serial = testFrames + 1,
        .flowTime = static_cast<float>(flowTime),
        .frame = musicAt(testFrames * frameSeconds),
    };
    corrupt.frame.chroma[3] = std::numeric_limits<float>::quiet_NaN();
    assert(!musicFollower.consume(encodePairedMusicState(corrupt)));
    corrupt.frame.chroma[3] = 0.0f;
    assert(musicFollower.consume(encodePairedMusicState(corrupt)));

    // Topology replacement starts both renderer processes again. A new
    // follower accepts the new leader's sequence from one immediately.
    PairedMusicFollower restartedFollower;
    PairedMusicState restarted{
        .serial = 1,
        .flowTime = 0.0f,
        .frame = musicAt(0.0),
    };
    assert(restartedFollower.consume(encodePairedMusicState(restarted)));

    std::cout << "paired sync passed: 2h, max audio drift "
              << maximumAudioDrift * 1000.0 << " ms, scene mismatch "
              << maximumSceneMismatch * 1000.0 << " ms\n";
}
