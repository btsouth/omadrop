#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string_view>

enum class ArrangementRole : std::uint8_t {
    Unknown,
    Intro,
    Verse,
    Chorus,
    Bridge,
    Breakdown,
    Build,
    Peak,
    Outro,
};

inline constexpr std::string_view arrangementRoleName(ArrangementRole role) {
    switch (role) {
        case ArrangementRole::Unknown: return "unknown";
        case ArrangementRole::Intro: return "intro";
        case ArrangementRole::Verse: return "verse";
        case ArrangementRole::Chorus: return "chorus";
        case ArrangementRole::Bridge: return "bridge";
        case ArrangementRole::Breakdown: return "breakdown";
        case ArrangementRole::Build: return "build";
        case ArrangementRole::Peak: return "peak";
        case ArrangementRole::Outro: return "outro";
    }
    return "unknown";
}

struct ArrangementObservation {
    int barIndex = 0;
    bool barAnalyzed = false;
    bool sectionCrossed = false;
    bool motifRecalled = false;
    bool musicActive = false;
    float trackProgress = -1.0f;
    float novelty = 0.0f;
    float noveltyThreshold = 0.10f;
    float energyFast = 0.0f;
    float energySlow = 0.0f;
    float energySlope = 0.0f;
    float percussive = 0.0f;
    float rhythmicDensity = 0.0f;
    float harmonicChange = 0.0f;
};

struct ArrangementState {
    ArrangementRole role = ArrangementRole::Unknown;
    float confidence = 0.0f;
    bool changed = false;
};

// Assigns slow, passage-level roles from structural evidence. This deliberately
// ignores individual hits: a role may change only at an analyzed bar, except
// for the first active intro frame. Silence never fabricates a new section.
class ArrangementClassifier {
public:
    const ArrangementState& update(const ArrangementObservation& observation,
                                   float seconds) {
        const float dt = std::clamp(seconds, 1.0f / 240.0f, 0.1f);
        state_.changed = false;
        if (!observation.musicActive) {
            risingSeconds_ = 0.0f;
            highSeconds_ = 0.0f;
            lowSeconds_ = 0.0f;
            return state_;
        }

        const float energy = std::clamp(
            0.58f * observation.energyFast + 0.42f * observation.energySlow,
            0.0f, 1.0f);
        if (!energyInitialized_) {
            energyMean_ = energy;
            energyPeak_ = energy;
            energyInitialized_ = true;
        } else {
            const float blend = 1.0f - std::exp(-0.10f * dt);
            energyMean_ += (energy - energyMean_) * blend;
            energyPeak_ = std::max(
                energy, energyPeak_ * std::exp(-0.008f * dt));
        }

        const bool rising = observation.energySlope > 0.018f
                         && observation.energyFast
                            > observation.energySlow + 0.025f;
        const bool high = energy > std::max(0.45f, energyPeak_ * 0.78f)
                       && observation.percussive > 0.34f
                       && observation.rhythmicDensity > 0.24f;
        const bool low = observation.barIndex >= 4
                      && energy < std::min(0.36f, energyPeak_ * 0.62f)
                      && observation.rhythmicDensity < 0.34f;
        risingSeconds_ = rising ? risingSeconds_ + dt : 0.0f;
        highSeconds_ = high ? highSeconds_ + dt : 0.0f;
        lowSeconds_ = low ? lowSeconds_ + dt : 0.0f;

        const bool knownPosition = observation.trackProgress >= 0.0f;
        const bool earlyByPosition = knownPosition
                                  && observation.trackProgress < 0.04f;
        const bool earlyByBars = observation.barIndex < 2
                              && (!knownPosition
                                  || observation.trackProgress < 0.12f);
        if (state_.role == ArrangementRole::Unknown
            && (earlyByPosition || earlyByBars)) {
            setRole(ArrangementRole::Intro, 0.88f, observation.barIndex);
        }
        if (earlyByPosition || earlyByBars) return state_;
        if (!observation.barAnalyzed) return state_;

        const bool enoughHold = state_.role == ArrangementRole::Unknown
                             || state_.role == ArrangementRole::Intro
                             || observation.barIndex - roleStartedAtBar_ >= 2;
        const bool late = observation.trackProgress >= 0.88f;
        const bool falling = observation.energySlope < -0.015f
                          || energy < energyMean_ * 0.72f;
        if (late && falling) {
            setRole(ArrangementRole::Outro, 0.91f, observation.barIndex);
            return state_;
        }

        const bool decisiveRise = observation.energyFast
                                  > observation.energySlow + 0.075f
                               && observation.energySlope > 0.08f;
        const bool peakAfterBuild = energy > std::max(
                                        0.42f, energyMean_ * 1.12f)
                                 && observation.rhythmicDensity > 0.32f;
        if (observation.sectionCrossed) {
            if (observation.motifRecalled) {
                setRole(ArrangementRole::Chorus, 0.97f,
                        observation.barIndex);
                return state_;
            }
            ++newSectionsSeen_;
            if (state_.role == ArrangementRole::Breakdown && decisiveRise) {
                setRole(ArrangementRole::Build, 0.86f,
                        observation.barIndex);
            } else if (lowSeconds_ >= 0.8f) {
                setRole(ArrangementRole::Breakdown, 0.88f,
                        observation.barIndex);
            } else if (highSeconds_ >= 0.8f) {
                setRole(ArrangementRole::Peak, 0.88f,
                        observation.barIndex);
            } else if (risingSeconds_ >= 1.2f) {
                setRole(ArrangementRole::Build, 0.84f,
                        observation.barIndex);
            } else if (newSectionsSeen_ >= 2
                       && observation.novelty
                          >= std::max(0.14f,
                              observation.noveltyThreshold * 1.30f)) {
                setRole(ArrangementRole::Bridge, 0.84f,
                        observation.barIndex);
            } else {
                setRole(ArrangementRole::Verse, 0.72f,
                        observation.barIndex);
            }
            return state_;
        }

        if (state_.role == ArrangementRole::Breakdown && decisiveRise) {
            setRole(ArrangementRole::Build, 0.84f, observation.barIndex);
        } else if (state_.role == ArrangementRole::Build
                   && (highSeconds_ >= 0.6f || peakAfterBuild)) {
            setRole(ArrangementRole::Peak, 0.84f, observation.barIndex);
        } else if (state_.role == ArrangementRole::Unknown) {
            setRole(ArrangementRole::Verse, 0.66f, observation.barIndex);
        } else if (state_.role == ArrangementRole::Intro
                   && !earlyByPosition && !earlyByBars) {
            setRole(ArrangementRole::Verse, 0.70f, observation.barIndex);
        } else if (enoughHold
                   && state_.role != ArrangementRole::Chorus
                   && state_.role != ArrangementRole::Bridge) {
            if (highSeconds_ >= 0.8f) {
                setRole(ArrangementRole::Peak, 0.80f,
                        observation.barIndex);
            } else if (risingSeconds_ >= 1.2f && energy < 0.76f) {
                setRole(ArrangementRole::Build, 0.77f,
                        observation.barIndex);
            } else if (lowSeconds_ >= 0.8f) {
                setRole(ArrangementRole::Breakdown, 0.80f,
                        observation.barIndex);
            }
        }
        return state_;
    }

    const ArrangementState& state() const { return state_; }
    void reset() { *this = ArrangementClassifier{}; }

private:
    void setRole(ArrangementRole role, float confidence, int barIndex) {
        if (role == state_.role) {
            state_.confidence = std::max(state_.confidence, confidence);
            return;
        }
        state_.role = role;
        state_.confidence = confidence;
        state_.changed = true;
        roleStartedAtBar_ = barIndex;
    }

    ArrangementState state_{};
    float energyMean_ = 0.0f;
    float energyPeak_ = 0.0f;
    float risingSeconds_ = 0.0f;
    float highSeconds_ = 0.0f;
    float lowSeconds_ = 0.0f;
    int roleStartedAtBar_ = 0;
    int newSectionsSeen_ = 0;
    bool energyInitialized_ = false;
};
