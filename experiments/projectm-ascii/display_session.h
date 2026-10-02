#pragma once

#include <cstdint>
#include <filesystem>

struct DisplaySessionConfig {
    std::filesystem::path startGatePath;
    std::filesystem::path readyPath;
    std::filesystem::path recordStopPath;
    int displayIndex = 0;
    bool immediateReveal = false;
};

class DisplaySession {
public:
    explicit DisplaySession(DisplaySessionConfig config);

    bool pollStartGate(std::uint64_t nowMs);
    void prepareFirstFrame(bool pairedFrameReady);
    bool framePrepared() const { return framePrepared_; }
    bool afterFramePresented(std::uint64_t nowMs,
                             bool presentationReady,
                             bool pairedFrameReady);
    void requestClose(std::uint64_t nowMs);
    bool closeComplete(std::uint64_t nowMs) const;
    void signalRecordingComplete();

    float visibility(std::uint64_t nowMs) const;
    bool startGateOpen() const { return startGateOpen_; }
    bool windowShown() const { return windowShown_; }
    bool closing() const { return closing_; }

private:
    static bool writeMarker(const std::filesystem::path& path);
    static float eased(float value);

    DisplaySessionConfig config_;
    std::filesystem::path displayReadyPath_;
    std::uint64_t closeDurationMs_ = 420;
    std::uint64_t revealStartedAtMs_ = 0;
    std::uint64_t closeStartedAtMs_ = 0;
    bool startGateOpen_ = false;
    bool revealStarted_ = false;
    bool windowShown_ = false;
    bool closing_ = false;
    bool recordingCompleteSignaled_ = false;
    bool framePrepared_ = false;
};
