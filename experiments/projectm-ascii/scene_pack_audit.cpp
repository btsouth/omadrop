#include "native_renderer.h"

#include <GL/glew.h>
#include <SDL2/SDL.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <string>
#include <vector>

namespace {
constexpr int auditWidth = 320;
constexpr int auditHeight = 180;
constexpr float deltaThreshold = 0.0125f;
constexpr float responseFloor = 0.00025f;

struct Motion {
    float mean = 0.0f;
    float coverage = 0.0f;
    float globalPulse = 0.0f;
};

struct RoleAudit {
    Motion motion;
    float ratio = 0.0f;
    float recovery = 0.0f;
    std::vector<float> difference;
};

std::vector<float> differenceMap(const std::vector<float>& before,
                                 const std::vector<float>& after) {
    std::vector<float> result(before.size());
    for (std::size_t index = 0; index < before.size(); ++index) {
        result[index] = std::abs(after[index] - before[index]);
    }
    return result;
}

float similarity(const std::vector<float>& first,
                 const std::vector<float>& second) {
    float dot = 0.0f;
    float firstPower = 0.0f;
    float secondPower = 0.0f;
    for (std::size_t index = 0; index < first.size(); ++index) {
        dot += first[index] * second[index];
        firstPower += first[index] * first[index];
        secondPower += second[index] * second[index];
    }
    return dot / std::sqrt(std::max(1e-12f, firstPower * secondPower));
}

std::vector<float> readLuminance(GLuint texture, int width, int height) {
    std::vector<float> rgba(static_cast<std::size_t>(width * height * 4));
    glBindTexture(GL_TEXTURE_2D, texture);
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_FLOAT, rgba.data());
    std::vector<float> result(static_cast<std::size_t>(width * height));
    for (std::size_t pixel = 0; pixel < result.size(); ++pixel) {
        const std::size_t offset = pixel * 4;
        result[pixel] = 0.299f * rgba[offset]
                      + 0.587f * rgba[offset + 1]
                      + 0.114f * rgba[offset + 2];
    }
    return result;
}

Motion motionBetween(const std::vector<float>& before,
                     const std::vector<float>& after) {
    Motion result;
    float absoluteTotal = 0.0f;
    float signedTotal = 0.0f;
    std::size_t changed = 0;
    for (std::size_t index = 0; index < before.size(); ++index) {
        const float delta = after[index] - before[index];
        absoluteTotal += std::abs(delta);
        signedTotal += delta;
        if (std::abs(delta) >= deltaThreshold) ++changed;
    }
    const float count = static_cast<float>(std::max<std::size_t>(1, before.size()));
    result.mean = absoluteTotal / count;
    result.coverage = static_cast<float>(changed) / count;
    const float coherence = std::abs(signedTotal / count)
                          / std::max(result.mean, 1e-7f);
    result.globalPulse = result.coverage * coherence;
    return result;
}

MusicFrame silentMusic() {
    MusicFrame music;
    music.bpm = 120.0f;
    return music;
}

MusicFrame quietMusic(int frame) {
    MusicFrame music;
    music.bpm = 120.0f;
    music.energyFast = 0.28f;
    music.energySlow = 0.30f;
    music.harmonic = 0.42f;
    music.percussive = 0.18f;
    music.clockConfidence = 0.88f;
    music.spectralCentroid = 0.46f;
    music.stereoWidth = 0.52f;
    music.rhythmicDensity = 0.30f;
    music.beatPhase = std::fmod(static_cast<float>(frame) / 30.0f, 1.0f);
    music.barPhase = std::fmod(static_cast<float>(frame) / 120.0f, 1.0f);
    music.phrasePhase = std::fmod(static_cast<float>(frame) / 480.0f, 1.0f);
    for (std::size_t index = 0; index < music.bandLevel.size(); ++index) {
        music.bandLevel[index] = 0.28f + 0.025f * static_cast<float>(index);
    }
    for (std::size_t index = 0; index < music.spectrumLevel.size(); ++index) {
        music.spectrumLevel[index] = 0.25f + 0.02f
            * std::sin(static_cast<float>(index) * 0.7f);
    }
    for (std::size_t index = 0; index < music.chroma.size(); ++index) {
        music.chroma[index] = index == 0 || index == 4 || index == 7 ? 0.72f : 0.08f;
    }
    return music;
}

NativeSceneState sceneState(const MusicFrame& music) {
    NativeSceneState scene;
    scene.currentScene = NativeSceneKind::DepthTunnel;
    scene.incomingScene = NativeSceneKind::DepthTunnel;
    scene.development = 0.65f;
    const bool active = music.energyFast > 0.01f || music.percussive > 0.01f
                     || music.harmonic > 0.01f;
    scene.drive = active ? 0.42f : 0.0f;
    scene.peak = active ? 0.18f : 0.0f;
    scene.sceneBeats = 12.0f;
    return scene;
}

bool render(NativeRenderer& renderer, const MusicFrame& music,
            int width, int height, std::string& error) {
    return renderer.render(music, sceneState(music), width, height,
                           {0.36f, 0.70f, 0.94f}, 0, 1.0f,
                           1.0f / 60.0f, error);
}

bool warm(NativeRenderer& renderer, bool silence, std::string& error,
          int frames = 75) {
    renderer.reset();
    for (int frame = 0; frame < frames; ++frame) {
        if (!render(renderer, silence ? silentMusic() : quietMusic(frame),
                    auditWidth, auditHeight, error)) return false;
    }
    return true;
}

RoleAudit auditRole(NativeRenderer& renderer, int role, float quietMotion,
                    std::string& error) {
    if (!warm(renderer, false, error)) return {};
    const std::vector<float> before = readLuminance(renderer.texture(),
                                                    auditWidth, auditHeight);
    MusicFrame gesture = quietMusic(75);
    if (role == 0) gesture.kick = 0.92f;
    if (role == 1) gesture.snare = 0.92f;
    if (role == 2) gesture.hat = 0.92f;
    if (!render(renderer, gesture, auditWidth, auditHeight, error)) return {};
    std::vector<float> previous = readLuminance(renderer.texture(),
                                                auditWidth, auditHeight);
    RoleAudit result;
    result.motion = motionBetween(before, previous);
    result.difference = differenceMap(before, previous);
    result.ratio = result.motion.mean / std::max(quietMotion, 1e-7f);

    float recoveryTotal = 0.0f;
    for (int frame = 0; frame < 10; ++frame) {
        if (!render(renderer, quietMusic(76 + frame), auditWidth,
                    auditHeight, error)) return {};
        const std::vector<float> current = readLuminance(
            renderer.texture(), auditWidth, auditHeight);
        recoveryTotal += motionBetween(previous, current).mean;
        previous = current;
    }
    result.recovery = recoveryTotal / 10.0f
                    / std::max(result.motion.mean, 1e-7f);
    return result;
}

void fail(std::vector<std::string>& failures, bool condition,
          const std::string& text) {
    if (condition) failures.push_back(text);
}
} // namespace

int main(int argc, char** argv) {
    if (argc != 6) {
        std::cerr << "usage: scene-pack-audit VERTEX_SHADER FRAGMENT_SHADER "
                     "QUIET_COVERAGE_LIMIT GLOBAL_PULSE_LIMIT FRAME_MS_LIMIT\n";
        return 2;
    }
    const float quietCoverageLimit = std::strtof(argv[3], nullptr);
    const float globalPulseLimit = std::strtof(argv[4], nullptr);
    const float frameMillisecondsLimit = std::strtof(argv[5], nullptr);

    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::cerr << "scene pack audit: SDL initialization failed\n";
        return 1;
    }
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_Window* window = SDL_CreateWindow("Omadrop scene pack audit", 0, 0,
        1280, 720, SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN);
    if (!window) {
        std::cerr << "scene pack audit: SDL window creation failed\n";
        SDL_Quit();
        return 1;
    }
    SDL_GLContext context = SDL_GL_CreateContext(window);
    if (!context) {
        std::cerr << "scene pack audit: OpenGL context creation failed\n";
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        std::cerr << "scene pack audit: GLEW initialization failed\n";
        return 1;
    }

    NativeRenderer renderer;
    std::string error;
    if (!renderer.initializeCustomShader(argv[1], argv[2], error)) {
        std::cerr << "scene pack audit: renderer rejected shader: " << error << '\n';
        return 1;
    }

    if (!warm(renderer, true, error, 90)) {
        std::cerr << "scene pack audit: " << error << '\n';
        return 1;
    }
    std::vector<float> previous = readLuminance(renderer.texture(),
                                                auditWidth, auditHeight);
    float silenceDrift = 0.0f;
    for (int frame = 0; frame < 30; ++frame) {
        if (!render(renderer, silentMusic(), auditWidth, auditHeight, error)) return 1;
        const std::vector<float> current = readLuminance(renderer.texture(),
                                                        auditWidth, auditHeight);
        silenceDrift += motionBetween(previous, current).mean / 30.0f;
        previous = current;
    }

    if (!warm(renderer, false, error)) return 1;
    previous = readLuminance(renderer.texture(), auditWidth, auditHeight);
    float quietMotion = 0.0f;
    float quietCoverage = 0.0f;
    for (int frame = 0; frame < 30; ++frame) {
        if (!render(renderer, quietMusic(75 + frame), auditWidth,
                    auditHeight, error)) return 1;
        const std::vector<float> current = readLuminance(renderer.texture(),
                                                        auditWidth, auditHeight);
        const Motion motion = motionBetween(previous, current);
        quietMotion += motion.mean / 30.0f;
        quietCoverage += motion.coverage / 30.0f;
        previous = current;
    }

    const RoleAudit kick = auditRole(renderer, 0, quietMotion, error);
    const RoleAudit snare = auditRole(renderer, 1, quietMotion, error);
    const RoleAudit hat = auditRole(renderer, 2, quietMotion, error);

    renderer.reset();
    for (int frame = 0; frame < 20; ++frame) {
        if (!render(renderer, quietMusic(frame), 1280, 720, error)) return 1;
    }
    glFinish();
    std::vector<float> frameTimes;
    frameTimes.reserve(120);
    for (int frame = 0; frame < 120; ++frame) {
        const auto started = std::chrono::steady_clock::now();
        if (!render(renderer, quietMusic(frame + 20), 1280, 720, error)) return 1;
        glFinish();
        frameTimes.push_back(std::chrono::duration<float, std::milli>(
            std::chrono::steady_clock::now() - started).count());
    }
    std::sort(frameTimes.begin(), frameTimes.end());
    const float p99Milliseconds = frameTimes[static_cast<std::size_t>(
        std::floor((frameTimes.size() - 1) * 0.99f))];

    std::vector<std::string> failures;
    fail(failures, silenceDrift > 0.003f, "silence drift exceeds 0.003");
    fail(failures, quietCoverage > quietCoverageLimit,
         "quiet motion coverage exceeds declaration");
    const std::array<RoleAudit, 3> roles{kick, snare, hat};
    const std::array<const char*, 3> names{"kick", "snare", "hat"};
    for (std::size_t index = 0; index < roles.size(); ++index) {
        fail(failures, roles[index].motion.mean < responseFloor,
             std::string(names[index]) + " response is not visible");
        fail(failures, roles[index].ratio < 1.75f,
             std::string(names[index]) + " response is below 1.75x quiet motion");
        fail(failures, roles[index].motion.globalPulse > globalPulseLimit,
             std::string(names[index]) + " global pulse exceeds declaration");
        fail(failures, roles[index].recovery > 0.75f,
             std::string(names[index]) + " gesture does not recover within 167 ms");
    }
    const float minimumCoverage = std::min({kick.motion.coverage,
                                            snare.motion.coverage,
                                            hat.motion.coverage});
    const float maximumCoverage = std::max({kick.motion.coverage,
                                            snare.motion.coverage,
                                            hat.motion.coverage});
    fail(failures, minimumCoverage >= 0.60f
                   && maximumCoverage - minimumCoverage < 0.18f,
         "kick, snare, and hat all move most of the frame similarly");
    const float maximumSimilarity = std::max({
        similarity(kick.difference, snare.difference),
        similarity(kick.difference, hat.difference),
        similarity(snare.difference, hat.difference)});
    fail(failures, maximumSimilarity > 0.97f,
         "kick, snare, and hat do not have distinct visual roles");
    fail(failures, p99Milliseconds > frameMillisecondsLimit,
         "720p p99 frame time exceeds declaration");

    std::cout << std::fixed << std::setprecision(6)
              << "scene_pack_audit silence=" << silenceDrift
              << " quiet_motion=" << quietMotion
              << " quiet_coverage=" << quietCoverage
              << " kick_ratio=" << kick.ratio
              << " kick_motion=" << kick.motion.mean
              << " kick_coverage=" << kick.motion.coverage
              << " kick_pulse=" << kick.motion.globalPulse
              << " snare_ratio=" << snare.ratio
              << " snare_motion=" << snare.motion.mean
              << " snare_coverage=" << snare.motion.coverage
              << " snare_pulse=" << snare.motion.globalPulse
              << " hat_ratio=" << hat.ratio
              << " hat_motion=" << hat.motion.mean
              << " hat_coverage=" << hat.motion.coverage
              << " hat_pulse=" << hat.motion.globalPulse
              << " max_recovery=" << std::max({kick.recovery, snare.recovery,
                                                hat.recovery})
              << " max_role_similarity=" << maximumSimilarity
              << " p99_ms=" << p99Milliseconds << '\n';
    for (const std::string& failure : failures) {
        std::cerr << "scene pack audit failed: " << failure << '\n';
    }

    renderer.shutdown();
    SDL_GL_DeleteContext(context);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return failures.empty() ? 0 : 1;
}
