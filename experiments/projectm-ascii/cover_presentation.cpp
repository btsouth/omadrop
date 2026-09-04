#include "cover_presentation.h"

#include <algorithm>
#include <cmath>

CoverPresentation::CoverPresentation(float holdSeconds, float dissolveSeconds)
    : holdSeconds_(std::max(0.0f, holdSeconds)),
      dissolveSeconds_(std::max(0.001f, dissolveSeconds)) {}

void CoverPresentation::show(std::uint64_t nowMs) {
    hasArtwork_ = true;
    startedAtMs_ = nowMs;
}

void CoverPresentation::restart(std::uint64_t nowMs) {
    if (hasArtwork_) startedAtMs_ = nowMs;
}

void CoverPresentation::clear() {
    hasArtwork_ = false;
    startedAtMs_ = 0;
}

CoverPresentationFrame CoverPresentation::frame(std::uint64_t nowMs) const {
    if (!hasArtwork_) return {};
    const float age = nowMs >= startedAtMs_
        ? (nowMs - startedAtMs_) / 1000.0f : 0.0f;
    CoverPresentationFrame result;
    result.complete = age >= holdSeconds_ + dissolveSeconds_;
    if (age < holdSeconds_) {
        result.coverMix = 1.0f;
    } else if (!result.complete) {
        const float position = std::clamp(
            (age - holdSeconds_) / dissolveSeconds_, 0.0f, 1.0f);
        const float eased = position * position * (3.0f - 2.0f * position);
        result.coverMix = 1.0f - eased;
    }
    result.paletteInfluence = 0.12f + 0.28f * std::exp(
        -std::max(0.0f, age - holdSeconds_) / 22.0f);
    return result;
}
