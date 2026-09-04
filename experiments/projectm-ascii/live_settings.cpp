#include "live_settings.h"

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
std::string commandOutput(const char* command) {
    FILE* pipe = popen(command, "r");
    if (!pipe) return {};
    std::array<char, 512> buffer{};
    std::string result;
    while (fgets(buffer.data(), static_cast<int>(buffer.size()), pipe)) {
        result += buffer.data();
    }
    pclose(pipe);
    while (!result.empty() && (result.back() == '\n' || result.back() == '\r')) {
        result.pop_back();
    }
    return result;
}

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
    if (const char* testPath = std::getenv("OMADROP_TEST_SINK_PATH")) {
        std::ifstream input(testPath);
        std::string sink;
        if (std::getline(input, sink)) return sink;
    }
    return commandOutput("pactl get-default-sink");
}
