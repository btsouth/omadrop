#include "signal_monitor.h"

#include <cassert>
#include <iostream>

int main() {
    constexpr int width = 640;
    constexpr int height = 360;
    const std::array<float, 3> base{0.4f, 0.3f, 0.2f};
    MusicFrame music;
    music.beatPhase = 0.5f;
    music.barPhase = 0.25f;
    music.clockConfidence = 0.75f;
    music.kick = 1.25f;
    music.rhythmicDensity = 0.6f;
    music.section = 0.8f;

    const SignalMonitorLayout layout = signalMonitorLayout(width, height);
    const auto untouched = signalMonitorPixel(
        music, width / 2, layout.panelTop - 1, width, height, base);
    assert(untouched == base);

    const int kickY = layout.panelTop + 3 * layout.laneHeight
                    + layout.laneHeight / 2;
    const int middleX = layout.trackStart
                      + (layout.trackEnd - layout.trackStart) / 2;
    const auto activeKick = signalMonitorPixel(
        music, middleX, kickY, width, height, base);
    MusicFrame quiet = music;
    quiet.kick = 0.0f;
    const auto quietKick = signalMonitorPixel(
        quiet, middleX, kickY, width, height, base);
    assert(activeKick[0] > quietKick[0] + 0.35f);

    const int beatY = layout.panelTop + layout.laneHeight / 2;
    const auto beatCursor = signalMonitorPixel(
        music, middleX, beatY, width, height, base);
    music.beatPhase = 0.15f;
    const auto beatTrack = signalMonitorPixel(
        music, middleX, beatY, width, height, base);
    assert(beatCursor[0] > beatTrack[0] + 0.45f);

    std::cout << "signal monitor passed\n";
    return 0;
}
