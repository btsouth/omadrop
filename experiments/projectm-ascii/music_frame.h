#pragma once

#include "arrangement_classifier.h"
#include "audio_features.h"
#include "musical_structure.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>

// The stable contract between musical analysis and every visual backend.
// Renderers should not inspect analyzer internals or invent their own smoothing.
struct MusicFrame {
    std::array<float, AudioFeatures::roleCount> bandLevel{};
    std::array<float, AudioFeatures::roleCount> bandFlux{};
    std::array<float, AudioFeatures::spectrumCount> spectrumLevel{};
    std::array<float, AudioFeatures::spectrumCount> spectrumFlux{};
    std::array<float, AudioFeatures::chromaCount> chroma{};
    float kick = 0.0f;
    float snare = 0.0f;
    float hat = 0.0f;
    float percussive = 0.0f;
    float harmonic = 0.0f;
    float spectralCentroid = 0.0f;
    float stereoWidth = 0.0f;
    float rhythmicDensity = 0.0f;
    float syncopation = 0.0f;
    float tonalMotion = 0.0f;
    float harmonicChange = 0.0f;
    double audioTimeSeconds = 0.0;
    float presentationDelaySeconds = 0.0f;
    float bpm = 120.0f;
    float beatPhase = 0.0f;
    float beatAnticipation = 0.0f;
    float beatPulse = 0.0f;
    float onsetPulse = 0.0f;
    float downbeat = 0.0f;
    float barPhase = 0.0f;
    float phrasePhase = 0.0f;
    float clockConfidence = 0.0f;
    float energyFast = 0.0f;
    float energySlow = 0.0f;
    float energySlope = 0.0f;
    float novelty = 0.0f;
    float section = 0.0f;
    float trackProgress = -1.0f;
    float arrangementConfidence = 0.0f;
    int motifIdentity = -1;
    ArrangementRole arrangementRole = ArrangementRole::Unknown;
    bool arrangementChanged = false;
};

class MusicFrameBuilder {
public:
    const MusicFrame& update(const AudioFeatures& features,
                             const MusicalStructureState& structure,
                             float seconds, float presentationDelaySeconds = 0.0f,
                             float trackProgress = -1.0f) {
        const float dt = std::clamp(seconds, 1.0f / 240.0f, 0.1f);
        frame_.bandLevel = features.level;
        frame_.bandFlux = features.flux;
        frame_.spectrumLevel = features.spectrumLevel;
        frame_.spectrumFlux = features.spectrumFlux;
        frame_.chroma = features.chroma;
        // Analyzer impacts intentionally linger for classification and tempo
        // work. Visual gestures need a separate, much shorter envelope or a
        // busy recording reads as one continuous kick/snare state. Trigger on
        // the discrete event, then return toward baseline before the next hit.
        if (features.kick) kickEnvelope_ = std::max(
            kickEnvelope_, features.kickImpact);
        else kickEnvelope_ *= std::exp(-16.0f * dt);
        if (features.snare) snareEnvelope_ = std::max(
            snareEnvelope_, features.snareImpact);
        else snareEnvelope_ *= std::exp(-20.0f * dt);
        if (features.hat) hatEnvelope_ = std::max(
            hatEnvelope_, features.hatImpact);
        else hatEnvelope_ *= std::exp(-30.0f * dt);
        frame_.kick = kickEnvelope_;
        frame_.snare = snareEnvelope_;
        frame_.hat = hatEnvelope_;
        frame_.bpm = features.bpm;
        frame_.beatPhase = features.beatPhase;
        frame_.barPhase = features.barPhase;
        frame_.phrasePhase = features.phrasePhase;
        frame_.clockConfidence = features.beatConfidence;
        frame_.novelty = structure.novelty;
        frame_.motifIdentity = structure.motifIdentity;
        frame_.spectralCentroid = features.spectralCentroid;
        frame_.stereoWidth = features.stereoWidth;
        frame_.audioTimeSeconds = features.audioTimeSeconds;
        frame_.presentationDelaySeconds = std::max(0.0f, presentationDelaySeconds);
        frame_.trackProgress = trackProgress >= 0.0f
            ? std::clamp(trackProgress, 0.0f, 1.0f) : -1.0f;

        const float anticipationPosition = std::clamp(
            (features.beatPhase - 0.55f) / 0.45f, 0.0f, 1.0f);
        frame_.beatAnticipation = anticipationPosition * anticipationPosition
                                * (3.0f - 2.0f * anticipationPosition)
                                * features.beatConfidence;

        const bool musicActive = *std::max_element(
            features.level.begin(), features.level.end()) > 0.08f;
        densityAccumulator_ *= std::exp(-1.4f * dt);
        if (features.kick) densityAccumulator_ += 0.12f;
        if (features.snare) densityAccumulator_ += 0.10f;
        if (features.hat) densityAccumulator_ += 0.05f;
        frame_.rhythmicDensity = smooth(
            frame_.rhythmicDensity,
            musicActive ? std::clamp(densityAccumulator_, 0.0f, 1.0f) : 0.0f,
            3.0f, dt);

        syncopationSum_ *= std::exp(-0.7f * dt);
        syncopationWeight_ *= std::exp(-0.7f * dt);
        const float eventWeight = (features.kick ? 1.0f : 0.0f)
                                + (features.snare ? 0.8f : 0.0f)
                                + (features.hat ? 0.35f : 0.0f);
        if (eventWeight > 0.0f && features.beatConfidence > 0.25f) {
            const float distance = std::min(
                features.beatPhase, 1.0f - features.beatPhase);
            syncopationSum_ += eventWeight * std::clamp(distance * 2.0f, 0.0f, 1.0f);
            syncopationWeight_ += eventWeight;
        }
        const float syncopationTarget = musicActive && syncopationWeight_ > 1e-4f
            ? syncopationSum_ / syncopationWeight_ : 0.0f;
        frame_.syncopation = smooth(
            frame_.syncopation, syncopationTarget, 2.0f, dt);

        float tonalDifference = 0.0f;
        if (chromaInitialized_) {
            float dot = 0.0f;
            float currentPower = 0.0f;
            float baselinePower = 0.0f;
            for (std::size_t index = 0; index < features.chroma.size(); ++index) {
                dot += features.chroma[index] * chromaBaseline_[index];
                currentPower += features.chroma[index] * features.chroma[index];
                baselinePower += chromaBaseline_[index] * chromaBaseline_[index];
            }
            const float similarity = dot / std::sqrt(
                std::max(1e-8f, currentPower * baselinePower));
            tonalDifference = std::clamp((1.0f - similarity) * 2.4f, 0.0f, 1.0f)
                            * std::clamp(features.harmonicEnergy * 1.5f, 0.0f, 1.0f);
        } else {
            chromaBaseline_ = features.chroma;
            chromaInitialized_ = true;
        }
        const float chromaBlend = 1.0f - std::exp(-1.4f * dt);
        for (std::size_t index = 0; index < features.chroma.size(); ++index) {
            chromaBaseline_[index] += (features.chroma[index] - chromaBaseline_[index])
                                    * chromaBlend;
        }
        frame_.tonalMotion = smooth(
            frame_.tonalMotion, tonalDifference,
            tonalDifference > frame_.tonalMotion ? 7.0f : 2.0f, dt);
        if (harmonicChangeCooldown_ == 0 && tonalDifference > 0.18f
            && tonalDifference > previousTonalDifference_ + 0.055f) {
            harmonicChangeEnvelope_ = std::max(
                harmonicChangeEnvelope_, tonalDifference);
            harmonicChangeCooldown_ = 12;
        }
        if (harmonicChangeCooldown_ > 0) --harmonicChangeCooldown_;
        frame_.harmonicChange = harmonicChangeEnvelope_;
        harmonicChangeEnvelope_ *= std::exp(-6.0f * dt);
        previousTonalDifference_ = tonalDifference;

        float strongestOnset = 0.0f;
        float broadOnset = 0.0f;
        for (float flux : features.flux) {
            const float onset = std::log1p(std::max(0.0f, flux - 0.9f));
            strongestOnset = std::max(strongestOnset, onset);
            broadOnset += onset / AudioFeatures::roleCount;
        }
        const float onsetCandidate = std::clamp(
            (0.65f * strongestOnset + 0.80f * broadOnset - 0.90f) / 1.40f,
            0.0f, 1.0f);
        // Flux remains elevated across the body of many notes. Emit only a
        // sharp rising edge, then enforce a 100 ms refractory period so this
        // signal describes attacks instead of becoming another energy meter.
        if (musicActive && onsetCooldown_ == 0
            && onsetCandidate > 0.10f
            && onsetCandidate > previousOnsetCandidate_ + 0.25f) {
            onsetEnvelope_ = std::max(onsetEnvelope_, onsetCandidate);
            onsetCooldown_ = 6;
        } else if (!musicActive) {
            onsetEnvelope_ = 0.0f;
            onsetCooldown_ = 0;
        }
        if (onsetCooldown_ > 0) --onsetCooldown_;
        previousOnsetCandidate_ = musicActive ? onsetCandidate : 0.0f;
        if (features.beatCrossed && musicActive) beatEnvelope_ = 1.0f;
        if (features.barCrossed && musicActive) downbeatEnvelope_ = 1.0f;
        if (structure.sectionCrossed) sectionEnvelope_ = 1.0f;
        beatEnvelope_ *= std::exp(-9.0f * dt);
        downbeatEnvelope_ *= std::exp(-5.0f * dt);
        sectionEnvelope_ *= std::exp(-1.8f * dt);
        frame_.beatPulse = beatEnvelope_;
        frame_.onsetPulse = onsetEnvelope_;
        frame_.downbeat = downbeatEnvelope_;
        frame_.section = sectionEnvelope_;
        onsetEnvelope_ *= std::exp(-30.0f * dt);

        frame_.percussive = smooth(frame_.percussive,
                                   features.percussiveEnergy, 12.0f, dt);
        frame_.harmonic = smooth(frame_.harmonic,
                                 features.harmonicEnergy, 3.0f, dt);

        const float energyTarget = std::clamp(
            0.22f * features.level[0] + 0.22f * features.level[1]
            + 0.16f * features.level[2] + 0.16f * features.level[3]
            + 0.14f * features.level[4] + 0.10f * features.level[5],
            0.0f, 2.5f) / 2.5f;
        const float previousFast = frame_.energyFast;
        frame_.energyFast = smooth(frame_.energyFast, energyTarget, 5.0f, dt);
        frame_.energySlow = smooth(frame_.energySlow, energyTarget, 0.45f, dt);
        frame_.energySlope = smooth(frame_.energySlope,
                                    (frame_.energyFast - previousFast) / dt,
                                    2.0f, dt);
        const ArrangementState& arrangement = arrangementClassifier_.update({
            .barIndex = structure.barIndex,
            .barAnalyzed = structure.barAnalyzed,
            .sectionCrossed = structure.sectionCrossed,
            .motifRecalled = structure.motifRecalled,
            .musicActive = musicActive,
            .trackProgress = frame_.trackProgress,
            .novelty = structure.novelty,
            .noveltyThreshold = structure.noveltyThreshold,
            .energyFast = frame_.energyFast,
            .energySlow = frame_.energySlow,
            .energySlope = frame_.energySlope,
            .percussive = frame_.percussive,
            .rhythmicDensity = frame_.rhythmicDensity,
            .harmonicChange = frame_.harmonicChange,
        }, dt);
        frame_.arrangementRole = arrangement.role;
        frame_.arrangementConfidence = arrangement.confidence;
        frame_.arrangementChanged = arrangement.changed;
        return frame_;
    }

    void reset() { *this = MusicFrameBuilder{}; }

private:
    static float smooth(float current, float target, float speed, float seconds) {
        return current + (target - current) * (1.0f - std::exp(-speed * seconds));
    }

    MusicFrame frame_{};
    float kickEnvelope_ = 0.0f;
    float snareEnvelope_ = 0.0f;
    float hatEnvelope_ = 0.0f;
    float beatEnvelope_ = 0.0f;
    float onsetEnvelope_ = 0.0f;
    float previousOnsetCandidate_ = 0.0f;
    int onsetCooldown_ = 0;
    float downbeatEnvelope_ = 0.0f;
    float sectionEnvelope_ = 0.0f;
    float densityAccumulator_ = 0.0f;
    float syncopationSum_ = 0.0f;
    float syncopationWeight_ = 0.0f;
    std::array<float, AudioFeatures::chromaCount> chromaBaseline_{};
    bool chromaInitialized_ = false;
    float previousTonalDifference_ = 0.0f;
    float harmonicChangeEnvelope_ = 0.0f;
    int harmonicChangeCooldown_ = 0;
    ArrangementClassifier arrangementClassifier_;
};
