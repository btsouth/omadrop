#include "display_session.h"

#include <algorithm>
#include <fstream>
#include <string>
#include <unistd.h>
#include <utility>

DisplaySession::DisplaySession(DisplaySessionConfig config)
    : config_(std::move(config)),
      closeDurationMs_(config_.recordStopPath.empty() ? 420u : 920u),
      startGateOpen_(config_.startGatePath.empty()) {
    if (!config_.startGatePath.empty()) {
        displayReadyPath_ = config_.startGatePath.string() + "."
            + std::to_string(config_.displayIndex) + ".ready";
    }
}

bool DisplaySession::writeMarker(const std::filesystem::path& path) {
    if (path.empty()) return true;
    std::ofstream marker(path, std::ios::trunc);
    if (!marker) return false;
    marker << getpid() << '\n';
    return static_cast<bool>(marker);
}

float DisplaySession::eased(float value) {
    const float position = std::clamp(value, 0.0f, 1.0f);
    return position * position * (3.0f - 2.0f * position);
}

bool DisplaySession::pollStartGate(std::uint64_t nowMs) {
    if (startGateOpen_ || config_.startGatePath.empty()) return false;
    std::error_code error;
    if (!std::filesystem::exists(config_.startGatePath, error) || error) {
        return false;
    }
    startGateOpen_ = true;
    revealStarted_ = true;
    revealStartedAtMs_ = nowMs;
    return true;
}

bool DisplaySession::afterFramePresented(std::uint64_t nowMs,
                                         bool presentationReady,
                                         bool pairedFrameReady) {
    if (windowShown_ || !presentationReady || !pairedFrameReady) return false;
    if (config_.immediateReveal && !startGateOpen_) return false;
    writeMarker(displayReadyPath_);
    writeMarker(config_.readyPath);
    if (config_.startGatePath.empty()) {
        revealStarted_ = true;
        revealStartedAtMs_ = nowMs;
    }
    windowShown_ = true;
    return true;
}

void DisplaySession::prepareFirstFrame(bool pairedFrameReady) {
    if (framePrepared_ || !pairedFrameReady) return;
    framePrepared_ = true;
    writeMarker(displayReadyPath_);
}

void DisplaySession::requestClose(std::uint64_t nowMs) {
    if (closing_) return;
    closing_ = true;
    closeStartedAtMs_ = nowMs;
}

bool DisplaySession::closeComplete(std::uint64_t nowMs) const {
    return closing_ && nowMs >= closeStartedAtMs_
        && nowMs - closeStartedAtMs_ >= closeDurationMs_;
}

void DisplaySession::signalRecordingComplete() {
    if (recordingCompleteSignaled_) return;
    writeMarker(config_.recordStopPath);
    recordingCompleteSignaled_ = true;
}

float DisplaySession::visibility(std::uint64_t nowMs) const {
    const float entrance = config_.immediateReveal ? 1.0f : startGateOpen_ && revealStarted_
        ? eased((nowMs >= revealStartedAtMs_
            ? nowMs - revealStartedAtMs_ : 0u) / 480.0f)
        : 0.0f;
    const float exit = closing_
        ? 1.0f - eased((nowMs >= closeStartedAtMs_
            ? nowMs - closeStartedAtMs_ : 0u)
            / static_cast<float>(closeDurationMs_))
        : 1.0f;
    return entrance * exit;
}
