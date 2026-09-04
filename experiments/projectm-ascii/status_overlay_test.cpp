#include "status_overlay.h"

#include <SDL2/SDL.h>

#include <algorithm>
#include <array>
#include <cassert>
#include <fstream>
#include <iostream>

int main(int argc, char** argv) {
    const StatusBitmap empty = rasterizeStatusLabel("");
    assert(empty.rgba.empty());

    const StatusBitmap label = rasterizeStatusLabel("Auto: Paper Horizon");
    assert(label.width > 300);
    assert(label.height == 39);
    assert(label.rgba.size()
           == static_cast<std::size_t>(label.width * label.height * 4));
    int opaquePixels = 0;
    int textPixels = 0;
    for (std::size_t index = 0; index < label.rgba.size(); index += 4) {
        opaquePixels += label.rgba[index + 3] > 0;
        textPixels += label.rgba[index] > 200 && label.rgba[index + 3] == 255;
    }
    assert(opaquePixels > label.width * label.height * 0.80);
    assert(textPixels > 300);

    const StatusBitmap capped = rasterizeStatusLabel(
        "THIS STATUS MESSAGE IS DELIBERATELY LONGER THAN THE DISPLAY LIMIT");
    assert(capped.width <= 885);

    const StatusBitmap calibration = rasterizeStatusLabel(
        "SYNC 35 MS  [ EARLIER  ] LATER  ESC DONE");
    assert(calibration.width > 600);
    assert(calibration.width < 885);

    assert(SDL_Init(SDL_INIT_VIDEO) == 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_Window* window = SDL_CreateWindow(
        "Omadrop status overlay test", 0, 0, 640, 360,
        SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN);
    assert(window);
    SDL_GLContext context = SDL_GL_CreateContext(window);
    assert(context);
    glewExperimental = GL_TRUE;
    assert(glewInit() == GLEW_OK);
    StatusOverlay overlay;
    std::string error;
    assert(overlay.initialize(error));
    overlay.show("AUTO: PAPER HORIZON", 1000);
    glViewport(0, 0, 640, 360);
    glClearColor(0.5f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    assert(overlay.render(640, 360, 1000, error));
    std::array<unsigned char, 4> pixel{};
    glReadPixels(320, 27, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel.data());
    assert(pixel[0] < 100);
    glClearColor(0.0f, 0.5f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    assert(overlay.render(640, 360, 3000, error));
    glReadPixels(320, 27, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel.data());
    assert(pixel[1] >= 126);
    overlay.shutdown();
    SDL_GL_DeleteContext(context);
    SDL_DestroyWindow(window);
    SDL_Quit();

    if (argc == 2) {
        std::ofstream output(argv[1], std::ios::binary);
        assert(output);
        output << "P6\n" << label.width << ' ' << label.height << "\n255\n";
        for (std::size_t index = 0; index < label.rgba.size(); index += 4) {
            const float alpha = label.rgba[index + 3] / 255.0f;
            const std::array<unsigned char, 3> background{24, 13, 39};
            for (int channel = 0; channel < 3; ++channel) {
                const auto value = static_cast<unsigned char>(
                    background[channel] * (1.0f - alpha)
                    + label.rgba[index + channel] * alpha + 0.5f);
                output.write(reinterpret_cast<const char*>(&value), 1);
            }
        }
        assert(output);
    }
    std::cout << "status overlay passed\n";
}
