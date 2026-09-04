#include "live_projectm.h"

#include "preset_adapters.h"

#include <fstream>
#include <regex>
#include <sstream>
#include <string>

namespace {
constexpr float visualTempo = 0.82f;
}

bool loadPresetAtVisualTempo(projectm_handle projectm,
                             const std::string& filename,
                             bool smoothTransition) {
    std::ifstream input(filename, std::ios::binary);
    if (!input) return false;
    std::ostringstream contents;
    contents << input.rdbuf();
    std::string preset = contents.str();

    applyOmadropAdapter(filename, preset);
    const std::string scale = std::to_string(visualTempo);
    preset = std::regex_replace(
        preset, std::regex(R"(\btime\b)"), "(time*" + scale + ")");
    preset = std::regex_replace(
        preset, std::regex(R"(\bframe\b)"), "(frame*" + scale + ")");
    float sensitivity = 1.30f;
    if (filename.find("Halls Of Centrifuge") != std::string::npos) {
        sensitivity = 1.36f;
    } else if (filename.find("Songflower") != std::string::npos) {
        sensitivity = 1.34f;
    }
    projectm_set_beat_sensitivity(projectm, sensitivity);
    projectm_load_preset_data(projectm, preset.c_str(), smoothTransition);
    return true;
}
