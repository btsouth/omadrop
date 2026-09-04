#include "gpu_pass_timer.h"

#include <SDL2/SDL.h>

#include <cassert>
#include <cmath>
#include <iostream>

int main() {
    assert(SDL_Init(SDL_INIT_VIDEO) == 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_Window* window = SDL_CreateWindow(
        "Omadrop GPU timer test", 0, 0, 64, 64,
        SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN);
    assert(window);
    SDL_GLContext context = SDL_GL_CreateContext(window);
    assert(context);
    glewExperimental = GL_TRUE;
    assert(glewInit() == GLEW_OK);

    GpuPassTimer timer;
    assert(timer.initialize());
    timer.begin();
    glClearColor(0.1f, 0.2f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    timer.end();
    glFinish();
    timer.collect();
    assert(timer.sampleSerial() == 1);
    assert(timer.latestMilliseconds().has_value());
    assert(std::isfinite(*timer.latestMilliseconds()));
    assert(*timer.latestMilliseconds() >= 0.0);
    timer.shutdown();

    SDL_GL_DeleteContext(context);
    SDL_DestroyWindow(window);
    SDL_Quit();
    std::cout << "GPU pass timer passed\n";
}
