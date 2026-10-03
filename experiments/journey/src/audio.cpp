#include "audio.h"
#include "../../projectm-ascii/audio_features.h"

#include <QFile>
#include <QtEndian>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <limits>

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
            value = -std::expm1(-8.0 * std::sqrt(value) / window);
        return power;
    }
private:
    std::array<std::array<float, window>, 2> history_{};
    std::array<float, window> hann_{}, input_{};
    std::array<std::array<float, 2>, window / 2 + 1> output_{};
    fftwf_plan plan_ = nullptr;
};
}

bool AudioTimeline::load(const QString& path, double offset, double duration, QString& error) {
    frames_.clear();
    offset_ = 0;
    error.clear();
    if (!std::isfinite(offset) || !std::isfinite(duration) || offset < 0
        || duration <= 0 || offset + duration > 21600) {
        error = QStringLiteral("Audio offset/duration must be finite, nonnegative and total at most 21600 seconds.");
        return false;
    }
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        error = QStringLiteral("Cannot read audio '%1': %2").arg(path, file.errorString());
        return false;
    }
    constexpr qint64 bytesPerFrame = 2 * sizeof(float);
    if (file.size() == 0 || file.size() % bytesPerFrame != 0) {
        error = QStringLiteral("Audio must contain complete stereo float32 little-endian frames (8 bytes each).");
        return false;
    }
    const qint64 needed = qint64(std::ceil((offset + duration) * rate - 1e-7));
    if (file.size() / bytesPerFrame < needed) {
        error = QStringLiteral("Audio is too short: %1 seconds available; need %2 seconds for offset + duration.")
            .arg(double(file.size() / bytesPerFrame) / rate, 0, 'f', 6)
            .arg(offset + duration, 0, 'f', 6);
        return false;
    }
    AudioFeatureBus leftAnalyzer, rightAnalyzer;
    AbsoluteBands absolute;
    if (!absolute.valid()) {
        error = QStringLiteral("Cannot create the FFTW analysis plan.");
        return false;
    }
    std::vector<Frame> result;
    result.reserve(std::size_t(needed / hop + 2));
    result.push_back({0, {}});
    Audio current;
    std::array<float, hop * 2> samples{};
    std::array<std::array<float, hop * 2>, 2> monoStereo{};
    qint64 processed = 0;
    while (processed < needed) {
        const int count = int(std::min<qint64>(hop, needed - processed));
        const QByteArray bytes = file.read(count * bytesPerFrame);
        if (bytes.size() != count * bytesPerFrame) {
            error = QStringLiteral("Audio read failed near sample %1: %2").arg(processed).arg(file.errorString());
            return false;
        }
        for (int i = 0; i < count * 2; ++i) {
            const quint32 bits = qFromLittleEndian<quint32>(bytes.constData() + i * sizeof(float));
            std::memcpy(&samples[i], &bits, sizeof(float));
            if (!std::isfinite(samples[i]) || std::abs(samples[i]) > 16.0f) {
                error = QStringLiteral("Invalid float32 audio at stereo sample %1 (nonfinite or magnitude over 16).")
                    .arg(processed + i / 2);
                return false;
            }
        }
        // The bus downmixes L+R. Give each bus duplicated mono so stereo phase
        // cannot cancel bass or onsets, while keeping its existing analysis.
        for (int i = 0; i < count; ++i) {
            for (int channel = 0; channel < 2; ++channel) {
                monoStereo[channel][2 * i] = samples[2 * i + channel];
                monoStereo[channel][2 * i + 1] = samples[2 * i + channel];
            }
        }
        const auto& left = leftAnalyzer.processStereo(monoStereo[0].data(), count);
        const auto& right = rightAnalyzer.processStereo(monoStereo[1].data(), count);
        const auto bands = absolute.process(samples.data(), count);
        const double dt = count == hop ? step : double(count) / rate;
        for (std::size_t i = 0; i < bands.size(); ++i)
            current.bands[i] = smooth(current.bands[i], bands[i], dt);
        const double bass = std::sqrt((double(left.bassBody) * left.bassBody
            + double(right.bassBody) * right.bassBody) * 0.5);
        current.bass = smooth(current.bass, std::clamp(bass, 0.0, 1.0), dt);
        // Keep an onset audible in either channel, on the original impact scale.
        const double impact = std::max({double(left.kickImpact),
            double(left.snareImpact), double(left.hatImpact), double(right.kickImpact),
            double(right.snareImpact), double(right.hatImpact)}) / 1.35;
        current.accent = smooth(current.accent, std::clamp(impact, 0.0, 1.0), dt, 0.025, 0.13);
        processed += count;
        result.push_back({double(processed) / rate, current});
    }
    frames_ = std::move(result);
    offset_ = offset;
    return true;
}

Audio AudioTimeline::at(double movieSeconds) const {
    if (frames_.empty() || !std::isfinite(movieSeconds)) return {};
    const double seconds = std::clamp(movieSeconds + offset_, 0.0, frames_.back().seconds);
    const auto right = std::upper_bound(frames_.begin(), frames_.end(), seconds,
        [](double time, const Frame& frame) { return time < frame.seconds; });
    if (right == frames_.end()) return frames_.back().audio;
    if (right == frames_.begin()) return right->audio;
    const auto& left = *(right - 1);
    const double amount = (seconds - left.seconds) / (right->seconds - left.seconds);
    Audio value;
    for (std::size_t i = 0; i < value.bands.size(); ++i)
        value.bands[i] = left.audio.bands[i] + amount * (right->audio.bands[i] - left.audio.bands[i]);
    value.bass = left.audio.bass + amount * (right->audio.bass - left.audio.bass);
    value.accent = left.audio.accent + amount * (right->audio.accent - left.audio.accent);
    return value;
}
}
