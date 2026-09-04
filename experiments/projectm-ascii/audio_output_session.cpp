#include "audio_output_session.h"

AudioOutputFollowResult followAudioOutput(
    const std::string& candidateSink,
    std::string& activeSink,
    const std::function<void()>& stopCapture,
    const std::function<void(const std::string&)>& resetForSink,
    const std::function<bool(const std::string&)>& startCapture) {
    if (candidateSink.empty() || candidateSink == activeSink) {
        return AudioOutputFollowResult::Unchanged;
    }

    stopCapture();
    activeSink = candidateSink;
    resetForSink(activeSink);
    return startCapture(activeSink)
        ? AudioOutputFollowResult::Followed
        : AudioOutputFollowResult::Failed;
}
