#pragma once

#include <GL/glew.h>

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

struct StatusBitmap {
    int width = 0;
    int height = 0;
    std::vector<unsigned char> rgba;
};

StatusBitmap rasterizeStatusLabel(std::string_view text);

class StatusOverlay {
public:
    StatusOverlay() = default;
    ~StatusOverlay();
    StatusOverlay(const StatusOverlay&) = delete;
    StatusOverlay& operator=(const StatusOverlay&) = delete;

    bool initialize(std::string& error);
    void show(std::string_view text, std::uint64_t nowMilliseconds);
    bool render(int outputWidth, int outputHeight,
                std::uint64_t nowMilliseconds, std::string& error);
    void shutdown();

private:
    GLuint program_ = 0;
    GLuint vertexShader_ = 0;
    GLuint fragmentShader_ = 0;
    GLuint texture_ = 0;
    GLuint vao_ = 0;
    int width_ = 0;
    int height_ = 0;
    std::uint64_t shownAt_ = 0;
    bool active_ = false;
};
