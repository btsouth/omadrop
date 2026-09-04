#pragma once

#include "music_frame.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <optional>
#include <string>
#include <type_traits>

struct PairedMusicState {
    std::uint32_t magic = 0x4f4d4d46u;
    std::uint32_t version = 4;
    std::uint64_t serial = 0;
    float flowTime = 0.0f;
    MusicFrame frame;
};

static_assert(std::is_trivially_copyable_v<MusicFrame>);
static_assert(std::is_trivially_copyable_v<PairedMusicState>);

inline std::string encodePairedMusicState(const PairedMusicState& state) {
    return std::string(reinterpret_cast<const char*>(&state), sizeof(state));
}

inline std::optional<PairedMusicState> decodePairedMusicState(
    const std::string& input) {
    if (input.size() != sizeof(PairedMusicState)) return std::nullopt;
    PairedMusicState state{};
    std::memcpy(&state, input.data(), sizeof(state));
    const auto finite = [](const auto& values) {
        return std::all_of(values.begin(), values.end(), [](float value) {
            return std::isfinite(value);
        });
    };
    const std::array<float, 27> scalarValues{
        state.flowTime,
        state.frame.kick, state.frame.snare, state.frame.hat,
        state.frame.percussive, state.frame.harmonic,
        state.frame.spectralCentroid, state.frame.stereoWidth,
        state.frame.rhythmicDensity, state.frame.syncopation,
        state.frame.tonalMotion, state.frame.harmonicChange,
        state.frame.presentationDelaySeconds, state.frame.bpm,
        state.frame.beatPhase, state.frame.beatAnticipation,
        state.frame.beatPulse, state.frame.onsetPulse, state.frame.downbeat,
        state.frame.barPhase, state.frame.phrasePhase,
        state.frame.clockConfidence, state.frame.energyFast,
        state.frame.energySlow, state.frame.energySlope,
        state.frame.novelty, state.frame.section,
    };
    if (state.magic != 0x4f4d4d46u || state.version != 4 || state.serial == 0
        || !finite(state.frame.bandLevel) || !finite(state.frame.bandFlux)
        || !finite(state.frame.spectrumLevel)
        || !finite(state.frame.spectrumFlux) || !finite(state.frame.chroma)
        || !finite(scalarValues)
        || !std::isfinite(state.frame.audioTimeSeconds)
        || state.flowTime < 0.0f || state.frame.audioTimeSeconds < 0.0
        || state.frame.presentationDelaySeconds < 0.0f
        || state.frame.bpm < 0.0f || state.frame.bpm > 400.0f
        || state.frame.beatPhase < 0.0f || state.frame.beatPhase > 1.0f
        || state.frame.barPhase < 0.0f || state.frame.barPhase > 1.0f
        || state.frame.phrasePhase < 0.0f || state.frame.phrasePhase > 1.0f) {
        return std::nullopt;
    }
    return state;
}

class PairedMusicFollower {
public:
    std::optional<PairedMusicState> consume(const std::string& input) {
        const auto state = decodePairedMusicState(input);
        if (!state || state->serial <= lastSerial_) return std::nullopt;
        lastSerial_ = state->serial;
        return state;
    }

private:
    std::uint64_t lastSerial_ = 0;
};
