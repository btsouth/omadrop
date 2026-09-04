#include "display_session.h"

#include <cassert>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <unistd.h>

namespace {
std::filesystem::path testDirectory(const std::string& suffix) {
    const auto path = std::filesystem::temp_directory_path()
        / ("omadrop-display-session-" + std::to_string(getpid()) + "-" + suffix);
    std::filesystem::remove_all(path);
    std::filesystem::create_directories(path);
    return path;
}

bool near(float first, float second) {
    return std::abs(first - second) < 0.001f;
}
}

int main() {
    const auto directDirectory = testDirectory("direct");
    const auto directReady = directDirectory / "ready";
    DisplaySession direct({
        .startGatePath = {},
        .readyPath = directReady,
        .recordStopPath = {},
        .displayIndex = 0,
    });
    assert(direct.startGateOpen());
    assert(!direct.windowShown());
    assert(!direct.afterFramePresented(100, false, true));
    assert(!std::filesystem::exists(directReady));
    assert(direct.afterFramePresented(120, true, true));
    assert(direct.windowShown());
    assert(std::filesystem::exists(directReady));
    assert(near(direct.visibility(120), 0.0f));
    assert(near(direct.visibility(360), 0.5f));
    assert(near(direct.visibility(600), 1.0f));
    direct.requestClose(1000);
    assert(direct.closing());
    assert(!direct.closeComplete(1419));
    assert(direct.closeComplete(1420));
    assert(near(direct.visibility(1420), 0.0f));

    DisplaySession zeroClock({});
    assert(zeroClock.afterFramePresented(0, true, true));
    assert(near(zeroClock.visibility(0), 0.0f));
    assert(near(zeroClock.visibility(480), 1.0f));

    const auto pairedDirectory = testDirectory("paired");
    const auto gate = pairedDirectory / "gate";
    const auto ready = pairedDirectory / "capture-ready";
    const auto recordStop = pairedDirectory / "record-stop";
    DisplaySession paired({
        .startGatePath = gate,
        .readyPath = ready,
        .recordStopPath = recordStop,
        .displayIndex = 2,
    });
    assert(!paired.startGateOpen());
    assert(!paired.pollStartGate(100));
    assert(!paired.afterFramePresented(120, true, false));
    assert(paired.afterFramePresented(140, true, true));
    assert(std::filesystem::exists(gate.string() + ".2.ready"));
    assert(std::filesystem::exists(ready));
    assert(near(paired.visibility(200), 0.0f));
    {
        std::ofstream openGate(gate);
        openGate << "go\n";
    }
    assert(paired.pollStartGate(300));
    assert(paired.startGateOpen());
    assert(near(paired.visibility(540), 0.5f));
    assert(!paired.pollStartGate(600));
    paired.signalRecordingComplete();
    paired.signalRecordingComplete();
    assert(std::filesystem::exists(recordStop));
    paired.requestClose(1000);
    assert(!paired.closeComplete(1919));
    assert(paired.closeComplete(1920));

    std::filesystem::remove_all(directDirectory);
    std::filesystem::remove_all(pairedDirectory);
    std::cout << "display session passed\n";
}
