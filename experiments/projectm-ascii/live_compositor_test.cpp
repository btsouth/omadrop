#include "live_compositor.h"

#include <GL/glew.h>
#include <SDL2/SDL.h>

#include <array>
#include <cassert>
#include <iostream>
#include <string>

namespace {
constexpr const char* vertex = R"GLSL(
#version 330 core
out vec2 uv;
void main() {
    vec2 p = vec2((gl_VertexID << 1) & 2, gl_VertexID & 2);
    uv = p;
    gl_Position = vec4(p * 2.0 - 1.0, 0.0, 1.0);
}
)GLSL";

constexpr const char* fragment = R"GLSL(
#version 330 core
in vec2 uv;
out vec4 color;
uniform sampler2D sourceFrame;
uniform float visibility;
void main() {
    color = vec4(texture(sourceFrame, uv).rgb * visibility, 1.0);
}
)GLSL";

GLuint solidTexture(const std::array<unsigned char, 4>& pixel) {
    GLuint texture = 0;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 1, 1, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, pixel.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    return texture;
}
}

int main() {
    assert(SDL_Init(SDL_INIT_VIDEO) == 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_Window* window = SDL_CreateWindow(
        "Omadrop compositor test", 0, 0, 64, 64,
        SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN);
    assert(window);
    SDL_GLContext context = SDL_GL_CreateContext(window);
    assert(context);
    glewExperimental = GL_TRUE;
    assert(glewInit() == GLEW_OK);

    LiveCompositor compositor;
    std::string error;
    assert(compositor.initialize(vertex, fragment, error));
    const GLuint red = solidTexture({255, 0, 0, 255});
    const GLuint black = solidTexture({0, 0, 0, 255});
    LiveCompositorFrame frame;
    frame.sourceTexture = red;
    frame.nextTexture = black;
    frame.coverTexture = black;
    frame.width = 64;
    frame.height = 64;
    frame.visibility = 1.0f;
    glBindTexture(GL_ARRAY_BUFFER, 0);
    assert(glGetError() == GL_INVALID_ENUM);
    glBindTexture(GL_ARRAY_BUFFER, 0);
    assert(compositor.render(frame, error));
    std::array<unsigned char, 4> pixel{};
    glReadPixels(32, 32, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel.data());
    assert(pixel[0] >= 250 && pixel[1] <= 5 && pixel[2] <= 5);

    frame.visibility = 0.0f;
    assert(compositor.render(frame, error));
    glReadPixels(32, 32, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel.data());
    assert(pixel[0] <= 5 && pixel[1] <= 5 && pixel[2] <= 5);

    frame.width = 0;
    assert(!compositor.render(frame, error));

    compositor.shutdown();
    glDeleteTextures(1, &red);
    glDeleteTextures(1, &black);
    SDL_GL_DeleteContext(context);
    SDL_DestroyWindow(window);
    SDL_Quit();
    std::cout << "live compositor passed\n";
    return 0;
}
