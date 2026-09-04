#include "live_settings.h"

#include <cassert>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>
#include <unistd.h>

int main() {
    const std::filesystem::path root = std::filesystem::temp_directory_path()
        / ("omadrop-live-settings-test-" + std::to_string(getpid()));
    std::filesystem::create_directories(root);
    setenv("XDG_CONFIG_HOME", root.c_str(), 1);
    unsetenv("OMADROP_SYNC_MS");

    assert(loadAsciiEnabled());
    saveAsciiEnabled(false);
    assert(!loadAsciiEnabled());
    saveAsciiEnabled(true);
    assert(loadAsciiEnabled());

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
