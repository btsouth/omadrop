#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string_view>

inline constexpr std::string_view firstRunControlsLabel
    = "N NEXT  P PREVIOUS  A ASCII  I RESPONSE\n"
      "B BRIGHTNESS  M MOTION  D DIRECTOR\n"
      "R REDUCED  S FLASH  C COLOR SAFE  ESC EXIT";
inline constexpr std::uint64_t firstRunControlsDurationMs = 7000;

class FirstRunControls {
public:
    explicit FirstRunControls(bool enabled) : pending_(enabled) {}

    void onWindowShown(std::uint64_t nowMilliseconds, bool hasArtwork,
                       float coverHoldSeconds, float coverDissolveSeconds) {
        if (!pending_ || scheduledAt_ != 0) return;
        const float delaySeconds = hasArtwork
            ? std::max(0.0f, coverHoldSeconds)
                + std::max(0.0f, coverDissolveSeconds) + 0.5f
            : 1.5f;
        scheduledAt_ = nowMilliseconds + static_cast<std::uint64_t>(
            std::lround(delaySeconds * 1000.0f));
    }

    bool shouldShow(std::uint64_t nowMilliseconds) const {
        return pending_ && scheduledAt_ != 0
            && nowMilliseconds >= scheduledAt_;
    }

    void markShown() { pending_ = false; }
    bool pending() const { return pending_; }
    std::uint64_t scheduledAt() const { return scheduledAt_; }

private:
    bool pending_ = false;
    std::uint64_t scheduledAt_ = 0;
};
