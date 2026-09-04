#include "audio_output_session.h"
#include "pipewire_capture.h"

#include <cassert>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <thread>
#include <unistd.h>
#include <vector>

namespace {
bool waitForText(const std::filesystem::path& path, const std::string& expected) {
    for (int attempt = 0; attempt < 100; ++attempt) {
        std::ifstream input(path);
        const std::string contents{std::istreambuf_iterator<char>(input),
                                   std::istreambuf_iterator<char>()};
        if (contents.find(expected) != std::string::npos) return true;
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
    return false;
}
} // namespace

int main(int argc, char** argv) {
    assert(argc == 2);
    const std::filesystem::path root = std::filesystem::temp_directory_path()
        / ("omadrop-audio-output-test-" + std::to_string(getpid()));
    std::filesystem::create_directories(root);
    const auto log = root / "capture.log";
    setenv("OMADROP_PW_RECORD_COMMAND", argv[1], 1);
    setenv("OMADROP_TEST_CAPTURE_LOG", log.c_str(), 1);

    PipeWireCapture capture;
    std::string sink = "alsa_output.speakers";
    assert(capture.start(sink));
    assert(waitForText(log, "--target alsa_output.speakers"));

    unsigned int syncDelayMs = 35;
    std::vector<std::string> order;
    const auto stop = [&] {
        order.emplace_back("stop");
        capture.stop();
    };
    const auto reset = [&](const std::string& newSink) {
        order.emplace_back("reset:" + newSink);
        syncDelayMs = newSink.rfind("bluez_", 0) == 0 ? 180u : 35u;
    };
    const auto start = [&](const std::string& newSink) {
        order.emplace_back("start:" + newSink);
        return capture.start(newSink);
    };

    assert(followAudioOutput("", sink, stop, reset, start)
           == AudioOutputFollowResult::Unchanged);
    assert(followAudioOutput(sink, sink, stop, reset, start)
           == AudioOutputFollowResult::Unchanged);
    assert(order.empty());

    const std::string headphones = "bluez_output.headphones";
    assert(followAudioOutput(headphones, sink, stop, reset, start)
           == AudioOutputFollowResult::Followed);
    assert(sink == headphones && syncDelayMs == 180u);
    assert((order == std::vector<std::string>{
        "stop", "reset:bluez_output.headphones",
        "start:bluez_output.headphones"}));
    assert(waitForText(log, "--target bluez_output.headphones"));
    assert(waitForText(log, "terminated"));

    capture.stop();
    setenv("OMADROP_PW_RECORD_COMMAND",
           "/omadrop-test/nonexistent-pw-record", 1);
    order.clear();
    assert(followAudioOutput("alsa_output.hdmi", sink, stop, reset, start)
           == AudioOutputFollowResult::Failed);
    assert(sink == "alsa_output.hdmi" && syncDelayMs == 35u);

    unsetenv("OMADROP_PW_RECORD_COMMAND");
    unsetenv("OMADROP_TEST_CAPTURE_LOG");
    std::error_code error;
    std::filesystem::remove_all(root, error);
    std::cout << "audio output session passed\n";
}
