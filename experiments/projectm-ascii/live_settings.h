#pragma once

#include "native_scene_registry.h"

#include <string>
#include <vector>

using DirectorProfile = NativeDirectorProfile;

inline constexpr unsigned int livePreferencesVersion = 3;

struct LivePreferences {
    unsigned int version = livePreferencesVersion;
    bool asciiEnabled = true;
    float intensity = 1.0f;
    float brightness = 1.0f;
    float motion = 1.0f;
    bool reducedMotion = false;
    bool flashLimited = false;
    bool highContrast = false;
    bool colorVisionSafe = false;
    DirectorProfile directorProfile = DirectorProfile::Balanced;
    std::vector<std::string> favoriteScenes;
    std::vector<std::string> hiddenScenes;
};

const char* directorProfileName(DirectorProfile profile);
LivePreferences loadLivePreferences();
bool saveLivePreferences(const LivePreferences& preferences);

bool loadAsciiEnabled();
void saveAsciiEnabled(bool enabled);
unsigned int loadSyncDelay(const std::string& sink);
void saveSyncDelay(unsigned int milliseconds, const std::string& sink);
std::string defaultSinkName();
