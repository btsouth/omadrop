#pragma once

#include <GL/glew.h>

#include <array>
#include <string>

bool loadPngTexture(const std::string& filename, GLuint texture, float& aspect);
std::array<float, 3> loadPaletteColor(const std::string& filename);
