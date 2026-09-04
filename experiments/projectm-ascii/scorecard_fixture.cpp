#include "audio_features.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>

namespace {
constexpr double tau = 6.28318530717958647692;

enum class FixtureProfile {
    StructuredElectronic,
    SparseAcoustic,
    DenseCompressed,
    SustainedVocal,
    SyncopatedSections,
};

struct StereoField {
    float middle = 0.0f;
    float side = 0.0f;
};

std::optional<FixtureProfile> fixtureProfile(std::string_view name) {
    if (name == "structured-electronic" || name == "structured") {
        return FixtureProfile::StructuredElectronic;
    }
    if (name == "sparse-acoustic" || name == "sparse") {
        return FixtureProfile::SparseAcoustic;
    }
    if (name == "dense-compressed" || name == "dense") {
        return FixtureProfile::DenseCompressed;
    }
    if (name == "sustained-vocal" || name == "sustained") {
        return FixtureProfile::SustainedVocal;
    }
    if (name == "syncopated-sections" || name == "syncopated") {
        return FixtureProfile::SyncopatedSections;
    }
    return std::nullopt;
}

std::string_view fixtureProfileName(FixtureProfile profile) {
    switch (profile) {
        case FixtureProfile::StructuredElectronic:
            return "structured-electronic";
        case FixtureProfile::SparseAcoustic:
            return "sparse-acoustic";
        case FixtureProfile::DenseCompressed:
            return "dense-compressed";
        case FixtureProfile::SustainedVocal:
            return "sustained-vocal";
        case FixtureProfile::SyncopatedSections:
            return "syncopated-sections";
    }
    return "unknown";
}

double fixtureDuration(FixtureProfile profile) {
    return profile == FixtureProfile::StructuredElectronic ? 32.0 : 24.0;
}

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

float endingGain(double seconds, double duration, double fadeSeconds = 2.0) {
    return static_cast<float>(std::clamp(
        (duration - seconds) / fadeSeconds, 0.0, 1.0));
}

StereoField structuredElectronic(double seconds) {
    const int section = seconds < 4.0 ? 0 : seconds < 12.0 ? 1
                      : seconds < 16.0 ? 2 : seconds < 24.0 ? 3 : 4;
    const int chord = std::min(3, static_cast<int>(seconds / 4.0) % 4);
    StereoField field{chordTone(seconds, chord, false),
                      0.55f * chordTone(seconds, chord, true)};

    if (section != 0 && seconds < 30.0) {
        field.middle += 0.030f * static_cast<float>(
            std::sin(tau * 110.0 * seconds));
    }

    const bool kickActive = section == 1 || section == 3 || section == 4;
    double kickInterval = section == 3 ? 0.5 : 1.0;
    if (section == 4 && seconds >= 28.0) kickInterval = 2.0;
    if (kickActive && seconds < 30.0) {
        const double age = pulseAge(seconds, kickInterval, 4.0);
        field.middle += decayingTone(age, 0.075, 62.0, 0.56f);
    }

    const bool snareActive = section == 1 || section == 2
                          || section == 3 || section == 4;
    if (snareActive && seconds < 30.0) {
        const double age = pulseAge(seconds, 1.0, 4.5);
        field.middle += decayingTone(age, 0.045, 1100.0, 0.14f);
        field.middle += decayingTone(age, 0.038, 2200.0, 0.24f);
    }

    const bool hatActive = seconds >= 2.0 && seconds < 30.0;
    if (hatActive) {
        const double interval = section == 3 ? 0.125 : 0.25;
        const double age = pulseAge(seconds, interval, 2.0);
        const float hatEnvelope = age < 0.075
            ? 0.13f * static_cast<float>(std::exp(-age / 0.014)) : 0.0f;
        const float hat = decayingTone(age, 0.014, 7000.0, 0.13f)
                        + hatEnvelope * deterministicNoise() * 0.20f;
        field.middle += hat;
        field.side += hat * ((static_cast<int>((seconds - 2.0) / interval) & 1)
                             ? 0.36f : -0.36f);
    } else {
        deterministicNoise();
    }
    const float gain = endingGain(seconds, 32.0);
    field.middle *= gain;
    field.side *= gain;
    return field;
}

StereoField sparseAcoustic(double seconds) {
    constexpr std::array<double, 4> roots{196.0, 246.942, 220.0, 293.665};
    const int chord = static_cast<int>(seconds / 3.0) % 4;
    const double pluckAge = pulseAge(seconds, 0.75, 2.0);
    const double root = roots[chord];
    const float pluckEnvelope = pluckAge < 1.0
        ? static_cast<float>(std::exp(-pluckAge / 0.19)) : 0.0f;
    StereoField field;
    field.middle = pluckEnvelope * (
        0.15f * static_cast<float>(std::sin(tau * root * pluckAge))
        + 0.052f * static_cast<float>(std::sin(tau * root * 2.01 * pluckAge))
        + 0.025f * static_cast<float>(std::sin(tau * root * 3.02 * pluckAge)));
    field.side = field.middle * ((static_cast<int>((seconds - 2.0) / 0.75) & 1)
                                 ? 0.22f : -0.22f);

    const bool breakdown = seconds >= 10.0 && seconds < 14.0;
    if (seconds >= 5.0 && !breakdown && seconds < 22.0) {
        const double kickAge = pulseAge(seconds, 1.5, 5.0);
        field.middle += decayingTone(kickAge, 0.085, 58.0, 0.48f);

        const double brushAge = pulseAge(seconds, 1.5, 5.75);
        if (brushAge < 0.12) {
            const float brush = 0.14f * static_cast<float>(
                std::exp(-brushAge / 0.035)) * deterministicNoise();
            field.middle += brush;
            field.side -= brush * 0.18f;
        } else {
            deterministicNoise();
        }
    } else {
        deterministicNoise();
    }

    if (seconds >= 14.0 && seconds < 22.0) {
        const double hatAge = pulseAge(seconds, 0.375, 14.0);
        const float hat = decayingTone(hatAge, 0.012, 7600.0, 0.085f);
        field.middle += hat;
        field.side += hat * 0.32f;
    }
    const float intro = static_cast<float>(std::clamp(seconds / 2.0, 0.08, 1.0));
    const float gain = intro * endingGain(seconds, 24.0);
    field.middle *= gain;
    field.side *= gain;
    return field;
}

StereoField denseCompressed(double seconds) {
    const bool breakdown = seconds >= 10.0 && seconds < 12.0;
    const int chord = static_cast<int>(seconds / 3.0) % 4;
    StereoField field{2.1f * chordTone(seconds, chord, false),
                      1.2f * chordTone(seconds, chord, true)};
    field.middle += 0.055f * static_cast<float>(
        std::sin(tau * 82.407 * seconds));
    field.side += 0.035f * static_cast<float>(
        std::sin(tau * 329.628 * seconds + 0.4));

    if (seconds >= 2.0 && seconds < 22.0 && !breakdown) {
        const double kickAge = pulseAge(seconds, 0.5, 2.0);
        field.middle += decayingTone(kickAge, 0.065, 54.0, 0.72f);
        const double snareAge = pulseAge(seconds, 0.5, 2.25);
        field.middle += decayingTone(snareAge, 0.040, 1250.0, 0.23f);
        field.middle += decayingTone(snareAge, 0.030, 2500.0, 0.19f);

        const double hatAge = pulseAge(seconds, 0.125, 2.0);
        const float hatEnvelope = hatAge < 0.055
            ? 0.11f * static_cast<float>(std::exp(-hatAge / 0.012)) : 0.0f;
        const float hat = decayingTone(hatAge, 0.011, 8200.0, 0.10f)
                        + deterministicNoise() * hatEnvelope;
        field.middle += hat;
        field.side += hat * ((static_cast<int>((seconds - 2.0) / 0.125) & 1)
                             ? 0.30f : -0.30f);
    } else {
        deterministicNoise();
        field.middle *= breakdown ? 0.28f : 1.0f;
        field.side *= breakdown ? 0.28f : 1.0f;
    }

    field.middle = std::tanh(field.middle * 2.15f) * 0.58f;
    field.side = std::tanh(field.side * 1.65f) * 0.36f;
    const float gain = endingGain(seconds, 24.0);
    field.middle *= gain;
    field.side *= gain;
    return field;
}

StereoField sustainedVocal(double seconds) {
    constexpr std::array<double, 4> roots{174.614, 195.998, 164.814, 220.0};
    const int chord = static_cast<int>(seconds / 4.0) % 4;
    const double root = roots[chord];
    const double phrase = std::fmod(seconds, 4.0);
    const float phraseEnvelope = static_cast<float>(
        std::sin(3.14159265358979323846 * std::clamp(phrase / 3.6, 0.0, 1.0)));
    const double vocalPhase = tau * root * seconds
                            + 0.55 * std::sin(tau * 5.2 * seconds);
    StereoField field;
    field.middle = phraseEnvelope * (
        0.095f * static_cast<float>(std::sin(vocalPhase))
        + 0.047f * static_cast<float>(std::sin(vocalPhase * 2.01))
        + 0.022f * static_cast<float>(std::sin(vocalPhase * 3.02)));
    field.middle += chordTone(seconds, chord, false) * 0.72f;
    field.side = chordTone(seconds, chord, true) * 0.50f;

    const bool openSection = (seconds >= 7.0 && seconds < 12.0)
                          || (seconds >= 16.0 && seconds < 22.0);
    if (openSection) {
        const double kickAge = pulseAge(seconds, 1.0, 7.0);
        field.middle += decayingTone(kickAge, 0.080, 60.0, 0.42f);
        const double snareAge = pulseAge(seconds, 2.0, 8.0);
        field.middle += decayingTone(snareAge, 0.052, 1500.0, 0.20f);
        const double hatAge = pulseAge(seconds, 0.5, 7.0);
        const float breath = hatAge < 0.07
            ? deterministicNoise() * 0.075f * static_cast<float>(
                std::exp(-hatAge / 0.020)) : deterministicNoise() * 0.002f;
        field.middle += breath;
        field.side += breath * 0.24f;
    } else {
        const float breath = deterministicNoise() * phraseEnvelope * 0.003f;
        field.middle += breath;
    }
    const float intro = static_cast<float>(std::clamp(seconds / 5.0, 0.03, 1.0));
    const float gain = intro * endingGain(seconds, 24.0);
    field.middle *= gain;
    field.side *= gain;
    return field;
}

StereoField syncopatedSections(double seconds) {
    constexpr double stepDuration = 60.0 / 138.0 / 4.0;
    constexpr std::array<std::uint16_t, 3> kickPatterns{
        0b0001000100010001, 0b0100001001001001, 0b0010100100010101};
    constexpr std::array<std::uint16_t, 3> snarePatterns{
        0b0001000000010000, 0b0100000100000100, 0b0001010001000000};
    constexpr std::array<std::uint16_t, 3> hatPatterns{
        0b0101010101010101, 0b1011010110110101, 0b1111011011110110};
    const bool quiet = seconds < 3.0 || (seconds >= 9.0 && seconds < 11.0);
    const int section = std::min(2, static_cast<int>(
        std::max(0.0, seconds - 3.0) / 6.0));
    const double patternTime = std::max(0.0, seconds - 3.0);
    const int step = static_cast<int>(patternTime / stepDuration);
    const int patternStep = step & 15;
    const double age = patternTime - step * stepDuration;
    const auto active = [patternStep](std::uint16_t pattern) {
        return (pattern & (1u << patternStep)) != 0;
    };

    const int chord = static_cast<int>(seconds / 2.0) % 4;
    StereoField field{chordTone(seconds, chord, false) * 0.65f,
                      chordTone(seconds, chord, true) * 0.42f};
    if (!quiet && seconds < 22.0) {
        if (active(kickPatterns[section])) {
            field.middle += decayingTone(age, 0.070, 57.0, 0.60f);
        }
        if (active(snarePatterns[section])) {
            field.middle += decayingTone(age, 0.035, 1350.0, 0.25f);
            field.side -= decayingTone(age, 0.030, 2300.0, 0.10f);
        }
        const float noise = deterministicNoise();
        if (active(hatPatterns[section]) && age < 0.055) {
            const float envelope = 0.12f * static_cast<float>(
                std::exp(-age / 0.012));
            const float hat = envelope * noise
                            + decayingTone(age, 0.010, 7900.0, 0.075f);
            field.middle += hat;
            field.side += hat * ((step & 1) ? 0.35f : -0.35f);
        }
        if (patternStep == 0 || patternStep == 6 || patternStep == 11) {
            field.middle += decayingTone(age, 0.12, 330.0 + 55.0 * section,
                                         0.10f);
        }
    } else {
        deterministicNoise();
        field.middle *= quiet ? 0.30f : 1.0f;
        field.side *= quiet ? 0.30f : 1.0f;
    }
    const float gain = endingGain(seconds, 24.0);
    field.middle *= gain;
    field.side *= gain;
    return field;
}

StereoField fixtureSample(FixtureProfile profile, double seconds) {
    switch (profile) {
        case FixtureProfile::StructuredElectronic:
            return structuredElectronic(seconds);
        case FixtureProfile::SparseAcoustic:
            return sparseAcoustic(seconds);
        case FixtureProfile::DenseCompressed:
            return denseCompressed(seconds);
        case FixtureProfile::SustainedVocal:
            return sustainedVocal(seconds);
        case FixtureProfile::SyncopatedSections:
            return syncopatedSections(seconds);
    }
    return {};
}
} // namespace

int main(int argc, char** argv) {
    if (argc < 2 || argc > 3) {
        std::cerr << "usage: scorecard-fixture OUTPUT_RAW_F32_STEREO "
                     "[structured-electronic|sparse-acoustic|dense-compressed|"
                     "sustained-vocal|syncopated-sections]\n";
        return 2;
    }

    FixtureProfile profile = FixtureProfile::StructuredElectronic;
    if (argc == 3) {
        const auto requested = fixtureProfile(argv[2]);
        if (!requested) {
            std::cerr << "unknown fixture profile: " << argv[2] << "\n";
            return 2;
        }
        profile = *requested;
    }
    noiseState = 0x6d2b79f5u
               ^ (0x9e3779b9u * static_cast<std::uint32_t>(profile));

    std::ofstream output(argv[1], std::ios::binary);
    if (!output) {
        std::cerr << "could not create: " << argv[1] << "\n";
        return 1;
    }

    const double duration = fixtureDuration(profile);
    const std::int64_t frameCount = static_cast<std::int64_t>(
        duration * AudioFeatureBus::sampleRate);
    for (std::int64_t frame = 0; frame < frameCount; ++frame) {
        const double seconds = frame
            / static_cast<double>(AudioFeatureBus::sampleRate);
        const StereoField field = fixtureSample(profile, seconds);
        const float left = std::clamp(field.middle + field.side, -0.98f, 0.98f);
        const float right = std::clamp(field.middle - field.side, -0.98f, 0.98f);
        output.write(reinterpret_cast<const char*>(&left), sizeof(left));
        output.write(reinterpret_cast<const char*>(&right), sizeof(right));
    }

    if (!output) {
        std::cerr << "could not finish fixture: " << argv[1] << "\n";
        return 1;
    }
    std::cout << "wrote deterministic " << fixtureProfileName(profile) << " "
              << duration << "-second, 44.1 kHz stereo fixture to "
              << argv[1] << "\n";
}
