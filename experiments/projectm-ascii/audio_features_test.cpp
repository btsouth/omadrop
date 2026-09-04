#include "audio_features.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

namespace {
constexpr double tau = 6.28318530717958647692;

struct Counts { int kick = 0; int snare = 0; int hat = 0; };
struct ImpactResult {
    int hits = 0;
    float strongest = 0.0f;
    float peakLevel = 0.0f;
    float peakFlux = 0.0f;
};
struct TempoResult {
    float bpm = 0.0f;
    float confidence = 0.0f;
    int aligned = 0;
    int checked = 0;
    int bars = 0;
};
struct TempoStabilityResult {
    float bpm = 0.0f;
    float confidence = 0.0f;
    float maxStableJump = 0.0f;
    float maxGapError = 0.0f;
    int aligned = 0;
    int checked = 0;
    int reacquireBeats = -1;
};
struct TextureResult {
    float harmonic = 0.0f;
    float percussive = 0.0f;
    float centroid = 0.0f;
    float stereoWidth = 0.0f;
    int dominantChroma = -1;
};
struct MasteringResult {
    Counts counts;
    float meanEnergy = 0.0f;
    float meanKickImpact = 0.0f;
};

MasteringResult runMasteringFixture(float gain, bool compressed) {
    AudioFeatureBus bus;
    std::vector<float> mono(AudioFeatureBus::hopSize, 0.0f);
    double kickPhase = 0.0;
    double snarePhase = 0.0;
    double hatPhase = 0.0;
    double bedPhase = 0.0;
    MasteringResult result;
    int measuredFrames = 0;
    int kickImpacts = 0;
    for (int frame = 0; frame < 720; ++frame) {
        const bool active = frame >= 60;
        const bool kick = active && (frame - 60) % 60 == 0;
        const bool snare = active && (frame - 90) % 60 == 0;
        const bool hat = active && (frame - 60) % 15 == 0;
        for (int i = 0; i < AudioFeatureBus::hopSize; ++i) {
            float sample = active ? 0.022f * static_cast<float>(std::sin(bedPhase)) : 0.0f;
            if (kick) sample += 0.62f * static_cast<float>(std::sin(kickPhase));
            if (snare) sample += 0.24f * static_cast<float>(std::sin(snarePhase));
            if (hat) sample += 0.11f * static_cast<float>(std::sin(hatPhase));
            if (compressed) sample = std::tanh(sample * 3.2f) * 0.42f;
            mono[i] = sample * gain;
            kickPhase += tau * 62.0 / AudioFeatureBus::sampleRate;
            snarePhase += tau * 1800.0 / AudioFeatureBus::sampleRate;
            hatPhase += tau * 7200.0 / AudioFeatureBus::sampleRate;
            bedPhase += tau * 260.0 / AudioFeatureBus::sampleRate;
            if (kickPhase >= tau) kickPhase -= tau;
            if (snarePhase >= tau) snarePhase -= tau;
            if (hatPhase >= tau) hatPhase -= tau;
            if (bedPhase >= tau) bedPhase -= tau;
        }
        const auto& features = bus.processMono(mono.data(), mono.size());
        if (frame < 240) continue;
        result.counts.kick += features.kick;
        result.counts.snare += features.snare;
        result.counts.hat += features.hat;
        if (features.kick) {
            result.meanKickImpact += features.kickImpact;
            ++kickImpacts;
        }
        result.meanEnergy += 0.22f * features.level[0]
                           + 0.22f * features.level[1]
                           + 0.16f * features.level[2]
                           + 0.16f * features.level[3]
                           + 0.14f * features.level[4]
                           + 0.10f * features.level[5];
        ++measuredFrames;
    }
    result.meanEnergy /= std::max(1, measuredFrames);
    result.meanKickImpact /= std::max(1, kickImpacts);
    return result;
}

TextureResult runTexture(float frequency, float sideAmount) {
    AudioFeatureBus bus;
    std::vector<float> stereo(AudioFeatureBus::hopSize * 2, 0.0f);
    double middlePhase = 0.0;
    double sidePhase = 0.0;
    TextureResult result;
    for (int frame = 0; frame < 240; ++frame) {
        for (int i = 0; i < AudioFeatureBus::hopSize; ++i) {
            const float middle = 0.35f * static_cast<float>(std::sin(middlePhase));
            const float side = sideAmount * static_cast<float>(std::sin(sidePhase));
            stereo[i * 2] = middle + side;
            stereo[i * 2 + 1] = middle - side;
            middlePhase += tau * frequency / AudioFeatureBus::sampleRate;
            sidePhase += tau * frequency * 1.37 / AudioFeatureBus::sampleRate;
            if (middlePhase >= tau) middlePhase -= tau;
            if (sidePhase >= tau) sidePhase -= tau;
        }
        const auto& features = bus.processStereo(stereo.data(), AudioFeatureBus::hopSize);
        const auto dominant = std::max_element(
            features.chroma.begin(), features.chroma.end());
        result = {features.harmonicEnergy, features.percussiveEnergy,
                  features.spectralCentroid, features.stereoWidth,
                  static_cast<int>(dominant - features.chroma.begin())};
    }
    return result;
}

Counts runFixture(float frequency) {
    AudioFeatureBus bus;
    std::vector<float> mono(AudioFeatureBus::hopSize, 0.0f);
    Counts counts;
    double phase = 0.0;
    for (int frame = 0; frame < 300; ++frame) {
        const bool event = frame >= 60 && (frame - 60) % 60 == 0;
        for (int i = 0; i < AudioFeatureBus::hopSize; ++i) {
            mono[i] = event ? 0.72f * static_cast<float>(std::sin(phase)) : 0.0f;
            phase += tau * frequency / AudioFeatureBus::sampleRate;
            if (phase >= tau) phase -= tau;
        }
        const auto& features = bus.processMono(mono.data(), mono.size());
        counts.kick += features.kick;
        counts.snare += features.snare;
        counts.hat += features.hat;
    }
    return counts;
}

Counts runNoiseFixture(float amplitude) {
    AudioFeatureBus bus;
    std::vector<float> mono(AudioFeatureBus::hopSize, 0.0f);
    Counts counts;
    std::uint32_t state = 0x7f4a7c15u;
    for (int frame = 0; frame < 600; ++frame) {
        for (float& sample : mono) {
            state ^= state << 13;
            state ^= state >> 17;
            state ^= state << 5;
            sample = amplitude * (static_cast<float>(state & 0xffffu) / 32767.5f - 1.0f);
        }
        const auto& features = bus.processMono(mono.data(), mono.size());
        if (frame >= 120) {
            counts.kick += features.kick;
            counts.snare += features.snare;
            counts.hat += features.hat;
        }
    }
    return counts;
}

ImpactResult runMixedKickFixture(float kickAmplitude) {
    AudioFeatureBus bus;
    std::vector<float> mono(AudioFeatureBus::hopSize, 0.0f);
    ImpactResult result;
    double kickPhase = 0.0;
    double lowBedPhase = 0.0;
    double midBedPhase = 0.0;
    for (int frame = 0; frame < 360; ++frame) {
        const bool event = frame >= 60 && (frame - 60) % 60 == 0;
        for (int i = 0; i < AudioFeatureBus::hopSize; ++i) {
            const float bed = 0.035f * static_cast<float>(std::sin(lowBedPhase))
                            + 0.025f * static_cast<float>(std::sin(midBedPhase));
            mono[i] = bed + (event ? kickAmplitude
                * static_cast<float>(std::sin(kickPhase)) : 0.0f);
            kickPhase += tau * 62.0 / AudioFeatureBus::sampleRate;
            lowBedPhase += tau * 110.0 / AudioFeatureBus::sampleRate;
            midBedPhase += tau * 1800.0 / AudioFeatureBus::sampleRate;
            if (kickPhase >= tau) kickPhase -= tau;
            if (lowBedPhase >= tau) lowBedPhase -= tau;
            if (midBedPhase >= tau) midBedPhase -= tau;
        }
        const auto& features = bus.processMono(mono.data(), mono.size());
        if (frame >= 120 && features.kick) {
            ++result.hits;
            result.strongest = std::max(result.strongest, features.kickImpact);
            result.peakLevel = std::max(result.peakLevel,
                                        std::max(features.level[0], features.level[1]));
            result.peakFlux = std::max(result.peakFlux,
                                       std::max(features.flux[0], features.flux[1]));
        }
    }
    return result;
}

TempoResult runTempo(float bpm) {
    AudioFeatureBus bus;
    std::vector<float> mono(AudioFeatureBus::hopSize, 0.0f);
    TempoResult result;
    double tonePhase = 0.0;
    double beatPhase = 0.0;
    for (int frame = 0; frame < 720; ++frame) {
        beatPhase += bpm / 3600.0;
        const bool event = beatPhase >= 1.0;
        if (event) beatPhase -= 1.0;
        for (int i = 0; i < AudioFeatureBus::hopSize; ++i) {
            mono[i] = event ? 0.72f * static_cast<float>(std::sin(tonePhase)) : 0.0f;
            tonePhase += tau * 62.0 / AudioFeatureBus::sampleRate;
            if (tonePhase >= tau) tonePhase -= tau;
        }
        const auto& features = bus.processMono(mono.data(), mono.size());
        if (frame >= 480 && features.beatConfidence >= 0.30f && features.barCrossed) {
            ++result.bars;
        }
        if (event && frame >= 480 && features.beatConfidence >= 0.30f) {
            const float distance = std::min(features.beatPhase, 1.0f - features.beatPhase);
            result.aligned += distance <= 0.15f;
            ++result.checked;
        }
        result.bpm = features.bpm;
        result.confidence = features.beatConfidence;
    }
    return result;
}

TempoResult runMixedTempo120() {
    AudioFeatureBus bus;
    std::vector<float> mono(AudioFeatureBus::hopSize, 0.0f);
    TempoResult result;
    double kickPhase = 0.0;
    double snarePhase = 0.0;
    double hatPhase = 0.0;
    double bedPhase = 0.0;
    for (int frame = 0; frame < 720; ++frame) {
        const bool tick = frame >= 60 && (frame - 60) % 15 == 0;
        const int tickIndex = frame >= 60 ? (frame - 60) / 15 : 0;
        const bool beat = tick && tickIndex % 2 == 0;
        const bool snare = beat && (tickIndex / 2) % 4 % 2 == 1;
        for (int i = 0; i < AudioFeatureBus::hopSize; ++i) {
            float sample = 0.025f * static_cast<float>(std::sin(bedPhase));
            if (beat) sample += 0.58f * static_cast<float>(std::sin(kickPhase));
            if (snare) sample += 0.26f * static_cast<float>(std::sin(snarePhase));
            if (tick) sample += 0.12f * static_cast<float>(std::sin(hatPhase));
            mono[i] = sample;
            kickPhase += tau * 62.0 / AudioFeatureBus::sampleRate;
            snarePhase += tau * 2100.0 / AudioFeatureBus::sampleRate;
            hatPhase += tau * 7000.0 / AudioFeatureBus::sampleRate;
            bedPhase += tau * 240.0 / AudioFeatureBus::sampleRate;
            if (kickPhase >= tau) kickPhase -= tau;
            if (snarePhase >= tau) snarePhase -= tau;
            if (hatPhase >= tau) hatPhase -= tau;
            if (bedPhase >= tau) bedPhase -= tau;
        }
        const auto& features = bus.processMono(mono.data(), mono.size());
        if (frame >= 480 && features.beatConfidence >= 0.30f && features.barCrossed) {
            ++result.bars;
        }
        if (beat && frame >= 480 && features.beatConfidence >= 0.30f) {
            const float distance = std::min(features.beatPhase, 1.0f - features.beatPhase);
            result.aligned += distance <= 0.15f;
            ++result.checked;
        }
        result.bpm = features.bpm;
        result.confidence = features.beatConfidence;
    }
    return result;
}

TempoStabilityResult runHalfDoubleTempo() {
    AudioFeatureBus bus;
    std::vector<float> mono(AudioFeatureBus::hopSize, 0.0f);
    TempoStabilityResult result;
    double kickPhase = 0.0;
    double hatPhase = 0.0;
    double tickPhase = 0.0;
    int tickIndex = 0;
    float previousBpm = 0.0f;
    for (int frame = 0; frame < 960; ++frame) {
        tickPhase += 140.0 / 3600.0;
        const bool tick = frame >= 60 && tickPhase >= 1.0;
        if (tick) {
            tickPhase -= 1.0;
            ++tickIndex;
        }
        const bool lowAccent = tick && tickIndex % 2 == 0;
        for (int i = 0; i < AudioFeatureBus::hopSize; ++i) {
            float sample = 0.0f;
            if (lowAccent) sample += 0.66f * static_cast<float>(std::sin(kickPhase));
            if (tick) sample += 0.13f * static_cast<float>(std::sin(hatPhase));
            mono[i] = sample;
            kickPhase += tau * 62.0 / AudioFeatureBus::sampleRate;
            hatPhase += tau * 7200.0 / AudioFeatureBus::sampleRate;
            if (kickPhase >= tau) kickPhase -= tau;
            if (hatPhase >= tau) hatPhase -= tau;
        }
        const auto& features = bus.processMono(mono.data(), mono.size());
        if (frame >= 600 && features.beatConfidence >= 0.30f) {
            if (previousBpm > 0.0f) {
                result.maxStableJump = std::max(
                    result.maxStableJump, std::abs(features.bpm - previousBpm));
            }
            previousBpm = features.bpm;
            if (tick) {
                const float distance = std::min(
                    features.beatPhase, 1.0f - features.beatPhase);
                result.aligned += distance <= 0.15f;
                ++result.checked;
            }
        }
        result.bpm = features.bpm;
        result.confidence = features.beatConfidence;
    }
    return result;
}

TempoStabilityResult runBreakdownTempo() {
    AudioFeatureBus bus;
    std::vector<float> mono(AudioFeatureBus::hopSize, 0.0f);
    TempoStabilityResult result;
    double tonePhase = 0.0;
    int postGapBeats = 0;
    for (int frame = 0; frame < 1200; ++frame) {
        const bool beat = frame >= 60 && (frame - 60) % 30 == 0;
        const bool inGap = frame >= 540 && frame < 780;
        const bool event = beat && !inGap;
        for (int i = 0; i < AudioFeatureBus::hopSize; ++i) {
            mono[i] = event ? 0.72f * static_cast<float>(std::sin(tonePhase)) : 0.0f;
            tonePhase += tau * 62.0 / AudioFeatureBus::sampleRate;
            if (tonePhase >= tau) tonePhase -= tau;
        }
        const auto& features = bus.processMono(mono.data(), mono.size());
        if (inGap) {
            result.maxGapError = std::max(
                result.maxGapError, std::abs(features.bpm - 120.0f));
        }
        if (event && frame >= 780) {
            const float distance = std::min(
                features.beatPhase, 1.0f - features.beatPhase);
            if (features.beatConfidence >= 0.30f && distance <= 0.15f) {
                ++result.aligned;
                if (result.reacquireBeats < 0) result.reacquireBeats = postGapBeats;
            }
            ++result.checked;
            ++postGapBeats;
        }
        result.bpm = features.bpm;
        result.confidence = features.beatConfidence;
    }
    return result;
}

TempoStabilityResult runSyncopatedTempo() {
    AudioFeatureBus bus;
    std::vector<float> mono(AudioFeatureBus::hopSize, 0.0f);
    TempoStabilityResult result;
    double kickPhase = 0.0;
    double snarePhase = 0.0;
    double hatPhase = 0.0;
    float previousBpm = 0.0f;
    for (int frame = 0; frame < 960; ++frame) {
        const int relative = frame - 60;
        const bool beat = relative >= 0 && relative % 30 == 0;
        const bool syncopation = relative >= 0 && relative % 30 == 20;
        const bool snare = beat && (relative / 30) % 4 % 2 == 1;
        for (int i = 0; i < AudioFeatureBus::hopSize; ++i) {
            float sample = 0.0f;
            if (beat) sample += 0.62f * static_cast<float>(std::sin(kickPhase));
            if (snare) sample += 0.25f * static_cast<float>(std::sin(snarePhase));
            if (syncopation) sample += 0.18f * static_cast<float>(std::sin(hatPhase));
            mono[i] = sample;
            kickPhase += tau * 62.0 / AudioFeatureBus::sampleRate;
            snarePhase += tau * 2100.0 / AudioFeatureBus::sampleRate;
            hatPhase += tau * 7000.0 / AudioFeatureBus::sampleRate;
            if (kickPhase >= tau) kickPhase -= tau;
            if (snarePhase >= tau) snarePhase -= tau;
            if (hatPhase >= tau) hatPhase -= tau;
        }
        const auto& features = bus.processMono(mono.data(), mono.size());
        if (frame >= 600 && features.beatConfidence >= 0.30f) {
            if (previousBpm > 0.0f) {
                result.maxStableJump = std::max(
                    result.maxStableJump, std::abs(features.bpm - previousBpm));
            }
            previousBpm = features.bpm;
            if (beat) {
                const float distance = std::min(
                    features.beatPhase, 1.0f - features.beatPhase);
                result.aligned += distance <= 0.15f;
                ++result.checked;
            }
        }
        result.bpm = features.bpm;
        result.confidence = features.beatConfidence;
    }
    return result;
}

bool expect(const std::string& name, int actual, int minimum, int maximum) {
    if (actual >= minimum && actual <= maximum) return true;
    std::cerr << name << ": expected " << minimum << ".." << maximum
              << ", got " << actual << "\n";
    return false;
}
} // namespace

int main() {
    AudioFeatureBus timestampBus;
    std::vector<float> timestampSilence(AudioFeatureBus::hopSize, 0.0f);
    const double firstTimestamp = timestampBus.processMono(
        timestampSilence.data(), timestampSilence.size()).audioTimeSeconds;
    const double secondTimestamp = timestampBus.processMono(
        timestampSilence.data(), timestampSilence.size()).audioTimeSeconds;
    std::vector<float> resetTone(AudioFeatureBus::hopSize, 0.0f);
    double resetPhase = 0.0;
    AudioFeatures beforeReset;
    for (int frame = 0; frame < 20; ++frame) {
        for (float& sample : resetTone) {
            sample = 0.75f * static_cast<float>(std::sin(resetPhase));
            resetPhase += tau * 72.0 / AudioFeatureBus::sampleRate;
            if (resetPhase >= tau) resetPhase -= tau;
        }
        beforeReset = timestampBus.processMono(
            resetTone.data(), resetTone.size());
    }
    timestampBus.resetAnalysis();
    const AudioFeatures afterReset = timestampBus.processMono(
        timestampSilence.data(), timestampSilence.size());
    const Counts silence = runFixture(0.0f);
    const Counts kick = runFixture(62.0f);
    const Counts snare = runFixture(2200.0f);
    const Counts hat = runFixture(7000.0f);
    const Counts noise = runNoiseFixture(0.0005f);
    const ImpactResult softKick = runMixedKickFixture(0.16f);
    const ImpactResult mediumKick = runMixedKickFixture(0.34f);
    const ImpactResult hardKick = runMixedKickFixture(0.72f);
    const TempoResult tempo90 = runTempo(90.0f);
    const TempoResult tempo120 = runTempo(120.0f);
    const TempoResult tempo140 = runTempo(140.0f);
    const TempoResult tempo174 = runTempo(174.0f);
    const TempoResult mixedTempo120 = runMixedTempo120();
    const TempoStabilityResult halfDoubleTempo = runHalfDoubleTempo();
    const TempoStabilityResult breakdownTempo = runBreakdownTempo();
    const TempoStabilityResult syncopatedTempo = runSyncopatedTempo();
    const TextureResult lowTexture = runTexture(110.0f, 0.0f);
    const TextureResult highTexture = runTexture(4800.0f, 0.0f);
    const TextureResult wideTexture = runTexture(440.0f, 0.24f);
    const TextureResult aChroma = runTexture(440.0f, 0.0f);
    const TextureResult cChroma = runTexture(261.626f, 0.0f);
    const MasteringResult quietMaster = runMasteringFixture(0.03f, false);
    const MasteringResult referenceMaster = runMasteringFixture(0.35f, false);
    const MasteringResult loudMaster = runMasteringFixture(0.90f, false);
    const MasteringResult compressedMaster = runMasteringFixture(0.90f, true);

    bool ok = true;
    const double expectedHopSeconds = AudioFeatureBus::hopSize
                                    / static_cast<double>(AudioFeatureBus::sampleRate);
    if (std::abs((secondTimestamp - firstTimestamp) - expectedHopSeconds) > 1e-9) {
        std::cerr << "audio timestamps did not advance by one hop\n";
        ok = false;
    }
    if (afterReset.audioTimeSeconds <= beforeReset.audioTimeSeconds
        || afterReset.kick || afterReset.snare || afterReset.hat
        || afterReset.beatConfidence != 0.0f
        || *std::max_element(afterReset.level.begin(), afterReset.level.end())
            > 0.001f
        || *std::max_element(afterReset.flux.begin(), afterReset.flux.end())
            > 0.001f) {
        std::cerr << "analysis reset retained pre-resume signal state\n";
        ok = false;
    }
    ok &= expect("silence kick", silence.kick, 0, 0);
    ok &= expect("silence snare", silence.snare, 0, 0);
    ok &= expect("silence hat", silence.hat, 0, 0);
    ok &= expect("62 Hz kick", kick.kick, 3, 4);
    ok &= expect("62 Hz snare leakage", kick.snare, 0, 1);
    ok &= expect("62 Hz hat leakage", kick.hat, 0, 0);
    ok &= expect("2.2 kHz snare", snare.snare, 3, 4);
    ok &= expect("2.2 kHz kick leakage", snare.kick, 0, 0);
    ok &= expect("7 kHz hat", hat.hat, 3, 4);
    ok &= expect("7 kHz kick leakage", hat.kick, 0, 0);
    ok &= expect("noise kick", noise.kick, 0, 0);
    ok &= expect("noise snare", noise.snare, 0, 0);
    ok &= expect("noise hat", noise.hat, 0, 0);
    if (softKick.hits == 0 || mediumKick.hits == 0 || hardKick.hits == 0) {
        std::cerr << "mixed kick fixture: missing detections "
                  << softKick.hits << "," << mediumKick.hits << "," << hardKick.hits << "\n";
        ok = false;
    }
    if (!(softKick.strongest + 0.04f < mediumKick.strongest
          && mediumKick.strongest + 0.04f < hardKick.strongest)) {
        std::cerr << "mixed kick strength: expected soft < medium < hard, got "
                  << softKick.strongest << "," << mediumKick.strongest << ","
                  << hardKick.strongest << " levels=" << softKick.peakLevel << ","
                  << mediumKick.peakLevel << "," << hardKick.peakLevel
                  << " flux=" << softKick.peakFlux << "," << mediumKick.peakFlux
                  << "," << hardKick.peakFlux << "\n";
        ok = false;
    }
    for (const auto [expected, result] : {
             std::pair{90.0f, tempo90}, std::pair{120.0f, tempo120},
             std::pair{140.0f, tempo140}, std::pair{174.0f, tempo174}}) {
        if (std::abs(result.bpm - expected) > 4.0f || result.confidence < 0.30f) {
            std::cerr << "tempo " << expected << ": got " << result.bpm
                      << " confidence " << result.confidence << "\n";
            ok = false;
        }
        if (result.checked == 0 || result.aligned * 4 < result.checked * 3) {
            std::cerr << "phase " << expected << ": aligned " << result.aligned
                      << "/" << result.checked << "\n";
            ok = false;
        }
        if (result.bars == 0) {
            std::cerr << "bar clock " << expected << ": no bar crossings\n";
            ok = false;
        }
    }
    if (std::abs(mixedTempo120.bpm - 120.0f) > 4.0f
        || mixedTempo120.confidence < 0.30f
        || mixedTempo120.checked == 0
        || mixedTempo120.aligned * 4 < mixedTempo120.checked * 3
        || mixedTempo120.bars == 0) {
        std::cerr << "mixed tempo 120: bpm=" << mixedTempo120.bpm
                  << " confidence=" << mixedTempo120.confidence
                  << " aligned=" << mixedTempo120.aligned << "/" << mixedTempo120.checked
                  << " bars=" << mixedTempo120.bars << "\n";
        ok = false;
    }
    if (std::abs(halfDoubleTempo.bpm - 140.0f) > 4.0f
        || halfDoubleTempo.confidence < 0.30f
        || halfDoubleTempo.maxStableJump > 4.0f
        || halfDoubleTempo.checked == 0
        || halfDoubleTempo.aligned * 4 < halfDoubleTempo.checked * 3) {
        std::cerr << "half/double tempo: bpm=" << halfDoubleTempo.bpm
                  << " confidence=" << halfDoubleTempo.confidence
                  << " jump=" << halfDoubleTempo.maxStableJump
                  << " aligned=" << halfDoubleTempo.aligned << "/"
                  << halfDoubleTempo.checked << "\n";
        ok = false;
    }
    if (std::abs(breakdownTempo.bpm - 120.0f) > 4.0f
        || breakdownTempo.confidence < 0.30f
        || breakdownTempo.maxGapError > 4.0f
        || breakdownTempo.reacquireBeats < 0
        || breakdownTempo.reacquireBeats > 2
        || breakdownTempo.checked == 0
        || breakdownTempo.aligned * 4 < breakdownTempo.checked * 3) {
        std::cerr << "breakdown tempo: bpm=" << breakdownTempo.bpm
                  << " confidence=" << breakdownTempo.confidence
                  << " gap_error=" << breakdownTempo.maxGapError
                  << " reacquire_beats=" << breakdownTempo.reacquireBeats
                  << " aligned=" << breakdownTempo.aligned << "/"
                  << breakdownTempo.checked << "\n";
        ok = false;
    }
    if (std::abs(syncopatedTempo.bpm - 120.0f) > 4.0f
        || syncopatedTempo.confidence < 0.30f
        || syncopatedTempo.maxStableJump > 4.0f
        || syncopatedTempo.checked == 0
        || syncopatedTempo.aligned * 4 < syncopatedTempo.checked * 3) {
        std::cerr << "syncopated tempo: bpm=" << syncopatedTempo.bpm
                  << " confidence=" << syncopatedTempo.confidence
                  << " jump=" << syncopatedTempo.maxStableJump
                  << " aligned=" << syncopatedTempo.aligned << "/"
                  << syncopatedTempo.checked << "\n";
        ok = false;
    }
    if (lowTexture.harmonic < 0.45f || highTexture.harmonic < 0.45f) {
        std::cerr << "steady tones should be harmonic: "
                  << lowTexture.harmonic << "," << highTexture.harmonic << "\n";
        ok = false;
    }
    if (highTexture.centroid < lowTexture.centroid + 0.35f) {
        std::cerr << "spectral centroid did not separate low and high tones: "
                  << lowTexture.centroid << "," << highTexture.centroid << "\n";
        ok = false;
    }
    if (lowTexture.stereoWidth > 0.01f || wideTexture.stereoWidth < 0.25f) {
        std::cerr << "stereo width did not separate mono and wide fixtures: "
                  << lowTexture.stereoWidth << "," << wideTexture.stereoWidth << "\n";
        ok = false;
    }
    if (aChroma.dominantChroma != 9 || cChroma.dominantChroma != 0) {
        std::cerr << "chroma did not identify A and C: "
                  << aChroma.dominantChroma << "," << cChroma.dominantChroma
                  << "\n";
        ok = false;
    }

    const auto sameCounts = [](const Counts& left, const Counts& right) {
        return left.kick == right.kick && left.snare == right.snare
            && left.hat == right.hat;
    };
    if (!sameCounts(quietMaster.counts, referenceMaster.counts)
        || !sameCounts(loudMaster.counts, referenceMaster.counts)
        || !sameCounts(compressedMaster.counts, referenceMaster.counts)) {
        std::cerr << "mastering normalization changed transient counts\n";
        ok = false;
    }
    const std::array<float, 4> masteringEnergy{
        quietMaster.meanEnergy, referenceMaster.meanEnergy,
        loudMaster.meanEnergy, compressedMaster.meanEnergy};
    const auto [minimumEnergy, maximumEnergy] = std::minmax_element(
        masteringEnergy.begin(), masteringEnergy.end());
    if (*minimumEnergy <= 0.0f || *maximumEnergy > *minimumEnergy * 1.60f) {
        std::cerr << "mastering normalization energy ratio was "
                  << *maximumEnergy / std::max(1e-6f, *minimumEnergy) << "\n";
        ok = false;
    }
    const std::array<float, 4> kickImpacts{
        quietMaster.meanKickImpact, referenceMaster.meanKickImpact,
        loudMaster.meanKickImpact, compressedMaster.meanKickImpact};
    const auto [minimumImpact, maximumImpact] = std::minmax_element(
        kickImpacts.begin(), kickImpacts.end());
    if (*maximumImpact - *minimumImpact > 0.15f) {
        std::cerr << "mastering normalization impact spread was "
                  << *maximumImpact - *minimumImpact << "\n";
        ok = false;
    }

    std::cout << "mastering quiet=" << quietMaster.counts.kick << "/"
              << quietMaster.counts.snare << "/" << quietMaster.counts.hat
              << " reference=" << referenceMaster.counts.kick << "/"
              << referenceMaster.counts.snare << "/" << referenceMaster.counts.hat
              << " loud=" << loudMaster.counts.kick << "/"
              << loudMaster.counts.snare << "/" << loudMaster.counts.hat
              << " compressed=" << compressedMaster.counts.kick << "/"
              << compressedMaster.counts.snare << "/"
              << compressedMaster.counts.hat << " energy="
              << quietMaster.meanEnergy << "/" << referenceMaster.meanEnergy
              << "/" << loudMaster.meanEnergy << "/"
              << compressedMaster.meanEnergy << " kick_impact="
              << quietMaster.meanKickImpact << "/"
              << referenceMaster.meanKickImpact << "/"
              << loudMaster.meanKickImpact << "/"
              << compressedMaster.meanKickImpact << "\n";

    if (!ok) return 1;
    std::cout << "audio fixtures passed"
              << " | kick=" << kick.kick
              << " snare=" << snare.snare
              << " hat=" << hat.hat
              << " impact=" << softKick.strongest << ","
              << mediumKick.strongest << "," << hardKick.strongest
              << " tempos=" << tempo90.bpm << "," << tempo120.bpm << ","
              << tempo140.bpm << "," << tempo174.bpm
              << " ambiguity=" << halfDoubleTempo.bpm
              << " breakdown=" << breakdownTempo.bpm << "/"
              << breakdownTempo.reacquireBeats
              << " syncopated=" << syncopatedTempo.bpm << "\n";
    return 0;
}
