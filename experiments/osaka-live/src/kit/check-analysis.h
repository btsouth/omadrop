#pragma once
// Frame analysis for the world check: brightness change and the WCAG 2.3.1
// flash tests. Pure functions over 8-bit RGB frames, with no GPU or Qt
// dependency, so the preview tool can run the same analysis later.
#include <cstddef>
#include <cstdint>
#include <vector>

namespace Journey::Kit::Check {

// Brightness as the fidelity capture defines it: Rec.709 weights on the
// encoded bytes, scaled to 0..1.
double encodedBrightness(const unsigned char* rgb, std::size_t pixels);
// Mean absolute per-pixel brightness difference between two same-size frames.
double meanBrightnessDifference(const unsigned char* a, const unsigned char* b, std::size_t pixels);

enum class FlashKind { General, Red };

// A flash is a pair of opposing changes. More than three flashes in one second
// (more than six opposing changes) over more than a quarter of the frame fails.
// WCAG measures the area in a 10 degree field of view at normal viewing
// distance; this check uses a quarter of the whole frame instead.
constexpr double FlashAreaLimit = 0.25;
constexpr int FlashesPerSecondLimit = 3;

struct FlashResult {
    FlashKind kind = FlashKind::General;
    int frames = 0;
    double fps = 0;
    bool failed = false;
    // Worst one-second window, in seconds from the first analysed frame.
    double worstStartSeconds = 0, worstEndSeconds = 0;
    // Fraction of the frame whose pixels each changed more than three times.
    double worstArea = 0;
    // Flashes per second that more than a quarter of the frame reached.
    double worstFlashes = 0;
};

// Feed frames in order at a steady rate. Every pixel is tracked on its own:
// a change counts once it moves at least 10% of full relative luminance (or,
// for red, more than 20 on (R-G-B)*320) away from its last turning point,
// and only when the darker side is below 0.80 (or one side is saturated red).
class FlashAnalyzer {
public:
    FlashAnalyzer(FlashKind kind, int width, int height, double fps);
    void add(const unsigned char* rgb);
    FlashResult result() const;
private:
    struct Pixel { float extreme = 0; std::int8_t direction = 0; bool extremeQualifies = false; };
    FlashKind kind_;
    std::size_t pixels_;
    int window_;
    double fps_;
    int frames_ = 0;
    std::vector<Pixel> state_;
    std::vector<std::uint8_t> count_;
    std::vector<std::uint32_t> histogram_;
    std::vector<std::vector<std::uint32_t>> expiring_;
    FlashResult worst_;
    int worstFrame_ = -1;
};
}
