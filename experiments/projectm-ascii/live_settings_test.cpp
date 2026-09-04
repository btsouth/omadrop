#include "live_settings.h"

#include <cassert>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <unistd.h>

int main() {
    const std::filesystem::path root = std::filesystem::temp_directory_path()
        / ("omadrop-live-settings-test-" + std::to_string(getpid()));
    std::filesystem::create_directories(root);
    setenv("XDG_CONFIG_HOME", root.c_str(), 1);
    unsetenv("OMADROP_SYNC_MS");

    const auto omadropDirectory = root / "omadrop";
    std::filesystem::create_directories(omadropDirectory);
    {
        std::ofstream legacy(omadropDirectory / "ascii-enabled");
        legacy << "0\n";
    }
    assert(!loadAsciiEnabled());
    assert(std::filesystem::exists(omadropDirectory / "preferences.conf"));
    saveAsciiEnabled(true);
    assert(loadAsciiEnabled());

    LivePreferences preferences;
    preferences.asciiEnabled = false;
    preferences.intensity = 1.35f;
    preferences.brightness = 0.72f;
    preferences.motion = 0.40f;
    preferences.reducedMotion = true;
    preferences.flashLimited = true;
    preferences.highContrast = true;
    preferences.colorVisionSafe = true;
    preferences.controlsReferenceSeen = true;
    preferences.displayMode = "single";
    preferences.directorProfile = DirectorProfile::Restrained;
    preferences.favoriteScenes = {"paper-horizon", "paper-horizon",
                                  "not valid"};
    preferences.hiddenScenes = {"centrifuge"};
    assert(saveLivePreferences(preferences));
    const LivePreferences restored = loadLivePreferences();
    assert(restored.version == livePreferencesVersion);
    assert(!restored.asciiEnabled);
    assert(restored.intensity > 1.34f && restored.intensity < 1.36f);
    assert(restored.brightness > 0.71f && restored.brightness < 0.73f);
    assert(restored.motion > 0.39f && restored.motion < 0.41f);
    assert(restored.reducedMotion);
    assert(restored.flashLimited);
    assert(restored.highContrast);
    assert(restored.colorVisionSafe);
    assert(restored.controlsReferenceSeen);
    assert(restored.displayMode == "single");
    assert(restored.directorProfile == DirectorProfile::Restrained);
    assert(restored.favoriteScenes.size() == 1);
    assert(restored.favoriteScenes.front() == "paper-horizon");
    assert(restored.hiddenScenes.size() == 1);
    assert(restored.hiddenScenes.front() == "centrifuge");

    {
        std::ofstream versionTwo(omadropDirectory / "preferences.conf");
        versionTwo << "version=2\n"
                   << "ascii=0\n"
                   << "reduced-motion=1\n";
    }
    LivePreferences migrated = loadLivePreferences();
    assert(migrated.version == 2);
    assert(!migrated.asciiEnabled && migrated.reducedMotion);
    assert(!migrated.flashLimited);
    assert(!migrated.colorVisionSafe);
    assert(!migrated.controlsReferenceSeen);
    assert(migrated.displayMode == "all");
    assert(saveLivePreferences(migrated));
    migrated = loadLivePreferences();
    assert(migrated.version == livePreferencesVersion);

    {
        std::ofstream malformed(omadropDirectory / "preferences.conf");
        malformed << "version=1\n"
                  << "ascii=maybe\n"
                  << "intensity=99\n"
                  << "brightness=-2\n"
                  << "motion=nan\n"
                  << "controls-reference-seen=maybe\n"
                  << "display=wall-of-monitors\n"
                  << "director=unknown\n"
                  << "favorite=../escape\n";
    }
    const LivePreferences repaired = loadLivePreferences();
    assert(repaired.asciiEnabled);
    assert(repaired.intensity == 1.50f);
    assert(repaired.brightness == 0.50f);
    assert(repaired.motion == 1.0f);
    assert(!repaired.flashLimited);
    assert(!repaired.colorVisionSafe);
    assert(!repaired.controlsReferenceSeen);
    assert(repaired.displayMode == "all");
    assert(repaired.directorProfile == DirectorProfile::Balanced);
    assert(repaired.favoriteScenes.empty());

    LivePreferences future = repaired;
    future.version = livePreferencesVersion + 1;
    assert(!saveLivePreferences(future));

    const auto sinkPath = root / "default-sink";
    {
        std::ofstream sinkOutput(sinkPath);
        sinkOutput << "alsa_output.usb_interface\n";
    }
    setenv("OMADROP_TEST_SINK_PATH", sinkPath.c_str(), 1);
    assert(defaultSinkName() == "alsa_output.usb_interface");
    unsetenv("OMADROP_TEST_SINK_PATH");

    assert(loadSyncDelay("alsa_output.default") == 35u);
    assert(loadSyncDelay("bluez_output.headphones") == 180u);
    saveSyncDelay(74u, "alsa/output with spaces");
    assert(loadSyncDelay("alsa/output with spaces") == 74u);
    setenv("OMADROP_SYNC_MS", "999", 1);
    assert(loadSyncDelay("alsa_output.default") == 500u);

    std::error_code error;
    std::filesystem::remove_all(root, error);
    std::cout << "live settings passed\n";
}
