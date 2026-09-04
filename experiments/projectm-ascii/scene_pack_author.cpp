#include "native_renderer.h"

#include <GL/glew.h>
#include <SDL2/SDL.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>

namespace {
constexpr float frameSeconds = 1.0f / 60.0f;

const char* previewVertex = R"GLSL(
#version 330 core
out vec2 uv;
void main() {
    vec2 p = vec2((gl_VertexID << 1) & 2, gl_VertexID & 2);
    uv = p;
    gl_Position = vec4(p * 2.0 - 1.0, 0.0, 1.0);
}
)GLSL";

const char* previewFragment = R"GLSL(
#version 330 core
in vec2 uv;
out vec4 color;
uniform sampler2D sceneFrame;
uniform vec2 resolution;
uniform vec4 signals;

float lightness(vec3 c) { return dot(c, vec3(0.299, 0.587, 0.114)); }

vec3 asciiSample(vec2 p) {
    vec2 panePixels = vec2(resolution.x * 0.5, resolution.y);
    vec2 pixel = p * panePixels;
    vec2 cell = floor(pixel / vec2(10.0, 20.0));
    vec2 local = mod(pixel, vec2(10.0, 20.0));
    vec2 sourceUv = (cell * vec2(10.0, 20.0) + vec2(5.0, 10.0)) / panePixels;
    vec3 sampleColor = texture(sceneFrame, sourceUv).rgb;
    float level = clamp(sqrt(max(0.0, lightness(sampleColor))) * 1.18, 0.0, 1.0);
    int dotX = local.x < 5.0 ? 0 : 1;
    int dotY = int(clamp(floor(local.y / 5.0), 0.0, 3.0));
    int dotIndex = dotY * 2 + dotX;
    float thresholds[8] = float[8](0.12, 0.58, 0.34, 0.82,
                                   0.70, 0.22, 0.94, 0.46);
    vec2 center = vec2(dotX == 0 ? 2.5 : 7.5, 2.5 + float(dotY) * 5.0);
    float dot = 1.0 - smoothstep(1.1, 2.1, length(local - center));
    return level >= thresholds[dotIndex] ? sampleColor * (0.86 + 0.30 * level) * dot
                                         : vec3(0.0);
}

void main() {
    bool asciiPane = uv.x >= 0.5;
    vec2 paneUv = vec2(asciiPane ? (uv.x - 0.5) * 2.0 : uv.x * 2.0, uv.y);
    vec3 result = asciiPane ? asciiSample(paneUv)
                            : texture(sceneFrame, paneUv).rgb;
    float divider = 1.0 - smoothstep(0.0, 1.5 / resolution.x,
                                     abs(uv.x - 0.5));
    result = mix(result, vec3(0.30), divider * 0.7);

    if (uv.y < 0.035) {
        float lane = floor(uv.x * 4.0);
        float localX = fract(uv.x * 4.0);
        float value = lane < 0.5 ? signals.x
                    : lane < 1.5 ? signals.y
                    : lane < 2.5 ? signals.z : signals.w;
        vec3 meter = lane < 0.5 ? vec3(0.28, 0.68, 1.0)
                   : lane < 1.5 ? vec3(1.0, 0.38, 0.64)
                   : lane < 2.5 ? vec3(0.92, 0.84, 0.30)
                                : vec3(0.46, 0.92, 0.66);
        result = localX < clamp(value, 0.0, 1.0) ? meter : vec3(0.025);
    }
    color = vec4(result, 1.0);
}
)GLSL";

GLuint compile(GLenum type, const char* source, std::string& error) {
    const GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);
    GLint okay = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &okay);
    if (okay) return shader;
    GLint length = 0;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
    std::string log(std::max(1, length), '\0');
    glGetShaderInfoLog(shader, length, nullptr, log.data());
    error = log;
    glDeleteShader(shader);
    return 0;
}

GLuint previewProgram(std::string& error) {
    const GLuint vertex = compile(GL_VERTEX_SHADER, previewVertex, error);
    if (!vertex) return 0;
    const GLuint fragment = compile(GL_FRAGMENT_SHADER, previewFragment, error);
    if (!fragment) {
        glDeleteShader(vertex);
        return 0;
    }
    const GLuint program = glCreateProgram();
    glAttachShader(program, vertex);
    glAttachShader(program, fragment);
    glLinkProgram(program);
    glDeleteShader(vertex);
    glDeleteShader(fragment);
    GLint okay = GL_FALSE;
    glGetProgramiv(program, GL_LINK_STATUS, &okay);
    if (okay) return program;
    error = "preview compositor link failed";
    glDeleteProgram(program);
    return 0;
}

float envelope(float phase, float width, float decay) {
    return phase < width ? std::exp(-phase * decay / width) : 0.0f;
}

MusicFrame musicAt(float seconds) {
    const float loop = std::fmod(std::max(0.0f, seconds), 16.0f);
    const float beat = std::fmod(loop * 2.0f, 1.0f);
    const float halfBeat = std::fmod(loop * 4.0f, 1.0f);
    const int sectionIndex = std::min(3, static_cast<int>(loop / 4.0f));
    const std::array<float, 4> energyBySection{0.20f, 0.48f, 0.82f, 0.34f};
    const std::array<float, 4> densityBySection{0.16f, 0.48f, 0.84f, 0.28f};
    const float energy = energyBySection[sectionIndex];
    const float density = densityBySection[sectionIndex];

    MusicFrame music;
    music.audioTimeSeconds = loop;
    music.bpm = 120.0f;
    music.beatPhase = beat;
    music.barPhase = std::fmod(loop / 2.0f, 1.0f);
    music.phrasePhase = loop / 16.0f;
    music.clockConfidence = 0.95f;
    music.kick = envelope(beat, 0.22f, 4.8f)
               * (sectionIndex == 0 ? 0.34f : 0.92f);
    const float snarePhase = std::fmod(beat + 0.5f, 1.0f);
    music.snare = envelope(snarePhase, 0.18f, 5.5f)
                * (sectionIndex == 0 ? 0.20f : 0.78f);
    music.hat = envelope(halfBeat, 0.14f, 6.0f)
              * (sectionIndex == 2 ? 0.82f : sectionIndex == 1 ? 0.52f : 0.20f);
    music.beatPulse = envelope(beat, 0.28f, 4.0f);
    music.onsetPulse = std::max({music.kick, music.snare, music.hat});
    music.downbeat = music.barPhase < 0.12f
        ? envelope(music.barPhase, 0.12f, 4.0f) : 0.0f;
    music.section = std::fmod(loop, 4.0f) < 0.20f
        ? envelope(std::fmod(loop, 4.0f), 0.20f, 3.0f) : 0.0f;
    music.energyFast = energy + 0.08f * music.kick;
    music.energySlow = energy;
    music.energySlope = sectionIndex == 1 ? 0.08f
                      : sectionIndex == 3 ? -0.10f : 0.0f;
    music.percussive = 0.12f + density * 0.58f;
    music.harmonic = 0.42f + (sectionIndex == 2 ? 0.26f : 0.12f);
    music.spectralCentroid = 0.32f + density * 0.48f;
    music.stereoWidth = 0.44f + 0.28f * std::sin(loop * 0.31f);
    music.rhythmicDensity = density;
    music.syncopation = sectionIndex == 2 ? 0.62f : 0.24f;
    music.tonalMotion = 0.20f + 0.24f * std::sin(loop * 0.43f);
    music.harmonicChange = music.section;
    for (std::size_t index = 0; index < music.bandLevel.size(); ++index) {
        music.bandLevel[index] = energy * (0.78f + 0.07f * index);
    }
    for (std::size_t index = 0; index < music.spectrumLevel.size(); ++index) {
        music.spectrumLevel[index] = energy * (0.72f + 0.18f
            * std::sin(loop * 0.35f + static_cast<float>(index) * 0.63f));
    }
    for (std::size_t index = 0; index < music.chroma.size(); ++index) {
        const int root = sectionIndex == 0 ? 0 : sectionIndex == 1 ? 5
                       : sectionIndex == 2 ? 7 : 3;
        const int distance = std::min((static_cast<int>(index) - root + 12) % 12,
                                      (root - static_cast<int>(index) + 12) % 12);
        music.chroma[index] = std::exp(-0.72f * static_cast<float>(distance));
    }
    return music;
}

NativeSceneState sceneAt(float seconds, const MusicFrame& music) {
    NativeSceneState scene;
    scene.currentScene = NativeSceneKind::DepthTunnel;
    scene.incomingScene = NativeSceneKind::DepthTunnel;
    scene.sceneBeats = seconds * 2.0f;
    scene.development = std::clamp(seconds / 6.0f, 0.0f, 1.0f);
    scene.drive = std::clamp((music.energySlow - 0.18f) / 0.60f, 0.0f, 1.0f);
    scene.peak = std::clamp((music.energyFast - 0.62f) / 0.26f, 0.0f, 1.0f);
    scene.release = std::clamp(-music.energySlope * 5.0f, 0.0f, 1.0f);
    return scene;
}

std::unique_ptr<NativeRenderer> loadRenderer(
        const std::filesystem::path& vertex,
        const std::filesystem::path& fragment, std::string& error) {
    auto candidate = std::make_unique<NativeRenderer>();
    if (!candidate->initializeCustomShader(vertex, fragment, error)) return {};
    return candidate;
}
} // namespace

int main(int argc, char** argv) {
    if (argc == 2 && std::string(argv[1]) == "--help") {
        std::cout << "usage: scene-pack-author VERTEX_SHADER EXPANDED_SHADER SCENE_NAME\n";
        return 0;
    }
    if (argc != 4) {
        std::cerr << "usage: scene-pack-author VERTEX_SHADER EXPANDED_SHADER SCENE_NAME\n";
        return 2;
    }
    if (SDL_Init(SDL_INIT_VIDEO) != 0) return 1;
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_Window* window = SDL_CreateWindow("Omadrop Pack Author", SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED, 1280, 720,
        SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    if (!window) return 1;
    SDL_GLContext context = SDL_GL_CreateContext(window);
    if (!context) return 1;
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) return 1;
    SDL_GL_SetSwapInterval(1);

    std::string error;
    GLuint program = previewProgram(error);
    if (!program) {
        std::cerr << error << '\n';
        return 1;
    }
    GLuint vao = 0;
    glGenVertexArrays(1, &vao);
    auto renderer = loadRenderer(argv[1], argv[2], error);
    if (!renderer) {
        std::cerr << "scene author: " << error << '\n';
        return 1;
    }
    std::filesystem::file_time_type shaderTime
        = std::filesystem::last_write_time(argv[2]);

    bool running = true;
    bool paused = false;
    float playhead = 0.0f;
    auto previous = std::chrono::steady_clock::now();
    auto lastTitle = previous - std::chrono::seconds(1);
    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) running = false;
            if (event.type != SDL_KEYDOWN || event.key.repeat) continue;
            if (event.key.keysym.sym == SDLK_ESCAPE) running = false;
            if (event.key.keysym.sym == SDLK_SPACE) paused = !paused;
            if (event.key.keysym.sym == SDLK_LEFT) {
                playhead = std::fmod(playhead + 15.0f, 16.0f);
                renderer->reset();
            }
            if (event.key.keysym.sym == SDLK_RIGHT) {
                playhead = std::fmod(playhead + 1.0f, 16.0f);
                renderer->reset();
            }
        }

        std::error_code timeError;
        const auto currentShaderTime = std::filesystem::last_write_time(
            argv[2], timeError);
        if (!timeError && currentShaderTime != shaderTime) {
            std::string reloadError;
            auto candidate = loadRenderer(argv[1], argv[2], reloadError);
            if (candidate) {
                renderer = std::move(candidate);
                shaderTime = currentShaderTime;
                std::cout << "scene author: reloaded " << argv[3] << '\n';
            } else {
                shaderTime = currentShaderTime;
                std::cerr << "scene author: reload failed: " << reloadError << '\n';
            }
        }

        const auto now = std::chrono::steady_clock::now();
        const float elapsed = std::chrono::duration<float>(now - previous).count();
        previous = now;
        if (!paused) playhead = std::fmod(playhead + std::min(elapsed, 0.1f), 16.0f);
        const MusicFrame music = musicAt(playhead);
        int outputWidth = 0;
        int outputHeight = 0;
        SDL_GL_GetDrawableSize(window, &outputWidth, &outputHeight);
        const int sceneWidth = std::max(1, outputWidth / 2);
        if (!renderer->render(music, sceneAt(playhead, music), sceneWidth,
                              outputHeight, {0.36f, 0.70f, 0.94f}, 0, 1.0f,
                              frameSeconds, error)) {
            std::cerr << "scene author: render failed: " << error << '\n';
            break;
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, outputWidth, outputHeight);
        glUseProgram(program);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, renderer->texture());
        glUniform1i(glGetUniformLocation(program, "sceneFrame"), 0);
        glUniform2f(glGetUniformLocation(program, "resolution"),
                    static_cast<float>(outputWidth), static_cast<float>(outputHeight));
        glUniform4f(glGetUniformLocation(program, "signals"), music.kick,
                    music.snare, music.hat, music.energyFast);
        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        SDL_GL_SwapWindow(window);

        if (now - lastTitle >= std::chrono::milliseconds(200)) {
            std::ostringstream title;
            title << "Omadrop Pack Author | " << argv[3]
                  << " | continuous / ASCII | t " << std::fixed
                  << std::setprecision(1) << playhead << "s"
                  << " | K " << std::setprecision(2) << music.kick
                  << " S " << music.snare << " H " << music.hat
                  << " E " << music.energyFast
                  << (paused ? " | PAUSED" : " | Space pause, arrows seek");
            SDL_SetWindowTitle(window, title.str().c_str());
            lastTitle = now;
        }
    }

    renderer.reset();
    glDeleteVertexArrays(1, &vao);
    glDeleteProgram(program);
    SDL_GL_DeleteContext(context);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
