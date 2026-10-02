#pragma once

#include "native_scene_registry.h"

#include <string>
#include <vector>

using DirectorProfile = NativeDirectorProfile;

inline constexpr unsigned int livePreferencesVersion = 4;

struct LivePreferences {
    unsigned int version = livePreferencesVersion;
    bool asciiEnabled = true;
    bool captionsEnabled = true;
    float intensity = 1.0f;
    float brightness = 1.0f;
    float motion = 1.0f;
    bool reducedMotion = false;
    bool flashLimited = false;
    bool highContrast = false;
    bool colorVisionSafe = false;
    bool controlsReferenceSeen = false;
    std::string displayMode = "all";
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
