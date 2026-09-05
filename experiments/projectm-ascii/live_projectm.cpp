#include <GL/glew.h>
#include "live_projectm.h"

#include "preset_adapters.h"

#include <fstream>
#include <cstdlib>
#include <iostream>
#include <regex>
#include <sstream>
#include <string>

namespace {
constexpr float visualTempo = 0.82f;
}

bool loadPresetAtVisualTempo(projectm_handle projectm,
                             const std::string& filename,
                             bool smoothTransition) {
    if (const char* originals = std::getenv("OMADROP_MILKDROP_ORIGINALS");
        originals && std::string(originals) == "1") {
        if (!std::ifstream(filename)) return false;
        bool failed = false;
        projectm_set_preset_switch_failed_event_callback(projectm,
            [](const char* path, const char* message, void* context) {
                *static_cast<bool*>(context) = true;
                std::cerr << "preset load failed: " << path << ": " << message << "\n";
            }, &failed);
        projectm_set_beat_sensitivity(projectm, 1.0f);
        projectm_load_preset_file(projectm, filename.c_str(), smoothTransition);
        projectm_set_preset_switch_failed_event_callback(projectm, nullptr, nullptr);
        return !failed;
    }
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

void copyProjectmBackBuffer(unsigned int texture, int width, int height) {
    glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
    glReadBuffer(GL_BACK);
    glBindTexture(GL_TEXTURE_2D, texture);
    glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, width, height);
}
