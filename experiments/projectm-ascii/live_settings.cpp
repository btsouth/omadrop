#include "live_settings.h"
#include "child_process.h"
#include <fcntl.h>

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <set>
#include <sstream>
#include <string>
#include <system_error>
#include <unistd.h>

namespace {
// Cached, nonblocking output discovery. The render thread never waits for
// pactl or for the audio server to return after an interruption.
class SinkQuery {
public:
    ~SinkQuery() { stop(); }
    std::string poll() {
        const auto now = std::chrono::steady_clock::now();
        if (child_ > 0) {
            char buffer[512];
            ssize_t count;
            while ((count = read(fd_, buffer, sizeof(buffer))) > 0) {
                output_.append(buffer, static_cast<std::size_t>(count));
                if (output_.size() > 4096) { stop(); return cached_; }
            }
            int status = 0;
            const pid_t result = waitpid(child_, &status, WNOHANG);
            if (result == child_) {
                if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
                    // Drain output produced between the first read and exit.
                    while ((count = read(fd_, buffer, sizeof(buffer))) > 0) {
                        output_.append(buffer, static_cast<std::size_t>(count));
                        if (output_.size() > 4096) break;
                    }
                    while (!output_.empty() && (output_.back() == '\n'
                           || output_.back() == '\r')) output_.pop_back();
                    if (!output_.empty() && output_.size() <= 4096)
                        cached_ = output_;
                }
                close(fd_); fd_ = -1; child_ = -1;
            } else if (now - started_ >= std::chrono::milliseconds(500)) {
                stop();
            }
        }
        if (child_ < 0 && now >= next_) {
            next_ = now + std::chrono::seconds(2);
            int fds[2];
            if (pipe2(fds, O_CLOEXEC | O_NONBLOCK) != 0) return cached_;
            const char* command = std::getenv("OMADROP_PACTL_COMMAND");
            if (!command || !*command) command = "pactl";
            child_ = fork();
            if (child_ == 0) {
                setpgid(0, 0);
                dup2(fds[1], STDOUT_FILENO);
                const int nullFd = open("/dev/null", O_WRONLY);
                if (nullFd >= 0) dup2(nullFd, STDERR_FILENO);
                close(fds[0]); close(fds[1]);
                execlp(command, command, "get-default-sink", static_cast<char*>(nullptr));
                _exit(127);
            }
            close(fds[1]);
            if (child_ < 0) { close(fds[0]); return cached_; }
            setpgid(child_, child_);
            fd_ = fds[0]; output_.clear(); started_ = now;
        }
        return cached_;
    }
private:
    void stop() {
        stopChildProcess(child_);
        if (fd_ >= 0) close(fd_);
        fd_ = -1; child_ = -1; output_.clear();
    }
    pid_t child_ = -1;
    int fd_ = -1;
    std::string cached_, output_;
    std::chrono::steady_clock::time_point started_, next_;
};

std::filesystem::path configDirectory() {
    if (const char* configHome = std::getenv("XDG_CONFIG_HOME")) {
        return std::filesystem::path(configHome) / "omadrop";
    }
    if (const char* userHome = std::getenv("HOME")) {
        return std::filesystem::path(userHome) / ".config" / "omadrop";
    }
    return {};
}

std::filesystem::path syncSettingsPath(const std::string& sink) {
    std::string filename;
    for (const unsigned char character : sink) {
        filename += std::isalnum(character) || character == '-'
            || character == '_' || character == '.'
            ? static_cast<char>(character) : '_';
    }
    if (filename.empty()) filename = "default";
    if (filename.size() > 180) filename.resize(180);
    const auto directory = configDirectory();
    return directory.empty()
        ? directory : directory / "sync-by-sink" / (filename + ".ms");
}

std::filesystem::path legacySyncSettingsPath() {
    const auto directory = configDirectory();
    return directory.empty() ? directory : directory / "sync-ms";
}

std::filesystem::path asciiSettingsPath() {
    const auto directory = configDirectory();
    return directory.empty() ? directory : directory / "ascii-enabled";
}

std::filesystem::path preferencesPath() {
    const auto directory = configDirectory();
    return directory.empty() ? directory : directory / "preferences.conf";
}

std::string trim(std::string value) {
    const auto first = value.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return {};
    const auto last = value.find_last_not_of(" \t\r\n");
    return value.substr(first, last - first + 1);
}

bool parseBool(const std::string& value, bool& result) {
    if (value == "1" || value == "true") {
        result = true;
        return true;
    }
    if (value == "0" || value == "false") {
        result = false;
        return true;
    }
    return false;
}

bool parseFloat(const std::string& value, float& result) {
    char* end = nullptr;
    const float parsed = std::strtof(value.c_str(), &end);
    if (end == value.c_str() || *end != '\0' || !std::isfinite(parsed)) {
        return false;
    }
    result = parsed;
    return true;
}

bool validSceneSlug(const std::string& value) {
    if (value.empty() || value.size() > 64) return false;
    return std::all_of(value.begin(), value.end(), [](unsigned char character) {
        return std::islower(character) || std::isdigit(character)
            || character == '-';
    });
}

DirectorProfile parseDirectorProfile(const std::string& value) {
    if (value == "kinetic") return DirectorProfile::Kinetic;
    if (value == "restrained") return DirectorProfile::Restrained;
    if (value == "high-contrast") return DirectorProfile::HighContrast;
    return DirectorProfile::Balanced;
}

void appendUniqueScene(std::vector<std::string>& scenes,
                       const std::string& value) {
    if (!validSceneSlug(value) || scenes.size() >= 128) return;
    if (std::find(scenes.begin(), scenes.end(), value) == scenes.end()) {
        scenes.push_back(value);
    }
}
} // namespace

const char* directorProfileName(DirectorProfile profile) {
    switch (profile) {
        case DirectorProfile::Balanced: return "balanced";
        case DirectorProfile::Kinetic: return "kinetic";
        case DirectorProfile::Restrained: return "restrained";
        case DirectorProfile::HighContrast: return "high-contrast";
    }
    return "balanced";
}

LivePreferences loadLivePreferences() {
    LivePreferences preferences;
    const auto path = preferencesPath();
    std::ifstream input(path);
    if (!input) {
        std::ifstream legacy(asciiSettingsPath());
        int enabled = 1;
        if (legacy >> enabled) {
            preferences.asciiEnabled = enabled != 0;
            saveLivePreferences(preferences);
        }
        return preferences;
    }

    std::string line;
    while (std::getline(input, line)) {
        const auto separator = line.find('=');
        if (separator == std::string::npos) continue;
        const std::string key = trim(line.substr(0, separator));
        const std::string value = trim(line.substr(separator + 1));
        if (key == "version") {
            char* end = nullptr;
            const unsigned long parsed = std::strtoul(value.c_str(), &end, 10);
            if (end != value.c_str() && *end == '\0') {
                preferences.version = static_cast<unsigned int>(std::min(
                    parsed, static_cast<unsigned long>(UINT32_MAX)));
            }
        } else if (key == "ascii") {
            parseBool(value, preferences.asciiEnabled);
        } else if (key == "intensity") {
            parseFloat(value, preferences.intensity);
        } else if (key == "brightness") {
            parseFloat(value, preferences.brightness);
        } else if (key == "motion") {
            parseFloat(value, preferences.motion);
        } else if (key == "reduced-motion") {
            parseBool(value, preferences.reducedMotion);
        } else if (key == "flash-limited") {
            parseBool(value, preferences.flashLimited);
        } else if (key == "high-contrast") {
            parseBool(value, preferences.highContrast);
        } else if (key == "color-vision-safe") {
            parseBool(value, preferences.colorVisionSafe);
        } else if (key == "controls-reference-seen") {
            parseBool(value, preferences.controlsReferenceSeen);
        } else if (key == "display") {
            if (value == "all" || value == "single") {
                preferences.displayMode = value;
            }
        } else if (key == "director") {
            preferences.directorProfile = parseDirectorProfile(value);
        } else if (key == "favorite") {
            appendUniqueScene(preferences.favoriteScenes, value);
        } else if (key == "hidden") {
            appendUniqueScene(preferences.hiddenScenes, value);
        }
    }
    preferences.intensity = std::clamp(preferences.intensity, 0.50f, 1.50f);
    preferences.brightness = std::clamp(preferences.brightness, 0.50f, 1.25f);
    preferences.motion = std::clamp(preferences.motion, 0.0f, 1.0f);
    return preferences;
}

bool saveLivePreferences(const LivePreferences& supplied) {
    if (supplied.version > livePreferencesVersion) return false;
    const auto path = preferencesPath();
    if (path.empty()) return false;
    std::error_code error;
    std::filesystem::create_directories(path.parent_path(), error);
    if (error) return false;

    LivePreferences preferences = supplied;
    preferences.version = livePreferencesVersion;
    preferences.intensity = std::clamp(preferences.intensity, 0.50f, 1.50f);
    preferences.brightness = std::clamp(preferences.brightness, 0.50f, 1.25f);
    preferences.motion = std::clamp(preferences.motion, 0.0f, 1.0f);
    const auto temporary = path.string() + ".tmp-" + std::to_string(getpid());
    std::ofstream output(temporary, std::ios::trunc);
    if (!output) return false;
    output << "version=" << preferences.version << '\n'
           << "ascii=" << (preferences.asciiEnabled ? 1 : 0) << '\n'
           << "intensity=" << preferences.intensity << '\n'
           << "brightness=" << preferences.brightness << '\n'
           << "motion=" << preferences.motion << '\n'
           << "reduced-motion=" << (preferences.reducedMotion ? 1 : 0) << '\n'
           << "flash-limited=" << (preferences.flashLimited ? 1 : 0) << '\n'
           << "high-contrast=" << (preferences.highContrast ? 1 : 0) << '\n'
           << "color-vision-safe="
           << (preferences.colorVisionSafe ? 1 : 0) << '\n'
           << "controls-reference-seen="
           << (preferences.controlsReferenceSeen ? 1 : 0) << '\n'
           << "display="
           << (preferences.displayMode == "single" ? "single" : "all") << '\n'
           << "director=" << directorProfileName(preferences.directorProfile)
           << '\n';
    std::set<std::string> writtenFavorites;
    for (const std::string& scene : preferences.favoriteScenes) {
        if (validSceneSlug(scene) && writtenFavorites.insert(scene).second) {
            output << "favorite=" << scene << '\n';
        }
    }
    std::set<std::string> writtenHidden;
    for (const std::string& scene : preferences.hiddenScenes) {
        if (validSceneSlug(scene) && writtenHidden.insert(scene).second) {
            output << "hidden=" << scene << '\n';
        }
    }
    output.flush();
    if (!output) {
        output.close();
        std::filesystem::remove(temporary, error);
        return false;
    }
    output.close();
    std::filesystem::rename(temporary, path, error);
    if (error) {
        std::filesystem::remove(temporary, error);
        return false;
    }
    return true;
}

bool loadAsciiEnabled() {
    return loadLivePreferences().asciiEnabled;
}

void saveAsciiEnabled(bool enabled) {
    LivePreferences preferences = loadLivePreferences();
    preferences.asciiEnabled = enabled;
    saveLivePreferences(preferences);
}

void saveSyncDelay(unsigned int milliseconds, const std::string& sink) {
    const auto path = syncSettingsPath(sink);
    if (path.empty()) return;
    std::error_code error;
    std::filesystem::create_directories(path.parent_path(), error);
    if (error) return;
    std::ofstream output(path);
    if (output) output << milliseconds << "\n";
}

unsigned int loadSyncDelay(const std::string& sink) {
    if (const char* configuredDelay = std::getenv("OMADROP_SYNC_MS")) {
        return static_cast<unsigned int>(
            std::clamp(std::atoi(configuredDelay), 0, 500));
    }
    unsigned int milliseconds = sink.rfind("bluez_", 0) == 0 ? 180u : 35u;
    const auto sinkSettings = syncSettingsPath(sink);
    std::ifstream savedDelay(sinkSettings);
    bool migrateLegacyDelay = false;
    if (!savedDelay && !std::filesystem::exists(sinkSettings.parent_path())) {
        savedDelay.clear();
        savedDelay.open(legacySyncSettingsPath());
        migrateLegacyDelay = static_cast<bool>(savedDelay);
    }
    int savedMilliseconds = 0;
    if (savedDelay >> savedMilliseconds) {
        milliseconds = static_cast<unsigned int>(
            std::clamp(savedMilliseconds, 0, 500));
        if (migrateLegacyDelay) saveSyncDelay(milliseconds, sink);
    }
    return milliseconds;
}

std::string defaultSinkName() {
    if (const char* configuredSink = std::getenv("OMADROP_AUDIO_SINK")) {
        if (*configuredSink) return configuredSink;
    }
    if (const char* testPath = std::getenv("OMADROP_TEST_SINK_PATH")) {
        std::ifstream input(testPath);
        std::string sink;
        if (std::getline(input, sink)) return sink;
    }
    static SinkQuery query;
    return query.poll();
}
