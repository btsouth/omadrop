#pragma once

#include "music_frame.h"

#include <algorithm>
#include <array>
#include <cmath>

struct SignalMonitorLayout {
    int panelTop = 0;
    int panelHeight = 0;
    int laneHeight = 0;
    int margin = 0;
    int keyWidth = 0;
    int trackStart = 0;
    int trackEnd = 0;
};

inline SignalMonitorLayout signalMonitorLayout(int width, int height) {
    SignalMonitorLayout layout;
    layout.panelHeight = std::clamp(height / 5, 64, 180);
    layout.panelTop = height - layout.panelHeight;
    layout.laneHeight = std::max(1, layout.panelHeight / 8);
    layout.margin = std::max(6, width / 160);
    layout.keyWidth = std::max(8, width / 80);
    layout.trackStart = layout.margin + layout.keyWidth + layout.margin;
    layout.trackEnd = width - layout.margin;
    return layout;
}

inline std::array<float, 3> mixSignalMonitorColor(
    const std::array<float, 3>& base, const std::array<float, 3>& overlay,
    float amount) {
    std::array<float, 3> result{};
    for (int channel = 0; channel < 3; ++channel) {
        result[channel] = base[channel] * (1.0f - amount)
                        + overlay[channel] * amount;
    }
    return result;
}

inline std::array<float, 3> signalMonitorPixel(
    const MusicFrame& music, int x, int y, int width, int height,
    const std::array<float, 3>& base) {
    const SignalMonitorLayout layout = signalMonitorLayout(width, height);
    if (y < layout.panelTop || x < 0 || x >= width) return base;

    const std::array<std::array<float, 3>, 8> colors{{
        {0.96f, 0.96f, 0.96f},
        {0.45f, 0.72f, 1.00f},
        {0.20f, 0.48f, 1.00f},
        {1.00f, 0.22f, 0.12f},
        {0.18f, 0.82f, 1.00f},
        {1.00f, 0.82f, 0.12f},
        {0.30f, 0.92f, 0.48f},
        {0.92f, 0.25f, 0.86f},
    }};
    const std::array<float, 8> values{{
        std::clamp(music.beatPhase, 0.0f, 1.0f),
        std::clamp(music.barPhase, 0.0f, 1.0f),
        std::clamp(music.clockConfidence, 0.0f, 1.0f),
        std::clamp(music.kick / 1.25f, 0.0f, 1.0f),
        std::clamp(music.snare / 1.25f, 0.0f, 1.0f),
        std::clamp(music.hat / 1.25f, 0.0f, 1.0f),
        std::clamp(music.rhythmicDensity, 0.0f, 1.0f),
        std::clamp(std::max(music.section, music.novelty), 0.0f, 1.0f),
    }};

    std::array<float, 3> result = mixSignalMonitorColor(
        base, {0.006f, 0.008f, 0.014f}, 0.88f);
    const int lane = std::clamp(
        (y - layout.panelTop) / layout.laneHeight, 0, 7);
    const int laneTop = layout.panelTop + lane * layout.laneHeight;
    const int laneCenter = laneTop + layout.laneHeight / 2;
    const int halfThickness = std::max(1, layout.laneHeight / 5);
    const bool insideStroke = std::abs(y - laneCenter) <= halfThickness;

    if (x >= layout.margin && x < layout.margin + layout.keyWidth
        && insideStroke) {
        return mixSignalMonitorColor(result, colors[lane], 0.92f);
    }
    if (x < layout.trackStart || x > layout.trackEnd || !insideStroke) {
        if (y == laneTop && x >= layout.margin && x <= layout.trackEnd) {
            return mixSignalMonitorColor(result, {0.22f, 0.24f, 0.30f}, 0.28f);
        }
        return result;
    }

    result = mixSignalMonitorColor(result, colors[lane], 0.10f);
    const int trackWidth = std::max(1, layout.trackEnd - layout.trackStart);
    if (lane < 2) {
        const int cursor = layout.trackStart
            + static_cast<int>(std::round(values[lane] * trackWidth));
        const int cursorWidth = std::max(2, width / 320);
        if (std::abs(x - cursor) <= cursorWidth) {
            result = mixSignalMonitorColor(result, colors[lane], 0.96f);
        }
    } else {
        const int fillEnd = layout.trackStart
            + static_cast<int>(std::round(values[lane] * trackWidth));
        if (x <= fillEnd) {
            result = mixSignalMonitorColor(result, colors[lane], 0.82f);
        }
    }
    return result;
}
