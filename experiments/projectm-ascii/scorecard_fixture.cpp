#include "audio_features.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <string>

namespace {
constexpr double tau = 6.28318530717958647692;
constexpr double durationSeconds = 32.0;

double pulseAge(double seconds, double interval, double offset = 0.0) {
    if (seconds < offset) return 1000.0;
    double age = std::fmod(seconds - offset, interval);
    if (age < 0.0) age += interval;
    return age;
}

float decayingTone(double age, double decay, double frequency,
                   float amplitude) {
    if (age < 0.0 || age > decay * 7.0) return 0.0f;
    return amplitude * static_cast<float>(
        std::exp(-age / decay) * std::sin(tau * frequency * age));
}

std::uint32_t noiseState = 0x6d2b79f5u;

float deterministicNoise() {
    noiseState ^= noiseState << 13;
    noiseState ^= noiseState >> 17;
    noiseState ^= noiseState << 5;
    return static_cast<float>(noiseState) / 2147483648.0f - 1.0f;
}

float chordTone(double seconds, int chord, bool right) {
    constexpr double roots[]{110.0, 146.832, 130.813, 164.814};
    constexpr double ratios[]{1.0, 1.25, 1.5};
    float sample = 0.0f;
    for (int voice = 0; voice < 3; ++voice) {
        const double frequency = roots[chord] * ratios[voice];
        const double phase = right ? 0.21 * (voice + 1) : 0.0;
        sample += 0.020f * static_cast<float>(
            std::sin(tau * frequency * seconds + phase));
    }
    return sample;
}
} // namespace

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cerr << "usage: scorecard-fixture OUTPUT_RAW_F32_STEREO\n";
        return 2;
    }

    std::ofstream output(argv[1], std::ios::binary);
    if (!output) {
        std::cerr << "could not create: " << argv[1] << "\n";
        return 1;
    }

    const std::int64_t frameCount = static_cast<std::int64_t>(
        durationSeconds * AudioFeatureBus::sampleRate);
    for (std::int64_t frame = 0; frame < frameCount; ++frame) {
        const double seconds = frame
            / static_cast<double>(AudioFeatureBus::sampleRate);
        const int section = seconds < 4.0 ? 0 : seconds < 12.0 ? 1
                          : seconds < 16.0 ? 2 : seconds < 24.0 ? 3 : 4;
        const int chord = std::min(3, static_cast<int>(seconds / 4.0) % 4);

        float middle = chordTone(seconds, chord, false);
        float side = 0.55f * chordTone(seconds, chord, true);

        if (section != 0 && seconds < 30.0) {
            middle += 0.030f * static_cast<float>(
                std::sin(tau * 110.0 * seconds));
        }

        bool kickActive = section == 1 || section == 3 || section == 4;
        double kickInterval = section == 3 ? 0.5 : 1.0;
        if (section == 4 && seconds >= 28.0) kickInterval = 2.0;
        if (kickActive && seconds < 30.0) {
            const double age = pulseAge(seconds, kickInterval, 4.0);
            middle += decayingTone(age, 0.075, 62.0, 0.56f);
        }

        bool snareActive = section == 1 || section == 2
                        || section == 3 || section == 4;
        if (snareActive && seconds < 30.0) {
            const double age = pulseAge(seconds, 1.0, 4.5);
            middle += decayingTone(age, 0.045, 1100.0, 0.14f);
            middle += decayingTone(age, 0.038, 2200.0, 0.24f);
        }

        bool hatActive = seconds >= 2.0 && seconds < 30.0;
        if (hatActive) {
            const double interval = section == 3 ? 0.125 : 0.25;
            const double age = pulseAge(seconds, interval, 2.0);
            const float hatEnvelope = age < 0.075
                ? 0.13f * static_cast<float>(std::exp(-age / 0.014)) : 0.0f;
            const float hat = decayingTone(age, 0.014, 7000.0, 0.13f)
                            + hatEnvelope * deterministicNoise() * 0.20f;
            middle += hat;
            side += hat * ((static_cast<int>((seconds - 2.0) / interval) & 1)
                           ? 0.36f : -0.36f);
        } else {
            deterministicNoise();
        }

        const float sectionGain = seconds >= 30.0
            ? static_cast<float>(std::max(0.0, (32.0 - seconds) / 2.0)) : 1.0f;
        const float left = std::clamp((middle + side) * sectionGain, -0.98f, 0.98f);
        const float right = std::clamp((middle - side) * sectionGain, -0.98f, 0.98f);
        output.write(reinterpret_cast<const char*>(&left), sizeof(left));
        output.write(reinterpret_cast<const char*>(&right), sizeof(right));
    }

    if (!output) {
        std::cerr << "could not finish fixture: " << argv[1] << "\n";
        return 1;
    }
    std::cout << "wrote deterministic 32-second, 44.1 kHz stereo fixture to "
              << argv[1] << "\n";
}
