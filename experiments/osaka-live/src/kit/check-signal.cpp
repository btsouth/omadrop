#include "check-signal.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <memory>

namespace Journey::Kit::Check {
namespace {
constexpr double Rate = 44100;
constexpr double Pi = 3.14159265358979323846;

struct Noise {
    std::uint32_t state = 0x9e3779b9u;
    double next() {
        state = state * 1664525u + 1013904223u;
        return (state >> 8) / double(1 << 23) - 1.0;
    }
};

class Mix {
public:
    explicit Mix(double seconds) : left_(std::size_t(seconds * Rate)), right_(left_.size()) {}
    std::size_t size() const { return left_.size(); }
    // Adds a voice from start seconds; fn(t) returns the sample, t from 0.
    template <class Fn> void voice(double start, double length, double amplitude, double pan, Fn fn) {
        const std::size_t first = std::size_t(std::max(0.0, start) * Rate);
        const std::size_t last = std::min(size(), first + std::size_t(length * Rate));
        const double l = amplitude * std::sqrt(0.5 * (1 - pan)), r = amplitude * std::sqrt(0.5 * (1 + pan));
        for (std::size_t i = first; i < last; ++i) {
            const double s = fn((i - first) / Rate);
            left_[i] += float(s * l);
            right_[i] += float(s * r);
        }
    }
    std::vector<float> finish(double peak) const {
        float top = 1e-6f;
        for (std::size_t i = 0; i < size(); ++i) top = std::max({top, std::abs(left_[i]), std::abs(right_[i])});
        std::vector<float> out(size() * 2);
        for (std::size_t i = 0; i < size(); ++i) {
            out[2 * i] = left_[i] * float(peak) / top;
            out[2 * i + 1] = right_[i] * float(peak) / top;
        }
        return out;
    }
private:
    std::vector<float> left_, right_;
};

void kick(Mix& m, double at, double amplitude) {
    m.voice(at, 0.32, amplitude, 0, [](double t) {
        const double phase = 2 * Pi * (45 * t + 105 * 0.045 * (1 - std::exp(-t / 0.045)));
        return std::sin(phase) * std::exp(-t / 0.13);
    });
}
void snare(Mix& m, double at, double amplitude) {
    auto noise = std::make_shared<Noise>();
    m.voice(at, 0.25, amplitude, 0.1, [noise](double t) {
        return (0.6 * noise->next() + 0.5 * std::sin(2 * Pi * 190 * t)) * std::exp(-t / 0.07);
    });
}
void hat(Mix& m, double at, double amplitude, double pan) {
    auto noise = std::make_shared<Noise>();
    auto last = std::make_shared<double>(0);
    m.voice(at, 0.08, amplitude, pan, [noise, last](double t) {
        const double n = noise->next(), s = n - *last;
        *last = n;
        return s * std::exp(-t / 0.018);
    });
}
void tone(Mix& m, double at, double length, double hz, double amplitude, double pan, double attack = 0.01) {
    m.voice(at, length, amplitude, pan, [=](double t) {
        const double env = std::min(1.0, t / attack) * std::min(1.0, (length - t) / 0.04);
        return env * (std::sin(2 * Pi * hz * t) + 0.3 * std::sin(4 * Pi * hz * t));
    });
}
void pad(Mix& m, double at, double length, double amplitude) {
    // A minor ninth, slowly swelling.
    static constexpr double notes[] = {220.0, 261.63, 329.63, 392.0, 493.88};
    for (int i = 0; i < 5; ++i) {
        const double hz = notes[i];
        m.voice(at, length, amplitude, (i - 2) * 0.3, [=](double t) {
            const double env = std::min({1.0, t / 1.5, (length - t) / 1.5});
            return env * (0.7 + 0.3 * std::sin(2 * Pi * 0.15 * t + i)) * std::sin(2 * Pi * hz * t);
        });
    }
}

std::vector<float> music(double seconds) {
    Mix m(seconds);
    auto noise = std::make_shared<Noise>();
    // Pad throughout, louder in the break.
    for (double t = 0; t < seconds; t += 8) pad(m, t, 9, (t >= 24 && t < 32) ? 0.22 : 0.10);
    // Sections: 0-8 intro hats, 8-24 groove at 120 BPM, 24-32 break with a
    // rising noise sweep, 32-52 four on the floor at 128 BPM, 52-58 kick roll,
    // then a short tail.
    const double pattern[] = {55.0, 55.0, 82.41, 73.42};
    for (double t = 0; t < 8; t += 0.5) hat(m, t, 0.25, std::fmod(t, 1.0) == 0 ? -0.4 : 0.4);
    for (double t = 8; t < 24; t += 0.5) {
        const int beat = int((t - 8) / 0.5);
        kick(m, t, 0.9);
        if (beat % 2 == 1) snare(m, t, 0.5);
        hat(m, t + 0.25, 0.3, beat % 2 ? 0.5 : -0.5);
        tone(m, t, 0.45, pattern[(beat / 2) % 4], 0.45, 0, 0.02);
    }
    m.voice(24, 8, 0.35, 0, [noise](double t) { return noise->next() * std::pow(t / 8, 2.0); });
    const double beat = 60.0 / 128.0;
    for (int i = 0; 32 + i * beat < 52; ++i) {
        const double t = 32 + i * beat;
        kick(m, t, 0.95);
        if (i % 2 == 1) snare(m, t, 0.55);
        for (int s = 0; s < 4; ++s) hat(m, t + s * beat / 4, 0.28, (s % 2) ? 0.6 : -0.6);
        tone(m, t, beat * 0.9, pattern[(i / 2) % 4], 0.45, 0, 0.015);
        // Lead arpeggio in the upper bands.
        static constexpr double lead[] = {880.0, 1046.5, 1318.5, 1568.0, 2093.0, 2637.0};
        for (int s = 0; s < 4; ++s)
            tone(m, t + s * beat / 4, beat / 4 * 0.9, lead[(i * 2 + s) % 6], 0.16, (s % 2) ? 0.5 : -0.5, 0.004);
    }
    for (double t = 52; t < 58; t += 0.125) { kick(m, t, 0.95); hat(m, t + 0.0625, 0.25, 0); }
    return m.finish(0.7);
}

std::vector<float> strobe(double seconds) {
    Mix m(seconds);
    for (double t = 0.5; t < seconds - 0.3; t += 0.2) kick(m, t, 1.0);
    return m.finish(0.95);
}
}

std::vector<float> makeCheckSignal(SignalKind kind, double seconds) {
    return kind == SignalKind::Music ? music(seconds) : strobe(seconds);
}
}
