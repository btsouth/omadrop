#pragma once

#include <projectM-4/projectM.h>

#include <string>

bool loadPresetAtVisualTempo(projectm_handle projectm,
                             const std::string& filename,
                             bool smoothTransition);

// Capture projectM's completed output even if it leaves an internal read FBO bound.
void copyProjectmBackBuffer(unsigned int texture, int width, int height);
