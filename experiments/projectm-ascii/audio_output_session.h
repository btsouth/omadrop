#pragma once

#include <functional>
#include <string>

enum class AudioOutputFollowResult {
    Unchanged,
    Followed,
    Failed,
};

AudioOutputFollowResult followAudioOutput(
    const std::string& candidateSink,
    std::string& activeSink,
    const std::function<void()>& stopCapture,
    const std::function<void(const std::string&)>& resetForSink,
    const std::function<bool(const std::string&)>& startCapture);
