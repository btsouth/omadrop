#include "live_assets.h"

#include <png.h>

#include <algorithm>
#include <array>
#include <fstream>
#include <string>
#include <vector>

bool loadPngTexture(const std::string& filename, GLuint texture, float& aspect) {
    png_image image{};
    image.version = PNG_IMAGE_VERSION;
    if (!png_image_begin_read_from_file(&image, filename.c_str())) return false;
    image.format = PNG_FORMAT_RGBA;
    std::vector<unsigned char> pixels(PNG_IMAGE_SIZE(image));
    if (!png_image_finish_read(&image, nullptr, pixels.data(), 0, nullptr)) {
        png_image_free(&image);
        return false;
    }
    aspect = static_cast<float>(image.width) / static_cast<float>(image.height);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, static_cast<GLsizei>(image.width),
                 static_cast<GLsizei>(image.height), 0, GL_RGBA,
                 GL_UNSIGNED_BYTE, pixels.data());
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    png_image_free(&image);
    return true;
}

std::array<float, 3> loadPaletteColor(const std::string& filename) {
    std::ifstream input(filename + ".pal");
    std::string hex;
    std::array<float, 3> best{0.72f, 0.82f, 1.0f};
    float bestScore = -1.0f;
    while (input >> hex) {
        if (hex.size() != 7 || hex[0] != '#') continue;
        try {
            const int value = std::stoi(hex.substr(1), nullptr, 16);
            std::array<float, 3> color{
                ((value >> 16) & 255) / 255.0f,
                ((value >> 8) & 255) / 255.0f,
                (value & 255) / 255.0f};
            const auto [minimum, maximum] = std::minmax_element(
                color.begin(), color.end());
            const float light = (color[0] + color[1] + color[2]) / 3.0f;
            const float score = (*maximum - *minimum) * 1.7f + light * 0.35f;
            if (score > bestScore && light > 0.12f) {
                best = color;
                bestScore = score;
            }
        } catch (...) {}
    }
    return best;
}
