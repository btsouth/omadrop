#pragma once

#include <projectM-4/projectM.h>

#include <string>

bool loadPresetAtVisualTempo(projectm_handle projectm,
                             const std::string& filename,
                             bool smoothTransition);
