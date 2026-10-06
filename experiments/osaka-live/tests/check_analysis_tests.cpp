#include "../src/kit/check-analysis.h"
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <vector>

using namespace Journey::Kit::Check;

namespace {
void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

constexpr int W = 40, H = 20;
using Frame = std::vector<unsigned char>;

// Fills the first `rows` rows with a colour and the rest with `rest`.
Frame frame(int rows, unsigned char r, unsigned char g, unsigned char b, unsigned char rest = 0) {
    Frame f(W * H * 3, rest);
    for (int y = 0; y < rows; ++y)
        for (int x = 0; x < W; ++x) { f[(y * W + x) * 3] = r; f[(y * W + x) * 3 + 1] = g; f[(y * W + x) * 3 + 2] = b; }
    return f;
}

// An on/off flicker with `cycles` cycles per second for four seconds at 30 fps.
FlashResult flicker(FlashKind kind, int cycles, int rows, unsigned char r, unsigned char g, unsigned char b,
                    unsigned char rest = 0) {
    FlashAnalyzer analyzer(kind, W, H, 30);
    const Frame on = frame(rows, r, g, b, rest), off = frame(rows, rest, rest, rest, rest);
    for (int i = 0; i < 120; ++i) {
        const double phase = std::fmod(i * cycles / 30.0, 1.0);
        analyzer.add(phase < 0.5 ? on.data() : off.data());
    }
    return analyzer.result();
}
}

int main() {
    try {
        const FlashResult strobe = flicker(FlashKind::General, 5, H, 255, 255, 255);
        require(strobe.failed && strobe.worstArea > 0.99, "a full-screen 5 Hz strobe was not flagged");
        const FlashResult four = flicker(FlashKind::General, 4, H, 255, 255, 255);
        require(four.failed, "a full-screen 4 Hz flicker (four flashes a second) was not flagged");
        const FlashResult three = flicker(FlashKind::General, 3, H, 255, 255, 255);
        require(!three.failed, "three flashes a second must pass");
        require(three.worstFlashes >= 2.5 && three.worstFlashes <= 3.5, "three flashes a second was measured wrongly");
        const FlashResult small = flicker(FlashKind::General, 6, 4, 255, 255, 255);
        require(!small.failed && small.worstArea > 0.15 && small.worstArea < 0.25,
                "a flicker over a fifth of the frame must not exceed the area limit");
        const FlashResult large = flicker(FlashKind::General, 6, 6, 255, 255, 255);
        require(large.failed, "a flicker over 30% of the frame was not flagged");
        // Both sides at or above 0.80 relative luminance: not a flash.
        const FlashResult bright = flicker(FlashKind::General, 6, H, 255, 255, 255, 250);
        require(!bright.failed && bright.worstArea == 0, "a flicker between near-whites was flagged");
        const FlashResult faint = flicker(FlashKind::General, 6, H, 30, 30, 30, 0);
        require(!faint.failed, "a flicker under 10% luminance change was flagged");

        FlashAnalyzer fade(FlashKind::General, W, H, 30);
        for (int i = 0; i < 120; ++i) {
            const auto v = static_cast<unsigned char>(i * 2);
            const Frame f = frame(H, v, v, v);
            fade.add(f.data());
        }
        require(!fade.result().failed && fade.result().worstArea == 0, "a slow fade was flagged");

        const FlashResult red = flicker(FlashKind::Red, 5, H, 255, 0, 0);
        require(red.failed, "a saturated red strobe was not flagged as a red flash");
        const FlashResult greenStrobe = flicker(FlashKind::Red, 5, H, 0, 255, 0);
        require(!greenStrobe.failed, "a green strobe was flagged as red");
        const FlashResult whiteIsNotRed = flicker(FlashKind::Red, 5, H, 255, 255, 255);
        require(!whiteIsNotRed.failed, "a white strobe was flagged as red");

        const unsigned char black[3] = {0, 0, 0}, white[3] = {255, 255, 255};
        require(encodedBrightness(white, 1) > 0.999 && encodedBrightness(black, 1) == 0, "brightness range is wrong");
        require(meanBrightnessDifference(white, black, 1) > 0.999, "brightness difference is wrong");
        std::cout << "PASS: flash analysis counts flashes, area, darker-side and red rules\n";
    } catch (const std::exception& e) {
        std::cerr << "FAIL: " << e.what() << '\n';
        return 1;
    }
}
