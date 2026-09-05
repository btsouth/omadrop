#pragma once

#include "mpris_state.h"

#include <cstdint>
#include <chrono>
#include <filesystem>
#include <optional>
#include <string>

struct MprisPollResult {
    std::optional<MprisState> state;
    std::string error;
    std::uint64_t startedAtMs = 0;
    std::optional<std::string> artworkPath;
};

class MprisPoller {
public:
    explicit MprisPoller(std::filesystem::path helper);
    ~MprisPoller();
    MprisPoller(const MprisPoller&) = delete;
    MprisPoller& operator=(const MprisPoller&) = delete;

    bool start(bool skipArt, std::uint64_t nowMs);
    bool startArtwork(const std::string& url, std::uint64_t nowMs);
    std::optional<MprisPollResult> update();
    bool running() const { return childPid_ > 0; }
    void stop();

private:
    bool startCommand(const char* argument, bool artwork, std::uint64_t nowMs);
    bool artworkMode_ = false;
    std::filesystem::path helper_;
    int childPid_ = -1;
    int outputFd_ = -1;
    std::string output_;
    std::uint64_t startedAtMs_ = 0;
    std::chrono::steady_clock::time_point launchedAt_;
};
