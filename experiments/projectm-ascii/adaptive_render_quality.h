#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>

class AdaptiveRenderQuality {
public:
    bool observe(double nativeMilliseconds, double compositorMilliseconds,
                 bool transitioning) {
        if (!std::isfinite(nativeMilliseconds)
            || !std::isfinite(compositorMilliseconds)
            || nativeMilliseconds < 0.0 || compositorMilliseconds < 0.0) {
            return false;
        }
        const double nativeBudget = transitioning ? 12.0 : 6.0;
        const double load = std::max(
            nativeMilliseconds / nativeBudget,
            (nativeMilliseconds + compositorMilliseconds) / 14.0);
        if (!initialized_) {
            smoothedLoad_ = load;
            initialized_ = true;
        } else {
            smoothedLoad_ += (load - smoothedLoad_) * 0.08;
        }

        if (cooldownFrames_ > 0) {
            --cooldownFrames_;
            return false;
        }

        if (load > 1.0 || smoothedLoad_ > 1.04) {
            overloadScore_ += load > 1.50 ? 4 : 1;
        } else {
            overloadScore_ = std::max(0, overloadScore_ - 2);
        }
        if (load < 0.42 && smoothedLoad_ < 0.48) {
            ++headroomFrames_;
        } else {
            headroomFrames_ = 0;
        }

        // Never alter detail in the middle of an authored transition. Carry a
        // sustained overload to the first settled frame instead.
        if (transitioning) return false;
        if (overloadScore_ >= 120 && level_ + 1 < qualityLevels.size()) {
            ++level_;
            overloadScore_ = 0;
            headroomFrames_ = 0;
            cooldownFrames_ = 300;
            return true;
        }
        if (headroomFrames_ >= 3600 && level_ > 0) {
            --level_;
            overloadScore_ = 0;
            headroomFrames_ = 0;
            cooldownFrames_ = 300;
            return true;
        }
        return false;
    }

    float quality() const { return qualityLevels[level_]; }
    std::size_t level() const { return level_; }
    double smoothedLoad() const { return smoothedLoad_; }

private:
    static constexpr std::array<float, 3> qualityLevels{1.0f, 0.72f, 0.50f};
    std::size_t level_ = 0;
    int overloadScore_ = 0;
    int headroomFrames_ = 0;
    int cooldownFrames_ = 0;
    double smoothedLoad_ = 0.0;
    bool initialized_ = false;
};
