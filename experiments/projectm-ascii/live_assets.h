#pragma once

#include <GL/glew.h>

#include <array>
#include <string>
#include <vector>

bool loadPngPixels(const std::string& filename, std::vector<unsigned char>& pixels,
                   int& width, int& height);
bool loadPngTexture(const std::string& filename, GLuint texture, float& aspect);
std::array<float, 3> loadPaletteColor(const std::string& filename);
