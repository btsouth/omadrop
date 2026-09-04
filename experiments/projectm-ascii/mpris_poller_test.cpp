#include "mpris_poller.h"

#include <cassert>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <string>
#include <thread>
#include <csignal>
#include <fstream>
#include <unistd.h>

namespace {
std::optional<MprisPollResult> waitFor(MprisPoller& poller) {
    for (int attempt = 0; attempt < 100; ++attempt) {
        if (auto result = poller.update()) return result;
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
    return std::nullopt;
}
}

int main(int argc, char** argv) {
    if (argc == 2 && std::string(argv[1]) == "--no-art") {
        if (const char* ready = std::getenv("OMADROP_TEST_MPRIS_STUBBORN")) {
            std::signal(SIGTERM, SIG_IGN);
            std::ofstream(ready) << "ready\n";
            while (true) pause();
        }
        if (const char* delay = std::getenv("OMADROP_TEST_MPRIS_DELAY_MS")) {
            std::this_thread::sleep_for(
                std::chrono::milliseconds(std::atoi(delay)));
        }
        const char* output = std::getenv("OMADROP_TEST_MPRIS_OUTPUT");
        if (output) std::cout << output << "\n";
        return 0;
    }

    MprisPoller poller(argv[0]);
    const char* valid = R"({"identity":"track-a","playback_status":"Playing","art_path":"","position_seconds":12.5,"duration_seconds":180})";
    setenv("OMADROP_TEST_MPRIS_OUTPUT", valid, 1);
    assert(poller.start(true, 250));
    assert(poller.running());
    assert(!poller.start(true, 260));
    const auto result = waitFor(poller);
    assert(result && result->state && result->error.empty());
    assert(result->startedAtMs == 250);
    assert(result->state->identity == "track-a");
    assert(!poller.running());

    setenv("OMADROP_TEST_MPRIS_OUTPUT", "not-json", 1);
    assert(poller.start(true, 500));
    const auto invalid = waitFor(poller);
    assert(invalid && !invalid->state && !invalid->error.empty());

    unsetenv("OMADROP_TEST_MPRIS_OUTPUT");
    assert(poller.start(true, 750));
    const auto empty = waitFor(poller);
    assert(empty && !empty->state && empty->error.empty());

    setenv("OMADROP_TEST_MPRIS_DELAY_MS", "5000", 1);
    assert(poller.start(true, 1000));
    poller.stop();
    assert(!poller.running());
    unsetenv("OMADROP_TEST_MPRIS_DELAY_MS");

    const auto ready = std::filesystem::temp_directory_path()
        / ("omadrop-stubborn-mpris-" + std::to_string(getpid()));
    setenv("OMADROP_TEST_MPRIS_STUBBORN", ready.c_str(), 1);
    assert(poller.start(true, 1100));
    for (int i = 0; i < 100 && !std::filesystem::exists(ready); ++i)
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    assert(std::filesystem::exists(ready));
    const auto started = std::chrono::steady_clock::now();
    poller.stop();
    assert(std::chrono::steady_clock::now() - started < std::chrono::milliseconds(500));
    assert(!poller.running());
    assert(poller.start(true, 1200));
    std::optional<MprisPollResult> timedOut;
    for (int i = 0; i < 600 && !timedOut; ++i) {
        timedOut = poller.update();
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
    assert(timedOut && timedOut->error == "MPRIS helper timed out");
    unsetenv("OMADROP_TEST_MPRIS_STUBBORN");
    std::filesystem::remove(ready);

    MprisPoller missing("/definitely/missing/omadrop-mpris-helper");
    assert(missing.start(true, 1250));
    const auto failed = waitFor(missing);
    assert(failed && !failed->state && !failed->error.empty());

    std::cout << "MPRIS poller passed\n";
    return 0;
}
