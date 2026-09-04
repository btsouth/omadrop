#include "live_settings.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>

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
} // namespace

bool loadAsciiEnabled() {
    std::ifstream input(asciiSettingsPath());
    int enabled = 1;
    if (input >> enabled) return enabled != 0;
    return true;
}

void saveAsciiEnabled(bool enabled) {
    const auto path = asciiSettingsPath();
    if (path.empty()) return;
    std::error_code error;
    std::filesystem::create_directories(path.parent_path(), error);
    if (error) return;
    std::ofstream output(path, std::ios::trunc);
    if (output) output << (enabled ? 1 : 0) << "\n";
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
