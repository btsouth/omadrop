#include "live_settings.h"
#include <cassert>
#include <chrono>
#include <csignal>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <thread>
#include <unistd.h>

int main(int argc, char**) {
    if (argc > 1) {
        if (std::getenv("OMADROP_TEST_SINK_HANG")) {
            std::signal(SIGTERM, SIG_IGN);
            while (true) pause();
        }
        std::cout << "test-output\n";
        return 0;
    }
    unsetenv("OMADROP_AUDIO_SINK");
    unsetenv("OMADROP_TEST_SINK_PATH");
    const auto self = std::filesystem::canonical("/proc/self/exe");
    setenv("OMADROP_PACTL_COMMAND", self.c_str(), 1);
    setenv("OMADROP_TEST_SINK_HANG", "1", 1);
    const auto start = std::chrono::steady_clock::now();
    while (std::chrono::steady_clock::now() - start < std::chrono::milliseconds(900)) {
        const auto call = std::chrono::steady_clock::now();
        assert(defaultSinkName().empty());
        assert(std::chrono::steady_clock::now() - call < std::chrono::milliseconds(200));
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
    unsetenv("OMADROP_TEST_SINK_HANG");
    std::string recovered;
    for (int i = 0; i < 500 && recovered.empty(); ++i) {
        recovered = defaultSinkName();
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
    assert(recovered == "test-output");
    std::cout << "nonblocking audio-output query, timeout, and recovery passed\n";
}
