#include "native_renderer.h"

#include <GL/glew.h>
#include <SDL2/SDL.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <unistd.h>
#include <vector>

namespace {
constexpr int framesPerScene = 12 * 60;
constexpr int transitionFrames = 2 * 60;
constexpr float slowFrameMilliseconds = 1000.0f / 55.0f;

int environmentDimension(const char* name, int fallback) {
    const char* value = std::getenv(name);
    return value ? std::clamp(std::atoi(value), 320, 3840) : fallback;
}

std::size_t residentBytes() {
    std::ifstream statm("/proc/self/statm");
    std::size_t totalPages = 0;
    std::size_t residentPages = 0;
    if (!(statm >> totalPages >> residentPages)) return 0;
    return residentPages * static_cast<std::size_t>(sysconf(_SC_PAGESIZE));
}

float envelope(float phase, float width) {
    return phase < width ? std::exp(-phase * 5.0f / width) : 0.0f;
}

MusicFrame musicAt(std::size_t frame) {
    const float seconds = static_cast<float>(frame) / 60.0f;
    const float beat = std::fmod(seconds * 2.0f, 1.0f);
    const float halfBeat = std::fmod(seconds * 4.0f, 1.0f);
    const float energy = 0.50f + 0.32f * std::sin(seconds * 0.071f);
    MusicFrame music;
    music.audioTimeSeconds = seconds;
    music.bpm = 120.0f;
    music.beatPhase = beat;
    music.barPhase = std::fmod(seconds * 0.5f, 1.0f);
    music.phrasePhase = std::fmod(seconds * 0.125f, 1.0f);
    music.kick = envelope(beat, 0.13f);
    music.snare = envelope(std::fmod(beat + 0.5f, 1.0f), 0.11f);
    music.hat = envelope(halfBeat, 0.10f) * 0.72f;
    music.beatPulse = envelope(beat, 0.24f);
    music.onsetPulse = std::max({music.kick, music.snare, music.hat});
    music.downbeat = music.barPhase < 0.08f
        ? envelope(music.barPhase, 0.08f) : 0.0f;
    music.section = std::fmod(seconds, 48.0f) < 0.12f ? 1.0f : 0.0f;
    music.energyFast = std::clamp(energy + 0.12f * music.kick, 0.0f, 1.0f);
    music.energySlow = std::clamp(energy, 0.0f, 1.0f);
    music.energySlope = 0.32f * 0.071f * std::cos(seconds * 0.071f);
    music.percussive = 0.42f + 0.42f * music.onsetPulse;
    music.harmonic = 0.62f + 0.18f * std::sin(seconds * 0.037f);
    music.spectralCentroid = 0.50f + 0.28f * std::sin(seconds * 0.053f);
    music.stereoWidth = 0.55f + 0.30f * std::sin(seconds * 0.031f);
    music.rhythmicDensity = 0.58f;
    music.syncopation = 0.31f + 0.20f * std::sin(seconds * 0.11f);
    music.tonalMotion = 0.30f + 0.22f * std::sin(seconds * 0.043f);
    music.harmonicChange = music.section;
    music.clockConfidence = 0.94f;
    for (std::size_t index = 0; index < music.bandLevel.size(); ++index) {
        const float fi = static_cast<float>(index);
        music.bandLevel[index] = 0.42f + 0.26f
            * std::sin(seconds * (0.09f + fi * 0.013f) + fi);
        music.bandFlux[index] = 0.18f + music.onsetPulse
            * (0.16f + 0.06f * fi);
    }
    for (std::size_t index = 0; index < music.spectrumLevel.size(); ++index) {
        const float fi = static_cast<float>(index);
        music.spectrumLevel[index] = 0.38f + 0.32f
            * std::sin(seconds * (0.07f + fi * 0.006f) + fi * 0.41f);
    }
    for (std::size_t index = 0; index < music.chroma.size(); ++index) {
        const float distance = std::abs(static_cast<float>(index)
            - std::fmod(seconds * 0.10f, 12.0f));
        music.chroma[index] = std::exp(-0.7f * std::min(distance, 12.0f - distance));
    }
    return music;
}

NativeSceneState sceneAt(std::size_t frame) {
    const std::size_t sceneCycle = frame / framesPerScene;
    const int sceneFrame = static_cast<int>(frame % framesPerScene);
    NativeSceneState scene;
    scene.currentScene = static_cast<NativeSceneKind>(sceneCycle % nativeSceneCount);
    scene.incomingScene = nativeSceneOffset(scene.currentScene, 1);
    scene.sceneBeats = static_cast<float>(sceneFrame) / 30.0f;
    scene.development = std::clamp(scene.sceneBeats / 16.0f, 0.0f, 1.0f);
    scene.drive = 0.55f;
    scene.peak = 0.38f;
    if (sceneFrame >= framesPerScene - transitionFrames) {
        scene.transitioning = true;
        const float progress = static_cast<float>(
            sceneFrame - (framesPerScene - transitionFrames)) / transitionFrames;
        scene.transition = progress * progress * (3.0f - 2.0f * progress);
    }
    return scene;
}
} // namespace

int main(int argc, char** argv) {
    if (argc < 2 || argc > 4) {
        std::cerr << "usage: native-renderer-soak SHADER_DIRECTORY [MINUTES] [SCENE]\n";
        return 2;
    }
    const int minutes = argc >= 3 ? std::atoi(argv[2]) : 10;
    if (minutes < 1 || minutes > 240) {
        std::cerr << "native-renderer-soak: minutes must be between 1 and 240\n";
        return 2;
    }
    NativeSceneKind selectedScene{};
    const bool singleScene = argc == 4;
    if (singleScene && !nativeSceneFromName(argv[3], selectedScene)) {
        std::cerr << "native-renderer-soak: unknown scene\n";
        return 2;
    }
    const int width = environmentDimension("OMADROP_SOAK_WIDTH", 1920);
    const int height = environmentDimension("OMADROP_SOAK_HEIGHT", 1080);

    if (SDL_Init(SDL_INIT_VIDEO) != 0) return 1;
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_Window* window = SDL_CreateWindow("Omadrop renderer soak", 0, 0,
        width, height, SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN);
    if (!window) return 1;
    SDL_GLContext context = SDL_GL_CreateContext(window);
    if (!context) return 1;
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) return 1;

    NativeRenderer renderer;
    std::string error;
    if (!renderer.initialize(argv[1], error)) {
        std::cerr << error << '\n';
        return 1;
    }
    const std::array<float, 3> albumColor{0.34f, 0.72f, 0.92f};
    for (std::size_t index = 0; index < nativeSceneCount; ++index) {
        NativeSceneState scene;
        scene.currentScene = static_cast<NativeSceneKind>(index);
        scene.incomingScene = nativeSceneOffset(scene.currentScene, 1);
        if (!renderer.render(musicAt(index), scene, width, height, albumColor,
                             0, 1.0f, 1.0f / 60.0f, error)) {
            std::cerr << error << '\n';
            return 1;
        }
    }
    glFinish();
    const std::size_t baselineResident = residentBytes();
    std::size_t peakResident = baselineResident;
    std::vector<float> frameTimes;
    const std::size_t totalFrames = static_cast<std::size_t>(minutes) * 60 * 60;
    frameTimes.reserve(totalFrames);
    std::size_t slowStreak = 0;
    std::size_t maximumSlowStreak = 0;

    for (std::size_t frame = 0; frame < totalFrames; ++frame) {
        const auto started = std::chrono::steady_clock::now();
        auto state = sceneAt(frame);
        if (singleScene) {
            state.currentScene = state.incomingScene = selectedScene;
            state.transitioning = false;
        }
        if (!renderer.render(musicAt(frame), state, width, height,
                             albumColor, 0, 1.0f, 1.0f / 60.0f, error)) {
            std::cerr << error << '\n';
            return 1;
        }
        glFinish();
        const float milliseconds = std::chrono::duration<float, std::milli>(
            std::chrono::steady_clock::now() - started).count();
        frameTimes.push_back(milliseconds);
        if (milliseconds > slowFrameMilliseconds) {
            maximumSlowStreak = std::max(maximumSlowStreak, ++slowStreak);
        } else {
            slowStreak = 0;
        }
        if (frame % 60 == 0) {
            if (glGetError() != GL_NO_ERROR) {
                std::cerr << "native-renderer-soak: OpenGL error at frame "
                          << frame << '\n';
                return 1;
            }
            peakResident = std::max(peakResident, residentBytes());
        }
    }

    std::sort(frameTimes.begin(), frameTimes.end());
    double totalMilliseconds = 0.0;
    for (const float value : frameTimes) totalMilliseconds += value;
    const float mean = static_cast<float>(totalMilliseconds / frameTimes.size());
    const float p99 = frameTimes[static_cast<std::size_t>(
        std::floor((frameTimes.size() - 1) * 0.99))];
    const float maximum = frameTimes.back();
    const std::size_t memoryGrowth = peakResident > baselineResident
        ? peakResident - baselineResident : 0;
    constexpr std::size_t memoryGrowthLimit = 64u * 1024u * 1024u;
    std::cout << "renderer_soak simulated_minutes=" << minutes
              << " scene=" << (singleScene ? argv[3] : "all")
              << " frames=" << totalFrames
              << " resolution=" << width << 'x' << height
              << " mean_ms=" << mean
              << " p99_ms=" << p99
              << " max_ms=" << maximum
              << " max_slow_streak=" << maximumSlowStreak
              << " rss_growth_mib="
              << memoryGrowth / static_cast<double>(1024u * 1024u) << '\n';

    renderer.shutdown();
    SDL_GL_DeleteContext(context);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return p99 <= 18.5f && maximumSlowStreak < 60
        && memoryGrowth <= memoryGrowthLimit ? 0 : 1;
}
