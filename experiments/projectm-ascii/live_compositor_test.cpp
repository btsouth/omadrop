#include "live_compositor.h"
#include "live_projectm.h"
#include "live_compositor_shaders.h"

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

    // Exercise the real display path used by matched offline review. A neutral
    // half-gray patch must remain neutral and must not receive the old replay's
    // extra gamma lift (which made artistic comparisons misleading).
    assert(compositor.initialize(LiveCompositorShaders::vertexSource,
                                 LiveCompositorShaders::fragmentSource,error));
    const GLuint gray=solidTexture({128,128,128,255});
    frame.width=64; frame.visibility=1.0f;
    frame.sourceTexture=gray; frame.nextTexture=gray;
    frame.nativeRenderer=true; frame.fieldExposure=1.16f;
    assert(compositor.render(frame,error));
    glReadPixels(32,32,1,1,GL_RGBA,GL_UNSIGNED_BYTE,pixel.data());
    for(int c=0;c<3;++c) assert(pixel[c]>=103 && pixel[c]<=105);
    // Artwork remains opaque for the hold and fades continuously to the scene.
    frame.fieldExposure=1.65f;
    frame.coverTexture=red;
    frame.coverMix=1.0f;
    assert(compositor.render(frame,error));
    glReadPixels(32,32,1,1,GL_RGBA,GL_UNSIGNED_BYTE,pixel.data());
    assert(pixel[0]>=250 && pixel[1]<=5 && pixel[2]<=5);
    frame.coverMix=0.5f;
    assert(compositor.render(frame,error));
    glReadPixels(32,32,1,1,GL_RGBA,GL_UNSIGNED_BYTE,pixel.data());
    assert(pixel[0]>195 && pixel[0]<210 && pixel[1]>66 && pixel[1]<82);
    // Original presets retain exact source pixels at both transition endpoints.
    // Midpoint is a continuous dissolve without grading, zoom, or brightness lift.
    frame.nativeRenderer=false; frame.fieldExposure=1.0f; frame.coverMix=0.0f;
    frame.transitionMode=11; frame.sourceTexture=red; frame.nextTexture=gray;
    for (const float mix : {0.0f, 0.5f, 1.0f}) {
        frame.sceneMix=mix;
        assert(compositor.render(frame,error));
        glReadPixels(32,32,1,1,GL_RGBA,GL_UNSIGNED_BYTE,pixel.data());
        const int expectedR=static_cast<int>(255*(1-mix)+128*mix);
        const int expectedGB=static_cast<int>(128*mix);
        assert(std::abs(static_cast<int>(pixel[0])-expectedR)<=1);
        assert(std::abs(static_cast<int>(pixel[1])-expectedGB)<=1);
        assert(std::abs(static_cast<int>(pixel[2])-expectedGB)<=1);
    }
    // Regression: an engine can leave an internal read FBO bound. The capture
    // must copy the final back buffer, not that intermediate image.
    glBindFramebuffer(GL_FRAMEBUFFER,0);
    glClearColor(1,0,0,1); glClear(GL_COLOR_BUFFER_BIT);
    GLuint internalFbo=0; glGenFramebuffers(1,&internalFbo);
    glBindFramebuffer(GL_READ_FRAMEBUFFER,internalFbo);
    glFramebufferTexture2D(GL_READ_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,gray,0);
    glReadBuffer(GL_COLOR_ATTACHMENT0);
    copyProjectmBackBuffer(black,1,1);
    glBindTexture(GL_TEXTURE_2D,black);
    glGetTexImage(GL_TEXTURE_2D,0,GL_RGBA,GL_UNSIGNED_BYTE,pixel.data());
    assert(pixel[0]==255 && pixel[1]==0 && pixel[2]==0);
    glDeleteFramebuffers(1,&internalFbo);
    glDeleteTextures(1,&gray);

    compositor.shutdown();
    glDeleteTextures(1, &red);
    glDeleteTextures(1, &black);
    SDL_GL_DeleteContext(context);
    SDL_DestroyWindow(window);
    SDL_Quit();
    std::cout << "live compositor passed\n";
    return 0;
}
