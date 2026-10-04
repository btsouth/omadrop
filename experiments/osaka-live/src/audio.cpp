#include "audio.h"
#include "../../projectm-ascii/audio_features.h"
#include <algorithm>
#include <array>
#include <cmath>
namespace Journey {
namespace {
constexpr int rate = AudioFeatureBus::sampleRate;
constexpr int hop = AudioFeatureBus::hopSize;
constexpr int window = AudioFeatureBus::windowSize;
constexpr double step = double(hop) / rate;

double smooth(double previous, double target, double dt,
              double attack = 0.045, double release = 0.20) {
    return previous + (target - previous)
        * (1.0 - std::exp(-dt / (target > previous ? attack : release)));
}

// Reuse measured stereo features; no extra FFT or changes to the role mappings.
class MusicalSurge {
public:
    double process(const std::array<double, 6>& bands, double bass,
                   double onset, double dt) {
        double power = 0;
        for (double band : bands) power += band * band;
        const double energy = 0.85 * std::sqrt(power / bands.size()) + 0.15 * bass;
        level_ = smooth(level_, energy, dt, 0.10, 0.20);
        elapsed_ += dt;
        // One second of warmup seeds the baseline, suppressing startup events.
        if (elapsed_ <= 1.0) {
            baseline_ = level_;
            return 0;
        }
        // A 2s baseline, >=0.08 absolute energy and >=0.03 / 35% upward
        // novelty reserve swells for increases rather than sustained loudness.
        const double novelty = std::max(0.0, level_ - baseline_);
        const double threshold = std::max(0.03, 0.35 * baseline_);
        baseline_ = smooth(baseline_, level_, dt, 2.0, 2.0);
        cooldown_ = std::max(0.0, cooldown_ - dt);
        hold_ = std::max(0.0, hold_ - dt);
        // Hysteresis requires the rise to settle before another crossing.
        if (novelty < threshold * 0.5) armed_ = true;
        if (armed_ && novelty >= threshold && level_ >= 0.08) {
            armed_ = false;
            if (cooldown_ <= 0) {
                // Onset supports strength only; it cannot trigger a swell alone.
                strength_ = std::clamp(0.45 + 1.5 * novelty + 0.10 * onset, 0.0, 1.0);
                hold_ = 0.65;
                cooldown_ = 6.0;
            }
        }
        // Briefly hold the measured event, with 0.3s attack / 2s release.
        value_ = smooth(value_, hold_ > 0 ? strength_ : 0.0, dt, 0.30, 2.0);
        return std::clamp(value_, 0.0, 1.0);
    }
private:
    double level_ = 0, baseline_ = 0, elapsed_ = 0;
    double cooldown_ = 0, hold_ = 0, strength_ = 0, value_ = 0;
    bool armed_ = true;
};

// The bus's level[] is divided by each role's history. A supplementary FFT
// measures absolute power with a shared gain instead, keeping role balance.
// Analyze channels separately so opposite-phase stereo does not erase bands.
class AbsoluteBands {
public:
    AbsoluteBands() {
        for (int i = 0; i < window; ++i)
            hann_[i] = 0.5f - 0.5f * std::cos(2.0 * 3.141592653589793 * i / (window - 1));
        plan_ = fftwf_plan_dft_r2c_1d(window, input_.data(),
            reinterpret_cast<fftwf_complex*>(output_.data()), FFTW_ESTIMATE);
    }
    ~AbsoluteBands() { if (plan_) fftwf_destroy_plan(plan_); }
    bool valid() const { return plan_ != nullptr; }
    std::array<double, 6> process(const float* stereo, int count) {
        std::array<double, 6> power{};
        constexpr std::array<double, 7> edges{25, 70, 150, 400, 1500, 4000, 12000};
        for (int channel = 0; channel < 2; ++channel) {
            auto& history = history_[channel];
            std::move(history.begin() + count, history.end(), history.begin());
            for (int i = 0; i < count; ++i)
                history[window - count + i] = stereo[2 * i + channel];
            for (int i = 0; i < window; ++i) input_[i] = history[i] * hann_[i];
            fftwf_execute(plan_);
            for (int bin = 1; bin <= window / 2; ++bin) {
                const double hz = double(bin) * rate / window;
                for (std::size_t role = 0; role < power.size(); ++role) {
                    if (hz >= edges[role] && hz < edges[role + 1]) {
                        const double re = output_[bin][0], im = output_[bin][1];
                        power[role] += (re * re + im * im) * 0.5;
                        break;
                    }
                }
            }
        }
        for (double& value : power)
            value = std::sqrt(value) / window;
        return power;
    }
private:
    std::array<std::array<float, window>, 2> history_{};
    std::array<float, window> hann_{}, input_{};
    std::array<std::array<float, 2>, window / 2 + 1> output_{};
    fftwf_plan plan_ = nullptr;
};
}

struct StreamingAudio::Impl {
    AudioFeatureBus left, right;
    AbsoluteBands absolute;
    MusicalSurge surge;
    Audio current;
    std::array<float, hop * 2> pending{};
    int used = 0;
    double gain = 1, reference = 0.02, rms = 0;
    void analyze() {
        std::array<std::array<float, hop * 2>, 2> mono{};
        double power = 0;
        for (int i = 0; i < hop; ++i) {
            for (int ch = 0; ch < 2; ++ch) {
                const float v = pending[2*i + ch];
                mono[ch][2*i] = mono[ch][2*i+1] = v;
                power += double(v)*v;
            }
        }
        rms = std::sqrt(power / (hop * 2));
        const auto& l = left.processStereo(mono[0].data(), hop);
        const auto& r = right.processStereo(mono[1].data(), hop);
        auto bands = absolute.process(pending.data(), hop);
        double peak = *std::max_element(bands.begin(), bands.end());
        // Shared, causal gain: preserve band balance and meaningful dynamics.
        // No gain riding during silence. A -60 dB RMS gate avoids noise events.
        if (rms > 0.001) reference = smooth(reference, peak, step, 1.5, 8.0);
        const double target = std::clamp(0.035 / std::max(0.00875, reference), 1.0, 4.0);
        gain = smooth(gain, target, step, 2.0, 0.6);
        const double audible = std::clamp((rms - 0.0005) / 0.0015, 0.0, 1.0);
        for (int i = 0; i < 6; ++i) {
            bands[i] = -std::expm1(-8.0 * gain * bands[i]) * audible;
            current.bands[i] = smooth(current.bands[i], bands[i], step);
        }
        const double bass = std::sqrt((double(l.bassBody)*l.bassBody + double(r.bassBody)*r.bassBody)*0.5)*audible;
        current.bass = smooth(current.bass, std::clamp(bass, 0.0, 1.0), step);
        const double impact = std::max({double(l.kickImpact),double(l.snareImpact),double(l.hatImpact),
            double(r.kickImpact),double(r.snareImpact),double(r.hatImpact)}) / 1.35 * audible;
        current.accent = smooth(current.accent, std::clamp(impact,0.0,1.0), step, 0.025,0.13);
        current.surge = surge.process(bands, bass, impact, step);
    }
};
StreamingAudio::StreamingAudio() : impl_(std::make_unique<Impl>()) {}
StreamingAudio::~StreamingAudio() = default;
void StreamingAudio::push(const float* stereo, std::size_t frames, const Consumer& consume) {
    for (std::size_t i = 0; i < frames; ++i) {
        for (int ch = 0; ch < 2; ++ch) {
            const float v = stereo[2*i+ch];
            impl_->pending[2*impl_->used+ch] = std::isfinite(v) ? std::clamp(v,-16.f,16.f) : 0;
        }
        if (++impl_->used == hop) {
            impl_->analyze();
            impl_->used = 0;
            consume(impl_->current, step);
        }
    }
}
Audio StreamingAudio::current() const { return impl_->current; }
double StreamingAudio::gain() const { return impl_->gain; }
}
