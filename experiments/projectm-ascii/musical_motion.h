#pragma once

#include "music_frame.h"
#include <array>
#include <algorithm>
#include <cmath>

// Presentation state, separate from musical analysis. Exact critically damped
// integration preserves velocity between audio packets and is stable at any
// display rate. Nothing here invents a beat or changes the analyzer's clock.
struct MusicalSpring {
    float position = 0.0f;
    float velocity = 0.0f;
    float update(float target, float omega, float seconds) {
        const float dt = std::clamp(seconds, 0.0f, 0.1f);
        const float offset = position - target;
        const float combined = velocity + omega * offset;
        const float decay = std::exp(-omega * dt);
        position = target + (offset + combined * dt) * decay;
        velocity = (velocity - omega * combined * dt) * decay;
        return position;
    }
};

struct MusicalMotionFrame {
    std::array<float, 3> impact{};
    std::array<float, 6> bands{};
    std::array<float, 32> spectrum{};
    std::array<float, 32> harmonicShape{};
    float bassBody = 0.0f;
    float groove = 0.0f;
    float expansion = 0.0f;
};

class MusicalMotion {
public:
    static float impactStrength(float input, float intensity) {
        // Leave headroom: a routine 0.45 hit is much smaller than a 1.2 hit.
        return std::pow(std::clamp(input * intensity / 1.25f, 0.0f, 1.0f), 1.15f);
    }
    const MusicalMotionFrame& update(const MusicFrame& music, float seconds,
                                    float intensity = 1.0f) {
        const std::array<float, 3> hits{music.kick, music.snare, music.hat};
        // Visible gestures need time to travel. The former 85/115/160
        // responses settled so quickly that dense percussion looked like jitter.
        // Approximate step-to-90% times: 139 / 97 / 65 ms, with no overshoot.
        constexpr std::array<float, 3> response{28.0f, 40.0f, 60.0f};
        for (std::size_t i = 0; i < hits.size(); ++i)
            frame_.impact[i] = impact_[i].update(
                impactStrength(hits[i], intensity), response[i], seconds);
        for (std::size_t i = 0; i < frame_.bands.size(); ++i)
            frame_.bands[i] = bands_[i].update(
                std::clamp(music.bandLevel[i], 0.0f, 4.0f),
                26.0f + 4.0f * static_cast<float>(i), seconds);
        for (std::size_t i = 0; i < frame_.spectrum.size(); ++i)
            frame_.spectrum[i] = spectrum_[i].update(
                std::clamp(music.spectrumLevel[i], 0.0f, 6.0f), 28.0f, seconds);
        for (std::size_t i=0;i<frame_.harmonicShape.size();++i)
            frame_.harmonicShape[i]=harmonic_[i].update(
                std::clamp(music.harmonicShape[i],0.0f,1.0f),24.0f,seconds);
        frame_.bassBody=bassBody_.update(std::clamp(music.bassBody,0.0f,1.0f),28.0f,seconds);
        frame_.groove = groove_.update(music.clockConfidence
            * (music.beatPulse + 0.65f * music.downbeat
               - 0.45f * music.beatAnticipation), 85.0f, seconds);
        frame_.expansion = expansion_.update(
            std::clamp(music.energyFast, 0.0f, 1.0f), 9.0f, seconds);
        return frame_;
    }
    void reset() { *this = MusicalMotion{}; }
private:
    std::array<MusicalSpring, 3> impact_;
    std::array<MusicalSpring, 6> bands_;
    std::array<MusicalSpring, 32> spectrum_;
    std::array<MusicalSpring,32> harmonic_;
    MusicalSpring bassBody_, groove_, expansion_;
    MusicalMotionFrame frame_;
};
