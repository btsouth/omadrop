#pragma once

#include <cstdint>

struct CoverPresentationFrame {
    float coverMix = 0.0f;
    float paletteInfluence = 0.0f;
    bool complete = true;
};

class CoverPresentation {
public:
    CoverPresentation(float holdSeconds, float dissolveSeconds);

    void show(std::uint64_t nowMs);
    void restart(std::uint64_t nowMs);
    void clear();
    bool hasArtwork() const { return hasArtwork_; }
    CoverPresentationFrame frame(std::uint64_t nowMs) const;

private:
    float holdSeconds_ = 5.0f;
    float dissolveSeconds_ = 5.0f;
    bool hasArtwork_ = false;
    std::uint64_t startedAtMs_ = 0;
};
