#include "native_renderer.h"

#include <GL/glew.h>
#include <SDL2/SDL.h>

#include <array>
#include <filesystem>
#include <iostream>
#include <string>

namespace {

std::string glString(GLenum name) {
    const auto* value = glGetString(name);
    if (!value) return "unavailable";
    std::string result(reinterpret_cast<const char*>(value));
    for (char& character : result) {
        if (character == '\n' || character == '\r' || character == '=') {
            character = ' ';
        }
    }
    return result;
}

} // namespace

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cerr << "usage: gpu-probe SHADER_DIRECTORY\n";
        return 2;
    }
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::cerr << "gpu_probe_error=SDL init failed: " << SDL_GetError() << "\n";
        return 1;
    }
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_Window* window = SDL_CreateWindow(
        "Omadrop GPU probe", 0, 0, 96, 96,
        SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN);
    if (!window) {
        std::cerr << "gpu_probe_error=OpenGL 3.3 window failed: "
                  << SDL_GetError() << "\n";
        SDL_Quit();
        return 1;
    }
    SDL_GLContext context = SDL_GL_CreateContext(window);
    if (!context) {
        std::cerr << "gpu_probe_error=OpenGL 3.3 context failed: "
                  << SDL_GetError() << "\n";
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        std::cerr << "gpu_probe_error=GLEW initialization failed\n";
        SDL_GL_DeleteContext(context);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    // GLEW can leave GL_INVALID_ENUM behind on a core context.
    while (glGetError() != GL_NO_ERROR) {}

    GLint major = 0;
    GLint minor = 0;
    GLint maximumTextureSize = 0;
    glGetIntegerv(GL_MAJOR_VERSION, &major);
    glGetIntegerv(GL_MINOR_VERSION, &minor);
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maximumTextureSize);
    std::cout << "opengl_version=" << glString(GL_VERSION) << "\n"
              << "opengl_context=" << major << '.' << minor << " core\n"
              << "glsl_version=" << glString(GL_SHADING_LANGUAGE_VERSION) << "\n"
              << "gpu_vendor=" << glString(GL_VENDOR) << "\n"
              << "gpu_renderer=" << glString(GL_RENDERER) << "\n"
              << "max_texture_size=" << maximumTextureSize << "\n";

    NativeRenderer renderer;
    std::string error;
    if (!renderer.initialize(std::filesystem::path(argv[1]), error)) {
        std::cerr << "gpu_probe_error=native shader initialization failed: "
                  << error << "\n";
        renderer.shutdown();
        SDL_GL_DeleteContext(context);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    MusicFrame music;
    NativeSceneState scene;
    if (!renderer.render(music, scene, 96, 96,
                         std::array<float, 3>{0.46f, 0.72f, 1.0f},
                         0, 1.0f, 1.0f / 60.0f, error)) {
        std::cerr << "gpu_probe_error=native frame failed: " << error << "\n";
        renderer.shutdown();
        SDL_GL_DeleteContext(context);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    glFinish();
    const GLenum glError = glGetError();
    if (glError != GL_NO_ERROR) {
        std::cerr << "gpu_probe_error=OpenGL error " << glError << "\n";
        renderer.shutdown();
        SDL_GL_DeleteContext(context);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    std::cout << "native_renderer=ok\n";
    renderer.shutdown();
    SDL_GL_DeleteContext(context);
    SDL_DestroyWindow(window);
    SDL_Quit();
}
