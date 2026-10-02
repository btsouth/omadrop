#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <sstream>
#include <string>

struct PairedDisplayState {
    std::uint64_t serial = 0;
    std::size_t presetIndex = 0;
    std::uint64_t durationMs = 0;
    int transitionMode = 0;
    bool hardSync = false;
    int nativeScene = -1;
    int nativeSourceScene = -1;
    int asciiMode = -1;
    int fullscreenMode = -1;
    int syncDelayMs = -1;
    int closeMode = -1;
    int intensityPercent = -1;
    int brightnessPercent = -1;
    int motionPercent = -1;
    int reducedMotionMode = -1;
    int highContrastMode = -1;
    int directorProfile = -1;
    int manualSceneCue = 0;
    int flashLimitMode = -1;
    int colorVisionSafeMode = -1;
};

inline std::string encodePairedDisplayState(const PairedDisplayState& state) {
    std::ostringstream output;
    output << state.serial << ' ' << state.presetIndex << ' ' << state.durationMs
           << ' ' << state.transitionMode << ' ' << state.hardSync
           << ' ' << state.nativeScene << ' ' << state.nativeSourceScene
           << ' ' << state.asciiMode << ' ' << state.fullscreenMode
           << ' ' << state.syncDelayMs << ' ' << state.closeMode
           << ' ' << state.intensityPercent << ' ' << state.brightnessPercent
           << ' ' << state.motionPercent << ' ' << state.reducedMotionMode
           << ' ' << state.highContrastMode << ' ' << state.directorProfile
           << ' ' << state.manualSceneCue << ' ' << state.flashLimitMode
           << ' ' << state.colorVisionSafeMode
           << '\n';
    return output.str();
}

inline std::optional<PairedDisplayState> decodePairedDisplayState(
    const std::string& input, std::size_t presetCount,
    std::size_t nativeSceneCount = 0) {
    PairedDisplayState state;
    int hardSync = 0;
    std::istringstream stream(input);
    if (!(stream >> state.serial >> state.presetIndex >> state.durationMs
          >> state.transitionMode >> hardSync)
        || state.serial == 0 || state.presetIndex >= presetCount
        || state.transitionMode < 0
        || (state.transitionMode > 3
            && (state.transitionMode < 6 || state.transitionMode > 11))
        || (hardSync != 0 && hardSync != 1)) return std::nullopt;
    state.hardSync = hardSync == 1;
    int nativeScene = -1;
    if (stream >> nativeScene) {
        if (nativeScene < -1
            || (state.transitionMode == 11 && nativeScene >= 0)
            || (nativeScene >= 0
                && (nativeSceneCount == 0
                    || static_cast<std::size_t>(nativeScene) >= nativeSceneCount))) {
            return std::nullopt;
        }
        state.nativeScene = nativeScene;
        int nativeSourceScene = -1;
        if (stream >> nativeSourceScene) {
            if (nativeSourceScene < -1
                || (nativeSourceScene >= 0
                    && (nativeSceneCount == 0
                        || static_cast<std::size_t>(nativeSourceScene)
                           >= nativeSceneCount))) {
                return std::nullopt;
            }
            state.nativeSourceScene = nativeSourceScene;
            int asciiMode = -1;
            int fullscreenMode = -1;
            int syncDelayMs = -1;
            int closeMode = -1;
            if (stream >> asciiMode >> fullscreenMode >> syncDelayMs) {
                stream >> closeMode;
                if ((asciiMode < -1 || asciiMode > 1)
                    || (fullscreenMode < -1 || fullscreenMode > 1)
                    || syncDelayMs < -1 || syncDelayMs > 500
                    || closeMode < -1 || closeMode > 1) {
                    return std::nullopt;
                }
                state.asciiMode = asciiMode;
                state.fullscreenMode = fullscreenMode;
                state.syncDelayMs = syncDelayMs;
                state.closeMode = closeMode;
                int intensityPercent = -1;
                int brightnessPercent = -1;
                int motionPercent = -1;
                int reducedMotionMode = -1;
                int highContrastMode = -1;
                int directorProfile = -1;
                if (stream >> intensityPercent >> brightnessPercent
                           >> motionPercent >> reducedMotionMode
                           >> highContrastMode >> directorProfile) {
                    if (intensityPercent < 50 || intensityPercent > 150
                        || brightnessPercent < 50 || brightnessPercent > 125
                        || motionPercent < 0 || motionPercent > 100
                        || reducedMotionMode < 0 || reducedMotionMode > 1
                        || highContrastMode < 0 || highContrastMode > 1
                        || directorProfile < 0 || directorProfile > 3) {
                        return std::nullopt;
                    }
                    state.intensityPercent = intensityPercent;
                    state.brightnessPercent = brightnessPercent;
                    state.motionPercent = motionPercent;
                    state.reducedMotionMode = reducedMotionMode;
                    state.highContrastMode = highContrastMode;
                    state.directorProfile = directorProfile;
                    int manualSceneCue = 0;
                    if (stream >> manualSceneCue) {
                        if (manualSceneCue < 0 || manualSceneCue > 1) {
                            return std::nullopt;
                        }
                        state.manualSceneCue = manualSceneCue;
                        int flashLimitMode = -1;
                        if (stream >> flashLimitMode) {
                            if (flashLimitMode < -1 || flashLimitMode > 1) {
                                return std::nullopt;
                            }
                            state.flashLimitMode = flashLimitMode;
                            int colorVisionSafeMode = -1;
                            if (stream >> colorVisionSafeMode) {
                                if (colorVisionSafeMode < -1
                                    || colorVisionSafeMode > 1) {
                                    return std::nullopt;
                                }
                                state.colorVisionSafeMode
                                    = colorVisionSafeMode;
                            }
                        }
                    }
                }
            }
        }
    }
    return state;
}

class PairedDisplayFollower {
public:
    std::optional<PairedDisplayState> consume(const std::string& input,
                                              std::size_t presetCount,
                                              std::size_t nativeSceneCount = 0) {
        if (input == lastInput_) return std::nullopt;
        const auto state = decodePairedDisplayState(
            input, presetCount, nativeSceneCount);
        if (!state || state->serial <= lastSerial_) return std::nullopt;
        lastSerial_ = state->serial;
        lastInput_ = input;
        return state;
    }

private:
    std::uint64_t lastSerial_ = 0;
    std::string lastInput_;
};
