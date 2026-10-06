#include "check-analysis.h"
#include <algorithm>
#include <array>
#include <cmath>

namespace Journey::Kit::Check {
namespace {
const std::array<float, 256>& linearTable() {
    static const auto table = [] {
        std::array<float, 256> t{};
        for (int i = 0; i < 256; ++i) {
            const double c = i / 255.0;
            t[i] = float(c <= 0.04045 ? c / 12.92 : std::pow((c + 0.055) / 1.055, 2.4));
        }
        return t;
    }();
    return table;
}
constexpr float GeneralStep = 0.10f;      // 10% of the maximum relative luminance
constexpr float GeneralDarkerLimit = 0.80f;
constexpr float RedStep = 20.f;           // change in (R-G-B)*320
constexpr float RedShare = 0.80f;         // R/(R+G+B) for a saturated red
constexpr int OpposingChangesLimit = 2 * FlashesPerSecondLimit;
}

double encodedBrightness(const unsigned char* rgb, std::size_t pixels) {
    double total = 0;
    for (std::size_t i = 0; i < pixels; ++i)
        total += (0.2126 * rgb[3 * i] + 0.7152 * rgb[3 * i + 1] + 0.0722 * rgb[3 * i + 2]) / 255.0;
    return pixels ? total / pixels : 0;
}

double meanBrightnessDifference(const unsigned char* a, const unsigned char* b, std::size_t pixels) {
    double total = 0;
    for (std::size_t i = 0; i < pixels; ++i) {
        const double x = 0.2126 * a[3 * i] + 0.7152 * a[3 * i + 1] + 0.0722 * a[3 * i + 2];
        const double y = 0.2126 * b[3 * i] + 0.7152 * b[3 * i + 1] + 0.0722 * b[3 * i + 2];
        total += std::abs(x - y) / 255.0;
    }
    return pixels ? total / pixels : 0;
}

FlashAnalyzer::FlashAnalyzer(FlashKind kind, int width, int height, double fps)
    : kind_(kind), pixels_(std::size_t(width) * height), window_(std::max(1, int(std::lround(fps)))),
      fps_(fps), state_(pixels_), count_(pixels_, 0), histogram_(256, 0), expiring_(window_) {
    histogram_[0] = std::uint32_t(pixels_);
    worst_.kind = kind;
    worst_.fps = fps;
}

void FlashAnalyzer::add(const unsigned char* rgb) {
    const auto& linear = linearTable();
    const int frame = frames_++;
    // Changes that happened a full window ago leave the one-second count.
    auto& leaving = expiring_[frame % window_];
    for (auto p : leaving) {
        --histogram_[count_[p]];
        ++histogram_[--count_[p]];
    }
    leaving.clear();
    const float step = kind_ == FlashKind::General ? GeneralStep : RedStep;
    for (std::size_t p = 0; p < pixels_; ++p) {
        const float r = linear[rgb[3 * p]], g = linear[rgb[3 * p + 1]], b = linear[rgb[3 * p + 2]];
        float value;
        bool qualifies;
        if (kind_ == FlashKind::General) {
            value = 0.2126f * r + 0.7152f * g + 0.0722f * b;
            qualifies = value < GeneralDarkerLimit;
        } else {
            const float sum = r + g + b;
            value = (r - g - b) * 320.f;
            qualifies = sum > 0 && r / sum >= RedShare;
        }
        auto& s = state_[p];
        if (frame == 0) { s.extreme = value; s.extremeQualifies = qualifies; continue; }
        bool change = false;
        if (s.direction == 0) {
            if (std::abs(value - s.extreme) >= step && (qualifies || s.extremeQualifies)) {
                s.direction = value > s.extreme ? 1 : -1;
                change = true;
            }
        } else if (s.direction > 0 ? value > s.extreme : value < s.extreme) {
            s.extreme = value; s.extremeQualifies = qualifies;
            continue;
        } else if (std::abs(value - s.extreme) >= step && (qualifies || s.extremeQualifies)) {
            s.direction = std::int8_t(-s.direction);
            change = true;
        }
        if (!change) continue;
        s.extreme = value; s.extremeQualifies = qualifies;
        const unsigned c = count_[p];
        if (c < 255) {
            --histogram_[c]; ++histogram_[c + 1]; ++count_[p];
            expiring_[frame % window_].push_back(std::uint32_t(p));
        }
    }
    // Fraction of the frame with more than three flashes, and the highest
    // flash count that more than a quarter of the frame has reached.
    std::uint64_t above = 0;
    for (unsigned c = OpposingChangesLimit + 1; c < histogram_.size(); ++c) above += histogram_[c];
    const double area = double(above) / pixels_;
    unsigned reached = 0;
    std::uint64_t cumulative = 0;
    for (unsigned c = unsigned(histogram_.size()) - 1; c > 0; --c) {
        cumulative += histogram_[c];
        if (double(cumulative) / pixels_ > FlashAreaLimit) { reached = c; break; }
    }
    const bool worse = area > worst_.worstArea
        || (area == worst_.worstArea && reached / 2.0 > worst_.worstFlashes);
    if (worse || worstFrame_ < 0) {
        worst_.worstArea = area;
        worst_.worstFlashes = reached / 2.0;
        worstFrame_ = frame;
    }
}

FlashResult FlashAnalyzer::result() const {
    FlashResult r = worst_;
    r.frames = frames_;
    r.failed = worst_.worstArea > FlashAreaLimit;
    if (worstFrame_ >= 0) {
        r.worstEndSeconds = worstFrame_ / fps_;
        r.worstStartSeconds = std::max(0.0, (worstFrame_ + 1) / fps_ - 1.0);
    }
    return r;
}
}
