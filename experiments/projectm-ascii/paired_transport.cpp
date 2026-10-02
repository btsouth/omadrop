#include "paired_transport.h"

#include <fstream>
#include <iterator>
#include <system_error>
#include <utility>
#include <unistd.h>

PairedTransport::PairedTransport(std::filesystem::path statePath)
    : statePath_(std::move(statePath)) {
    if (!statePath_.empty()) {
        musicPath_ = statePath_.string() + ".music";
        requestPath_ = statePath_.string() + ".request";
    }
}

bool PairedTransport::enabled() const {
    return !statePath_.empty();
}

bool PairedTransport::writeAtomic(const std::filesystem::path& path,
                                  const std::string& contents,
                                  bool binary) const {
    if (path.empty()) return false;
    const std::filesystem::path temporary = path.string() + "."
        + std::to_string(getpid()) + ".tmp";
    const auto mode = std::ios::out | std::ios::trunc
        | (binary ? std::ios::binary : std::ios::openmode{});
    std::ofstream output(temporary, mode);
    if (!output) return false;
    output.write(contents.data(), static_cast<std::streamsize>(contents.size()));
    output.close();
    if (!output) {
        std::error_code cleanupError;
        std::filesystem::remove(temporary, cleanupError);
        return false;
    }
    std::error_code error;
    std::filesystem::rename(temporary, path, error);
    if (error) std::filesystem::remove(temporary, error);
    return !error;
}

std::string PairedTransport::read(const std::filesystem::path& path,
                                  bool binary) const {
    if (path.empty()) return {};
    const auto mode = std::ios::in
        | (binary ? std::ios::binary : std::ios::openmode{});
    std::ifstream input(path, mode);
    return {std::istreambuf_iterator<char>(input),
            std::istreambuf_iterator<char>()};
}

bool PairedTransport::publishDisplay(const PairedDisplayState& state) const {
    return writeAtomic(statePath_, encodePairedDisplayState(state), false);
}

bool PairedTransport::publishMusic(const PairedMusicState& state) const {
    return writeAtomic(musicPath_, encodePairedMusicState(state), true);
}

const std::string& PairedTransport::readDisplay() const {
    struct stat current{};
    if (statePath_.empty() || stat(statePath_.c_str(), &current) != 0) {
        displayCached_ = false;
        displaySnapshot_.clear();
        return displaySnapshot_;
    }
    // Display snapshots change only on scenes and controls. Keep checking each
    // frame, but avoid opening, allocating and reading unchanged files.
    if (!displayCached_ || current.st_ino != displayStat_.st_ino
        || current.st_dev != displayStat_.st_dev || current.st_size != displayStat_.st_size
        || current.st_mtim.tv_sec != displayStat_.st_mtim.tv_sec
        || current.st_mtim.tv_nsec != displayStat_.st_mtim.tv_nsec) {
        displaySnapshot_ = read(statePath_, false);
        displayStat_ = current;
        displayCached_ = !displaySnapshot_.empty();
    }
    return displaySnapshot_;
}

std::string PairedTransport::readMusic() const {
    return read(musicPath_, true);
}

bool PairedTransport::publishRequest(const std::string& request) const {
    if (request.empty()) return false;
    return writeAtomic(requestPath_, request + "\n", false);
}

std::optional<std::string> PairedTransport::consumeRequest() const {
    std::ifstream input(requestPath_);
    std::string request;
    if (!(input >> request)) return std::nullopt;
    input.close();
    std::error_code error;
    std::filesystem::remove(requestPath_, error);
    if (error) return std::nullopt;
    return request;
}
