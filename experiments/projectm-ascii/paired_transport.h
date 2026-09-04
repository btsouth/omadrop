#pragma once

#include "paired_display.h"
#include "paired_music_state.h"

#include <filesystem>
#include <optional>
#include <string>

class PairedTransport {
public:
    explicit PairedTransport(std::filesystem::path statePath);

    bool enabled() const;
    bool publishDisplay(const PairedDisplayState& state) const;
    bool publishMusic(const PairedMusicState& state) const;
    std::string readDisplay() const;
    std::string readMusic() const;
    bool publishRequest(const std::string& request) const;
    std::optional<std::string> consumeRequest() const;

    const std::filesystem::path& statePath() const { return statePath_; }
    const std::filesystem::path& musicPath() const { return musicPath_; }
    const std::filesystem::path& requestPath() const { return requestPath_; }

private:
    bool writeAtomic(const std::filesystem::path& path,
                     const std::string& contents, bool binary) const;
    std::string read(const std::filesystem::path& path, bool binary) const;

    std::filesystem::path statePath_;
    std::filesystem::path musicPath_;
    std::filesystem::path requestPath_;
};
